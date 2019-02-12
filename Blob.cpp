#include "Blob.hpp"
#include "cassert"

using namespace std;
using namespace arma;

//���캯��
Blob::Blob(const int batch_size, const int channel, const int height, const int width, int type) : batch_size(batch_size), channel(channel), height(height), width(width)
{
	//arma_rng::set_seed_random();  //ϵͳ�����������(���û����һ�䣬�ͻ�ÿ����������(����)ʱ��Ĭ�ϴ�����1��ʼ�������������
	_init(batch_size, channel, height, width, type);
}
Blob::Blob(const vector<int> shape_, int type) : batch_size(shape_[0]), channel(shape_[1]), height(shape_[2]), width(shape_[3])
{
	//arma_rng::set_seed_random();  //ϵͳ�����������(���û����һ�䣬�ͻ�ÿ����������(����)ʱ��Ĭ�ϴ�����1��ʼ�������������
	_init(batch_size, channel, height, width, type);
}
//��ʼ��
void Blob::_init(const int batch_size, const int channel, const int height, const int width, int type)
{
	if (type == TZEROS){
		blob_data = vector<cube>(batch_size, cube(height, width, channel, fill::zeros));
	}
	else if (type == TONES){
		blob_data = vector<cube>(batch_size, cube(height, width, channel, fill::ones));
	}
	else if (type == TRANDU){
		for (int i = 0; i < batch_size; ++i){
			blob_data.push_back(arma::randu<cube>(height, width, channel));//�ѵ�
		}
	}
	else if (type == TRANDN){
		for (int i = 0; i < batch_size; ++i){
			blob_data.push_back(arma::randn<cube>(height, width, channel));//�ѵ�
		}
	}
	else{
		blob_data = vector<cube>(batch_size, cube(height, width, channel));
	}
}

vector<int> Blob::size() const
{
	vector<int> shape_{ batch_size,
		channel,
		height,
		width };
	return shape_;
}
//��ӡblob���������
void Blob::print(string str)
{
	assert(!blob_data.empty()); //���ԣ�   blob_data��Ϊ�գ�������ֹ����
	cout << str << endl;
	for (int i = 0; i < batch_size; i++)
	{
		printf("N = %d\n", i); //N_Ϊblob_data��cube����
		this->blob_data[i].print();//��һ��ӡcube������cube�����غõ�print()
	}

}
//���������
cube& Blob::operator[] (int i)
{
	return blob_data[i];
}

//���������
Blob& Blob::operator*= (const double k)
{
	for (int i = 0; i < batch_size; i++)
	{
		blob_data[i] = blob_data[i] * k;   //����cube��ʵ�ֵ�*������
	}
	return *this;
}
//���������
Blob& Blob::operator= (double val)
{
	for (int i = 0; i < batch_size; ++i)
	{
		blob_data[i].fill(val);   //����cube��ʵ�ֵ�*������
	}
	return *this;
}

Blob operator*(Blob& A, Blob& B)  //��Ԫ�����ľ���ʵ�֣�����û�����޶����� (Blob& Blob::)������ʽ
{
	//(1). ȷ����������Blob�ߴ�һ��
	vector<int> size_A = A.size();
	vector<int> size_B = B.size();
	for (int i = 0; i < 4; ++i)
	{
		assert(size_A[i] == size_B[i]);  //���ԣ���������Blob�ĳߴ磨N,C,H,W��һ����
	}
	//(2). �������е�cube��ÿһ��cube����Ӧλ����ˣ�cube % cube��
	int N = size_A[0];
	Blob C(A.size());
	for (int i = 0; i < N; ++i)
	{
		C[i] = A[i] % B[i];
	}
	return C;
}

Blob operator*(double num, Blob& B)
{
	//�������е�cube��ÿһ��cube������һ����ֵnum
	int N = B.get_batch_size();
	Blob out(B.size());
	for (int i = 0; i < N; ++i)
	{
		out[i] = num * B[i];
	}
	return out;
}

Blob operator+(Blob& A, Blob& B)
{
	//(1). ȷ����������Blob�ߴ�һ��
	vector<int> size_A = A.size();
	vector<int> size_B = B.size();
	for (int i = 0; i < 4; ++i)
	{
		assert(size_A[i] == size_B[i]);  //���ԣ���������Blob�ĳߴ磨N,C,H,W��һ����
	}
	//(2). �������е�cube��ÿһ��cube����Ӧλ����ӣ�cube + cube��
	int N = size_A[0];
	Blob C(A.size());
	for (int i = 0; i < N; ++i)
	{
		C[i] = A[i] + B[i];
	}
	return C;
}

vector<cube>& Blob::get_data()
{
	return blob_data;
}

Blob Blob::subBlob(int low_idx, int high_idx)
{
	//������ [0,1,2,3,4,5]  -> [1,3)  -> [1,2]
	if (high_idx > low_idx)
	{
		Blob tmp(high_idx - low_idx, channel, height, width);  // high_idx > low_idx
		for (int i = low_idx; i < high_idx; ++i)
		{
			tmp[i - low_idx] = (*this)[i];
		}
		return tmp;
	}
	else
	{
		// low_idx >high_idx
		//������ [0,1,2,3,4,5]  -> [3,2)-> (6 - 3) + (2 -0) -> [3,4,5,0]
		Blob tmp(batch_size - low_idx + high_idx, channel, height, width);
		for (int i = low_idx; i < batch_size; ++i)   //�ֿ����ν�ȡ���Ƚ�ȡ��һ��
		{
			tmp[i - low_idx] = (*this)[i];
		}
		for (int i = 0; i < high_idx; ++i)   //�ֿ����ν�ȡ���ٽ�ȡѭ������0��ʼ�����
		{
			tmp[i + batch_size - low_idx] = (*this)[i];
		}
		return tmp;
	}
}


Blob Blob::pad(int pad, double val)
{
	assert(!blob_data.empty());
	Blob padX(batch_size, channel, height + 2 * pad, width + 2 * pad);

	for (int n = 0; n < batch_size; ++n)
	{
		for (int c = 0; c < channel; ++c)
		{
			for (int h = 0; h < height; ++h)
			{
				for (int w = 0; w < width; ++w)
				{
					padX[n](h + pad, w + pad, c) = blob_data[n](h, w, c);
				}
			}
		}
	}
	return padX;
}
Blob Blob::deletePad(int pad)
{
	assert(!blob_data.empty());   //���ԣ�Blob����Ϊ��
	Blob out(batch_size, channel, height - 2 * pad, width - 2 * pad);
	for (int n = 0; n < batch_size; ++n)
	{
		for (int c = 0; c < channel; ++c)
		{
			for (int h = pad; h < height - pad; ++h)
			{
				for (int w = pad; w < width - pad; ++w)
				{
					//ע�⣬out�������Ǵ�0��ʼ�ģ�����Ҫ��ȥpad
					out[n](h - pad, w - pad, c) = blob_data[n](h, w, c);
				}
			}
		}
	}
	return out;
}
void Blob::maxIn(double val)
{
	assert(!blob_data.empty());
	for (int i = 0; i < batch_size; ++i)
	{
		/*
		.transform(lambda_function) (C++11 Only)  �������ֻ֧��c++11���ϰ汾�����ڱ�����������֧��c++11��
		����һ��lambda������ʵ������Ҫ�ı任���ܣ�
		������Mat, Col, Row��Cube�����ھ��󣬰���column-by-column�����б任��
		���������壬����slice-by-slice���б任��ÿһ��slice��һ������
		*/
		blob_data[i].transform([val](double e){return e>val ? e : val; });
	}
	return;
}