#include "Layer.hpp"
#include<opencv2/opencv.hpp>

using namespace std;
using namespace arma;
void ConvLayer::initLayer(const vector<int>& inShape, const string& lname, vector<shared_ptr<Blob>>& in, const Param& param)
{
	int kernel_number = param.conv_kernels;
	int channel = inShape[1];
	int height = param.conv_height;
	int width = param.conv_width;

	//2.��ʼ���洢W��b��Blob  (in[1]->W��in[2]->b)
	if (!in[1])   //�洢W��Blob��Ϊ��
	{
		in[1].reset(new Blob(kernel_number, channel, height, width, TRANDN)); //��׼��˹��ʼ������= 0�ͦ�= 1��    //np.randn()*0.01
		//(*in[1]) *= 1e-2;
		cout << "initLayer: " << lname << "  Init weights  with standard Gaussian ;" << endl;
	}

	//2.��ʼ���洢W��b��Blob  (in[1]->W��in[2]->b)
	if (!in[2])   //�洢W��Blob��Ϊ��
	{
		in[2].reset(new Blob(kernel_number, 1, 1, 1, TRANDN)); //��׼��˹��ʼ������= 0�ͦ�= 1��    //np.randn()*0.01
		//(*in[2]) *= 1e-2;
		cout << "initLayer: " << lname << "  Init bias  with standard Gaussian ;" << endl;
	}
}

void ConvLayer::calcShape(const vector<int>& inShape, vector<int>& outShape, const Param& param)
{
	int batch_size = inShape[0];
	int channel = inShape[1];
	int height = inShape[2];
	int width = inShape[3];

	int kernel_number = param.conv_kernels;
	int conv_height = param.conv_height;
	int conv_width = param.conv_width;
	int conv_pad = param.conv_pad;
	int conv_stride = param.conv_stride;

	outShape[0] = batch_size; //���������
	outShape[1] = kernel_number; //���ͨ����
	outShape[2] = (height + 2 * conv_pad - conv_height) / conv_stride + 1; //�����
	outShape[3] = (width + 2 * conv_pad - conv_width) / conv_stride + 1; //�����
}

template<typename T>
void Arma_mat2cv_mat(const arma::Mat<T>& arma_mat_in, cv::Mat_<T>& cv_mat_out)  //��arma::Matת��Ϊcv::Mat
{
	cv::transpose(cv::Mat_<T>(static_cast<int>(arma_mat_in.n_cols),
		static_cast<int>(arma_mat_in.n_rows),
		const_cast<T*>(arma_mat_in.memptr())), cv_mat_out);
	return;
};

void visiable(const cube& in, vector<cv::Mat_<double>>& vec_mat)   //���ӻ�һ��cube�е�����Mat
{
	int num = in.n_slices;
	for (int i = 0; i < num; ++i)
	{
		cv::Mat_<double> mat_cv;
		arma::mat mat_arma = in.slice(i);
		Arma_mat2cv_mat<double>(mat_arma, mat_cv);
		vec_mat.push_back(mat_cv);
	}
	return;
}

void ConvLayer::forward(const vector<shared_ptr<Blob>>& in, shared_ptr<Blob>& out, const Param& param)
{
	if (out)
		out.reset();
	//-------step1.��ȡ��سߴ磨���룬����ˣ������
	assert(in[0]->get_channel() == in[1]->get_channel());  //���ԣ�����Blobͨ�����;����Blobͨ����һ������ر�֤��һ�㣩

	int N = in[0]->get_batch_size();        //����Blob��cube��������batch����������
	int C = in[0]->get_channel();         //����Blobͨ����
	int Hx = in[0]->get_height();      //����Blob��
	int Wx = in[0]->get_width();    //����Blob��

	int F = in[1]->get_batch_size();		  //����˸���
	int Hw = in[1]->get_height();     //����˸�
	int Ww = in[1]->get_width();   //����˿�

	int Ho = (Hx + param.conv_pad * 2 - Hw) / param.conv_stride + 1;    //���Blob�ߣ������
	int Wo = (Wx + param.conv_pad * 2 - Ww) / param.conv_stride + 1;  //���Blob�������
	//-------step2.����Ҫ����padding����
	Blob padX = in[0]->pad(param.conv_pad);
	//padX[0].slice(0).print("pad=");
	//arma::mat mat0_arma = padX[0].slice(0);
	//cv::Mat_<double> mat0_cv;
	//Arma_mat2cv_mat(mat0_arma, mat0_cv);
	//-------step3.��ʼ�������
	out.reset(new Blob(N, F, Ho, Wo));
	for (int n = 0; n < N; ++n)   //���cube��
	{
		for (int f = 0; f < F; ++f)  //���ͨ����
		{
			for (int hh = 0; hh < Ho; ++hh)   //���Blob�ĸ�
			{
				for (int ww = 0; ww < Wo; ++ww)   //���Blob�Ŀ�
				{
					cube window = padX[n](span(hh*param.conv_stride, hh*param.conv_stride + Hw - 1),
						span(ww*param.conv_stride, ww*param.conv_stride + Ww - 1),
						span::all);
					//out = Wx+b
					(*out)[n](hh, ww, f) = accu(window % (*in[1])[f]) + as_scalar((*in[2])[f]);    //b = (F,1,1,1)
				}
			}
		}
	}
	//(*in[1])[0].print("W=");
	//cout << "b=\n" << as_scalar((*in[2])[0]) << "\n" << endl;
	//(*out)[0].slice(0).print("out=");

	//vector < cv::Mat_<double>> vec_mat_w1;
	//visiable((*in[1])[0], vec_mat_w1);    //���ӻ���һ�������

	//vector < cv::Mat_<double>> vec_mat_b1;
	//visiable((*in[2])[0], vec_mat_b1);    //���ӻ���һ��ƫ�ú�

	//vector < cv::Mat_<double>> vec_mat_out;
	//visiable((*out)[0], vec_mat_out);    //���ӻ���һ�����cube


	return;
}

void ConvLayer::backward(const shared_ptr<Blob>& din,   //�����ݶ�
	const vector<shared_ptr<Blob>>& cache,
	vector<shared_ptr<Blob>>& grads,
	const Param& param)
{
	//step1. ��������ݶ�Blob�ĳߴ磨dX---grads[0]��
	grads[0].reset(new Blob(cache[0]->size(), TZEROS));
	grads[1].reset(new Blob(cache[1]->size(), TZEROS));
	grads[2].reset(new Blob(cache[2]->size(), TZEROS));
	//step2. ��ȡ�����ݶ�Blob�ĳߴ磨din��
	int Nd = din->get_batch_size();        //�����ݶ�Blob��cube��������batch����������
	int Cd = din->get_channel();         //�����ݶ�Blobͨ����
	int Hd = din->get_height();      //�����ݶ�Blob��
	int Wd = din->get_width();    //�����ݶ�Blob��
	//step3. ��ȡ�������ز���
	int Hw = param.conv_height;
	int Ww = param.conv_width;
	int stride = param.conv_stride;

	//step4. ������
	Blob pad_X = cache[0]->pad(param.conv_pad);  //����ʵ�ʷ��򴫲������Ӧ��������������Blob
	Blob pad_dX(pad_X.size(), TZEROS);                      //�ݶ�BlobӦ����ò������Blob�ߴ籣��һ��

	//step5. ��ʼ���򴫲�
	for (int n = 0; n < Nd; ++n)   //���������ݶ�din��������
	{
		for (int c = 0; c < Cd; ++c)  //���������ݶ�din��ͨ����
		{
			for (int hh = 0; hh < Hd; ++hh)   //���������ݶ�din�ĸ�
			{
				for (int ww = 0; ww < Wd; ++ww)   //���������ݶ�din�Ŀ�
				{
					//(1). ͨ���������ڣ���ȡ��ͬ��������Ƭ��
					cube window = pad_X[n](span(hh*stride, hh*stride + Hw - 1), span(ww*stride, ww*stride + Ww - 1), span::all);
					//(2). �����ݶ�
					//dX
					pad_dX[n](span(hh*stride, hh*stride + Hw - 1), span(ww*stride, ww*stride + Ww - 1), span::all) += (*din)[n](hh, ww, c) * (*cache[1])[c];
					//dW  --->grads[1]
					(*grads[1])[c] += (*din)[n](hh, ww, c) * window / Nd;
					//db   --->grads[2]
					(*grads[2])[c](0, 0, 0) += (*din)[n](hh, ww, c) / Nd;
				}
			}
		}
	}

	//step6. ȥ������ݶ��е�padding����
	(*grads[0]) = pad_dX.deletePad(param.conv_pad);

	////���Դ���
	//(*din)[0].slice(0).print("input:   din=");				    //�����ݶȣ���ӡ��һ��din�ĵ�һ������
	//(*din)[0].slice(1).print("input:   din=");				    //�����ݶȣ���ӡ��һ��din�ĵڶ�������
	//(*din)[0].slice(2).print("input:   din=");				    //�����ݶȣ���ӡ��һ��din�ĵ���������
	//(*cache[1])[0].slice(0).print("W1=");		                //��ӡ��һ������˵ĵ�һ������	
	//(*cache[1])[1].slice(0).print("W2=");		                //��ӡ�ڶ�������˵ĵ�һ������	
	//(*cache[1])[2].slice(0).print("W3=");		                //��ӡ����������˵ĵ�һ������		
	//pad_dX[0].slice(0).print("output:   pad_dX=");		//����ݶȣ���ӡ��һ��pad_dX�ĵ�һ������

	return;
}


void ReluLayer::initLayer(const vector<int>& inShape, const string& lname, vector<shared_ptr<Blob>>& in, const Param& param)
{
	cout << "ReluLayer::initLayer " << endl;
}
void ReluLayer::calcShape(const vector<int>& inShape, vector<int>& outShape, const Param& param)
{
	//�ߴ粻�䣬ֱ�Ӹ���
	outShape.assign(inShape.begin(), inShape.end());
}
void ReluLayer::forward(const vector<shared_ptr<Blob>>& in, shared_ptr<Blob>& out, const Param& param)
{
	if (out)
	{
		out.reset();
	}
	out.reset(new Blob(*in[0]));
	out->maxIn(0);
}
void ReluLayer::backward(const shared_ptr<Blob>& din,
	const vector<shared_ptr<Blob>>& cache,
	vector<shared_ptr<Blob>>& grads,
	const Param& param)
{
	//step1. ��������ݶ�Blob�ĳߴ磨dX---grads[0]��
	grads[0].reset(new Blob(*cache[0]));

	//step2. ��ȡ����mask
	int N = grads[0]->get_batch_size();
	for (int n = 0; n < N; ++n)
	{
		(*grads[0])[n].transform([](double e) {return e > 0 ? 1 : 0; });
	}
	(*grads[0]) = (*grads[0]) * (*din);

	//(*din)[0].slice(0).print("din=");				//�����ݶȣ���ӡ��һ��din�ĵ�һ������
	//(*cache[0])[0].slice(0).print("cache=");		//���룺 ��ӡ��һ��cache�ĵ�һ������
	//(*grads[0])[0].slice(0).print("grads=");		//����ݶȣ���ӡ��һ��grads�ĵ�һ������
	return;
}
void PoolLayer::initLayer(const vector<int>& inShape, const string& lname, vector<shared_ptr<Blob>>& in, const Param& param)
{
	cout << "PoolLayer::initLayer " << endl;
}
void PoolLayer::calcShape(const vector<int>& inShape, vector<int>& outShape, const Param& param)
{
	int batch_size = inShape[0];
	int channel = inShape[1];
	int height = inShape[2];
	int width = inShape[3];

	int pool_height = param.pool_height;
	int pool_width = param.pool_width;
	int pool_stride = param.pool_stride;

	outShape[0] = batch_size; //���������
	outShape[1] = channel; //���ͨ����
	outShape[2] = (height - pool_height) / pool_stride + 1; //�����
	outShape[3] = (width - pool_width) / pool_stride + 1; //�����
}
void PoolLayer::forward(const vector<shared_ptr<Blob>>& in, shared_ptr<Blob>& out, const Param& param)
{
	if (out)
		out.reset();
	//-------step1.��ȡ��سߴ磨���룬�ػ��ˣ������
	int N = in[0]->get_batch_size();        //����Blob��cube��������batch����������
	int C = in[0]->get_channel();         //����Blobͨ����
	int Hx = in[0]->get_height();      //����Blob��
	int Wx = in[0]->get_width();    //����Blob��

	int Hw = param.pool_height;     //�ػ��˸�
	int Ww = param.pool_width;   //�ػ��˿�

	int Ho = (Hx - Hw) / param.pool_stride + 1;    //���Blob�ߣ��ػ���
	int Wo = (Wx - Ww) / param.pool_stride + 1;  //���Blob���ػ���

	//-------step2.��ʼ�ػ�
	out.reset(new Blob(N, C, Ho, Wo));

	for (int n = 0; n < N; ++n)   //���cube��
	{
		for (int c = 0; c < C; ++c)  //���ͨ����
		{
			for (int hh = 0; hh < Ho; ++hh)   //���Blob�ĸ�
			{
				for (int ww = 0; ww < Wo; ++ww)   //���Blob�Ŀ�
				{
					(*out)[n](hh, ww, c) = (*in[0])[n](span(hh*param.pool_stride, hh*param.pool_stride + Hw - 1),
						span(ww*param.pool_stride, ww*param.pool_stride + Ww - 1),
						span(c, c)).max();
				}
			}
		}
	}
	//vector < cv::Mat_<double>> vec_mat_in;
	//visiable((*in[0])[0], vec_mat_in);    //���ӻ�����relu��ĵ�һ�����cube
	//vector < cv::Mat_<double>> vec_mat_out;
	//visiable((*out)[0], vec_mat_out);    //���ӻ�����relu��ĵ�һ�����cube
}

void PoolLayer::backward(const shared_ptr<Blob>& din,
	const vector<shared_ptr<Blob>>& cache,
	vector<shared_ptr<Blob>>& grads,
	const Param& param)
{
	//step1. ��������ݶ�Blob�ĳߴ磨dX---grads[0]��
	grads[0].reset(new Blob(cache[0]->size(), TZEROS));
	//step2. ��ȡ�����ݶ�Blob�ĳߴ磨din��
	int Nd = din->get_batch_size();        //�����ݶ�Blob��cube��������batch����������
	int Cd = din->get_channel();         //�����ݶ�Blobͨ����
	int Hd = din->get_height();      //�����ݶ�Blob��
	int Wd = din->get_width();    //�����ݶ�Blob��

	//step3. ��ȡ�ػ�����ز���
	int Hp = param.pool_height;
	int Wp = param.pool_width;
	int stride = param.pool_stride;

	//step4. ��ʼ���򴫲�
	for (int n = 0; n < Nd; ++n)   //���cube��
	{
		for (int c = 0; c < Cd; ++c)  //���ͨ����
		{
			for (int hh = 0; hh < Hd; ++hh)   //���Blob�ĸ�
			{
				for (int ww = 0; ww < Wd; ++ww)   //���Blob�Ŀ�
				{
					//(1). ��ȡ����mask
					mat window = (*cache[0])[n](span(hh*param.pool_stride, hh*param.pool_stride + Hp - 1),
						span(ww*param.pool_stride, ww*param.pool_stride + Wp - 1),
						span(c, c));
					double maxv = window.max();
					mat mask = conv_to<mat>::from(maxv == window);  //"=="���ص���һ��umat���͵ľ���umatת��Ϊmat
					//(2). �����ݶ�
					(*grads[0])[n](span(hh*param.pool_stride, hh*param.pool_stride + Hp - 1),
						span(ww*param.pool_stride, ww*param.pool_stride + Wp - 1),
						span(c, c)) += mask*(*din)[n](hh, ww, c);  //umat  -/-> mat
				}
			}
		}
	}
	//(*din)[0].slice(0).print("din=");
	//(*cache[0])[0].slice(0).print("cache=");  //mask
	//(*grads[0])[0].slice(0).print("grads=");

	return;
}

void FcLayer::initLayer(const vector<int>& inShape, const string& lname, vector<shared_ptr<Blob>>& in, const Param& param)
{
	int fc_kernels = param.fc_kernels;
	int channel = inShape[1];
	int height = inShape[2];
	int width = inShape[3];

	//2.��ʼ���洢W��b��Blob  (in[1]->W��in[2]->b)
	if (!in[1])   //�洢W��Blob��Ϊ��
	{
		in[1].reset(new Blob(fc_kernels, channel, height, width, TRANDN)); //��׼��˹��ʼ������= 0�ͦ�= 1��    //np.randn()*0.01
		(*in[1]) *= 1e-2;
		cout << "initLayer: " << lname << "  Init weights  with standard Gaussian ;" << endl;
	}

	//2.��ʼ���洢W��b��Blob  (in[1]->W��in[2]->b)
	if (!in[2])   //�洢W��Blob��Ϊ��
	{
		in[2].reset(new Blob(fc_kernels, 1, 1, 1, TRANDN)); //��׼��˹��ʼ������= 0�ͦ�= 1��    //np.randn()*0.01
		(*in[2]) *= 1e-2;
		cout << "initLayer: " << lname << "  Init bias  with standard Gaussian ;" << endl;
	}
}
void FcLayer::calcShape(const vector<int>& inShape, vector<int>& outShape, const Param& param)
{
	int batch_size = inShape[0];

	//��200,10,1,1��
	outShape[0] = batch_size; //���������
	outShape[1] = param.fc_kernels; //���ͨ������Ҳ������Ԫ����
	outShape[2] = 1; //�����
	outShape[3] = 1; //�����
}
void FcLayer::forward(const vector<shared_ptr<Blob>>& in, shared_ptr<Blob>& out, const Param& param)
{
	if (out)
		out.reset();
	//-------step1.��ȡ��سߴ磨���룬ȫ���Ӻˣ������
	int N = in[0]->get_batch_size();        //����Blob��cube��������batch����������
	int C = in[0]->get_channel();         //����Blobͨ����
	int Hx = in[0]->get_height();      //����Blob��
	int Wx = in[0]->get_width();    //����Blob��

	int F = in[1]->get_batch_size();		  //ȫ���Ӻ˸���
	int Hw = in[1]->get_height();     //ȫ���Ӻ˸�
	int Ww = in[1]->get_width();   //ȫ���Ӻ˿�
	assert(in[0]->get_channel() == in[1]->get_channel());  //���ԣ�����Blobͨ������ȫ���Ӻ�Blobͨ����һ������ر�֤��һ�㣩
	assert(Hx == Hw  && Wx == Ww);  //���ԣ�����Blob�ߺͿ��ȫ���Ӻ�Blob�ߺͿ�һ������ر�֤��һ�㣩

	int Ho = 1;    //���Blob�ߣ�ȫ���Ӳ�����
	int Wo = 1;  //���Blob��ȫ���Ӳ�����

	//-------step2.��ʼȫ��������
	out.reset(new Blob(N, F, Ho, Wo));

	for (int n = 0; n < N; ++n)   //���cube��
	{
		for (int f = 0; f < F; ++f)  //���ͨ����
		{
			(*out)[n](0, 0, f) = accu((*in[0])[n] % (*in[1])[f]) + as_scalar((*in[2])[f]);    //b = (F,1,1,1)
		}
	}

	//vector < cv::Mat_<double>> vec_mat_CUBE1;
	//visiable((*in[0])[0], vec_mat_CUBE1);    //���ӻ���һ��ȫ���Ӻ�

	//vector < cv::Mat_<double>> vec_mat_w1;
	//visiable((*in[1])[0], vec_mat_w1);    //���ӻ���һ��ȫ���Ӻ�

	//vector < cv::Mat_<double>> vec_mat_b1;
	//visiable((*in[2])[0], vec_mat_b1);    //���ӻ���һ��ƫ�ú�

	//vector < cv::Mat_<double>> vec_mat_out;
	//visiable((*out)[0], vec_mat_out);    //���ӻ���һ�����cube


}

void FcLayer::backward(const shared_ptr<Blob>& din,
	const vector<shared_ptr<Blob>>& cache,
	vector<shared_ptr<Blob>>& grads,
	const Param& param)
{
	//shared_ptr<Blob> din(new Blob(2, 2, 1, 1, TRANDU));
	//vector<shared_ptr<Blob>> cache(3, NULL);
	//cache[0].reset(new Blob(2, 2, 2, 2, TONES));
	//cache[1].reset(new Blob(2, 2, 2, 2, TRANDU));
	//cache[2].reset(new Blob(2, 1, 1, 1, TRANDU));

	//dX,dW,db  -> X,W,b
	grads[0].reset(new Blob(cache[0]->size(), TZEROS));
	grads[1].reset(new Blob(cache[1]->size(), TZEROS));
	grads[2].reset(new Blob(cache[2]->size(), TZEROS));
	int N = grads[0]->get_batch_size();
	int F = grads[1]->get_batch_size();
	assert(F == cache[1]->get_batch_size());

	for (int n = 0; n < N; ++n)
	{
		for (int f = 0; f < F; ++f)
		{
			//dX
			(*grads[0])[n] += (*din)[n](0, 0, f) * (*cache[1])[f];
			//dW
			(*grads[1])[f] += (*din)[n](0, 0, f) * (*cache[0])[n] / N;
			//db
			(*grads[2])[f] += (*din)[n](0, 0, f) / N;
		}
	}

	//vector < cv::Mat_<double>> vec_mat_dout0;
	//visiable((*din)[0], vec_mat_dout0);      //���ӻ���һ������źţ���㴫�����ݶȣ�

	//vector < cv::Mat_<double>> vec_mat_dout1;
	//visiable((*din)[1], vec_mat_dout1);      //���ӻ���һ������źţ���㴫�����ݶȣ�

	//vector < cv::Mat_<double>> vec_mat_x0;
	//visiable((*cache[0])[0], vec_mat_x0);     //���ӻ���һ������cube

	//vector < cv::Mat_<double>> vec_mat_x1;
	//visiable((*cache[0])[1], vec_mat_x1);     //���ӻ��ڶ�������cube

	//vector < cv::Mat_<double>> vec_mat_w0;
	//visiable((*cache[1])[0], vec_mat_w0);    //���ӻ���һ��ȫ���Ӻ�

	//vector < cv::Mat_<double>> vec_mat_w1;
	//visiable((*cache[1])[1], vec_mat_w1);    //���ӻ��ڶ���ȫ���Ӻ�

	//vector < cv::Mat_<double>> vec_mat_dx0;
	//visiable((*grads[0])[0], vec_mat_dx0);     //���ӻ���һ��dx

	//vector < cv::Mat_<double>> vec_mat_dw0;
	//visiable((*grads[1])[0], vec_mat_dw0);    //���ӻ���һ��dw

	//vector < cv::Mat_<double>> vec_mat_db0;
	//visiable((*grads[2])[0], vec_mat_db0);    //���ӻ���һ��db
	return;
}



void SoftmaxLossLayer::softmax_cross_entropy_with_logits(const vector<shared_ptr<Blob>>& in, double& loss, shared_ptr<Blob>& dout)
{
	if (dout)
		dout.reset();
	//-------step1.��ȡ��سߴ�
	int N = in[0]->get_batch_size();        //����Blob��cube��������batch����������
	int C = in[0]->get_channel();         //����Blobͨ����
	int Hx = in[0]->get_height();      //����Blob��
	int Wx = in[0]->get_width();    //����Blob��
	assert(Hx == 1 && Wx == 1);

	dout.reset(new Blob(N, C, Hx, Wx));   //��N,C,1,1��
	double loss_ = 0;
	//(*in[0])[0].print();
	//system("pause");
	for (int i = 0; i < N; ++i)
	{
		cube prob = arma::exp((*in[0])[i]) / arma::accu(arma::exp((*in[0])[i]));    //softmax��һ��
		loss_ += (-arma::accu((*in[1])[i] % arma::log(prob)));  //�ۼӸ��������Ľ�������ʧֵ
		//�ݶȱ��ʽ�Ƶ���https://blog.csdn.net/qian99/article/details/78046329
		(*dout)[i] = prob - (*in[1])[i];  //���������������������źţ������ݶȣ�
	}
	loss = loss_ / N;   //��ƽ����ʧ

	return;
}