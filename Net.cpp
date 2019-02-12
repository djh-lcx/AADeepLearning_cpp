#include "Net.hpp"
#include "Blob.hpp"
#include <json/json.h>
#include <fstream>
#include <cassert>
#include <memory>

#include<windows.h>

using namespace std;//ʹ��std�����ռ�


void NetParam::readNetParam(string file)
{
	ifstream ifs;
	ifs.open(file);
	assert(ifs.is_open());   //���ԣ�ȷ��json�ļ���ȷ��
	Json::Reader reader;  //  ������
	Json::Value value;      //�洢��
	if (reader.parse(ifs, value))
	{
		if (!value["train"].isNull())
		{
			//���þ��Ǳ�������˼
			auto &tparam = value["train"];  //ͨ�����÷�ʽ�������õ���train���������������Ԫ��
			this->lr = tparam["learning rate"].asDouble(); //������Double���ʹ��
			this->lr_decay = tparam["lr decay"].asDouble();
			//this->update = tparam["update method"].asString();//������String���ʹ��
			this->optimizer = tparam["optimizer"].asString();//������String���ʹ��
			this->momentum = tparam["momentum parameter"].asDouble();
			this->num_epochs = tparam["num epochs"].asInt();//������Int���ʹ��
			this->use_batch = tparam["use batch"].asBool();//������Bool���ʹ��
			this->batch_size = tparam["batch size"].asInt();
			this->eval_interval = tparam["evaluate interval"].asInt();
			this->lr_update = tparam["lr update"].asBool();
			this->snap_shot = tparam["snapshot"].asBool();
			this->snapshot_interval = tparam["snapshot interval"].asInt();
			this->fine_tune = tparam["fine tune"].asBool();
			this->preTrainModel = tparam["pre train model"].asString();//������String���ʹ��
		}
		if (!value["net"].isNull())
		{
			auto &nparam = value["net"];                                //ͨ�����÷�ʽ���õ���net��������������ж���
			for (int i = 0; i < (int)nparam.size(); ++i)                //������net��������������ж���
			{
				auto &ii = nparam[i];                                          //ͨ�����÷�ʽ���õ���ǰ�������������Ԫ��
				this->layers.push_back(ii["name"].asString());  //������vector�жѵ�����      a=[]   a.append()
				this->ltypes.push_back(ii["type"].asString());   //��������vector�жѵ�����

				if (ii["type"].asString() == "Conv")
				{
					int num = ii["kernel num"].asInt();
					int width = ii["kernel width"].asInt();
					int height = ii["kernel height"].asInt();
					int pad = ii["pad"].asInt();
					int stride = ii["stride"].asInt();

					this->lparams[ii["name"].asString()].conv_stride = stride;
					this->lparams[ii["name"].asString()].conv_kernels = num;
					this->lparams[ii["name"].asString()].conv_pad = pad;
					this->lparams[ii["name"].asString()].conv_width = width;
					this->lparams[ii["name"].asString()].conv_height = height;
				}
				if (ii["type"].asString() == "Pool")
				{
					int width = ii["kernel width"].asInt();
					int height = ii["kernel height"].asInt();
					int stride = ii["stride"].asInt();
					this->lparams[ii["name"].asString()].pool_stride = stride;
					this->lparams[ii["name"].asString()].pool_width = width;
					this->lparams[ii["name"].asString()].pool_height = height;
				}
				if (ii["type"].asString() == "Fc")
				{
					int num = ii["kernel num"].asInt();
					this->lparams[ii["name"].asString()].fc_kernels = num;
				}
			}
		}


	}

};

void Net::init(NetParam& param, vector<shared_ptr<Blob>> x, vector<shared_ptr<Blob>> y)
{
	shared_ptr<Layer> myLayer(NULL);
	layers_ = param.layers;   // ������param.layers����Ϊvector<string>
	ltypes_ = param.ltypes;    // ������ , param.ltypes����Ϊvector<string>

	X_train_ = x[0];
	Y_train_ = y[0];
	X_val_ = x[1];
	Y_val_ = y[1];

	for (int i = 0; i < layers_.size(); ++i)
	{
		data_[layers_[i]] = vector<shared_ptr<Blob>>(3, NULL); //Ϊÿһ�㴴��ǰ�����Ҫ�õ���3��Blob
		diff_[layers_[i]] = vector<shared_ptr<Blob>>(3, NULL); //Ϊÿһ�㴴��ǰ�����Ҫ�õ���3��Blob
		outShapes_[layers_[i]] = vector<int>(4);  //���建�棬�洢ÿһ�������ߴ�
	}

	vector<int> inShape = {
		param.batch_size,
		X_train_->get_channel(),
		X_train_->get_height(),
		X_train_->get_width(),
	};


	for (int i = 0; i < layers_.size()-1; i++)
	{
		string layer_name = layers_[i];
		string layer_type = ltypes_[i];
		if (layer_type == "Conv")
		{
			myLayer.reset(new ConvLayer);
		}
		else if (layer_type == "Relu")
		{
			myLayer.reset(new ReluLayer);
		}
		//else if (layer_type == "Tanh")
		//{

		//}
		else if (layer_type == "Pool")
		{
			myLayer.reset(new PoolLayer);
		}
		else if (layer_type == "Fc")
		{
			myLayer.reset(new FcLayer);
		}

		myLayers_[layer_name] = myLayer;
		myLayer->initLayer(inShape, layer_name, data_[layer_name], param.lparams[layer_name]);
		myLayer->calcShape(inShape, outShapes_[layer_name], param.lparams[layer_name]);
		inShape.assign(outShapes_[layer_name].begin(), outShapes_[layer_name].end());
		cout << layer_name << ".outShapes_->(" << outShapes_[layer_name][0] << "," << outShapes_[layer_name][1] << "," << outShapes_[layer_name][2] << "," << outShapes_[layer_name][3] << ")" << endl;

	}

	//data_["conv1"][1]->print("conv1WΪ��");
	//data_["conv1"][2]->print("conv1 bΪ��");
	//data_["fc1"][1]->print("fc1 WΪ��");
	//data_["fc1"][2]->print("fc1 bΪ��");
}

void Net::train(NetParam& net_param)
{

	int N = X_train_->get_batch_size();
	
	cout << "N = " << N << endl;
	int iter_per_epoch = N / net_param.batch_size;  //59000/200 = 295
	//�ܵ�������������������= ����epoch���������� * epoch����
	int num_batchs = iter_per_epoch * net_param.num_epochs;  // 295 * 2 = 590
	cout << "num_batchs(iterations) = " << num_batchs << endl;


	for (int iter = 0; iter < num_batchs; ++iter)
	//for (int iter = 0; iter < 1; ++iter)
	{
		//----------step1. ������ѵ�����л�ȡһ��mini-batch
		shared_ptr<Blob> X_batch;
		shared_ptr<Blob> Y_batch;

		X_batch.reset(new Blob(X_train_->subBlob((iter*net_param.batch_size) % N, ((iter+1)*net_param.batch_size) % N)));

		Y_batch.reset(new Blob(Y_train_->subBlob((iter*net_param.batch_size) % N, ((iter + 1)*net_param.batch_size) % N)));

		//----------step2. �ø�mini-batchѵ������ģ��
		train_with_batch(X_batch, Y_batch, net_param);

		//----------step3. ����ģ�͵�ǰ׼ȷ�ʣ�ѵ��������֤����
		evaluate_with_batch(net_param);
		printf("iter_%d    lr: %0.6f    loss: %f    train_acc: %0.2f%%    val_acc: %0.2f%%\n",
			iter, net_param.lr, loss_, train_accu_ * 100, val_accu_ * 100);
	}
}


void Net::train_with_batch(shared_ptr<Blob>& X, shared_ptr<Blob>& Y, NetParam& param, string mode)
{
	//------- step1. ��mini-batch��䵽��ʼ���X����
	data_[layers_[0]][0] = X;
	data_[layers_.back()][1] = Y;

	//------- step2. ���ǰ�����
	int n = layers_.size();  //����
	for (int i = 0; i < n-1; ++i)
	{
		string layer_name = layers_[i];
		shared_ptr<Blob> out;
		myLayers_[layer_name]->forward(data_[layer_name], out, param.lparams[layer_name]);
		data_[layers_[i + 1]][0] = out; //��ǰ������������һ�������

		//cout << "test " << endl;
	}

	//------- step3. softmaxǰ�����ͼ������ֵ
	SoftmaxLossLayer::softmax_cross_entropy_with_logits(data_[layers_.back()], loss_, diff_[layers_.back()][0]);
	//cout << "loss_=" << loss_ << endl;   //��һ�ε�������ʧֵԼΪ2.3
	if (mode == "TEST")//���������ǰ�򴫲��������ԣ���ѵ����������ǰ�˳���������ִ������ķ��򴫲����Ż�
		return;
	//------- step4. ��㷴�򴫲�     //conv1<-relu1<-pool1<-fc1<-softmax
	for (int i = n - 2; i >= 0; --i)
	{
		string lname = layers_[i];
		myLayers_[lname]->backward(diff_[layers_[i + 1]][0], data_[lname], diff_[lname], param.lparams[lname]);
	}

	//----------step5. �������£������ݶ��½���
	optimizer_with_batch(param);
}

void Net::optimizer_with_batch(NetParam& param)
{
	for (auto lname : layers_)    //for lname in layers_
	{

		//(1).����û��w��b�Ĳ�
		if (!data_[lname][1] || !data_[lname][2])
		{
			continue;  //��������ѭ��������ִ��ѭ����ע�ⲻ����break����ֱ������ѭ����
		}

		//cout << "lname=" << lname << endl;
		//Sleep(1000);
		//(2).�����ݶ��½�������w��b�Ĳ�
		for (int i = 1; i <= 2; ++i)
		{
			assert(param.optimizer == "sgd" || param.optimizer == "momentum" || param.optimizer == "rmsprop");//sgd/momentum/rmsprop
			//w:=w-param.lr*dw ;    b:=b-param.lr*db     ---->  "sgd"
			shared_ptr<Blob> dparam(new Blob(data_[lname][i]->size(), TZEROS));
			(*dparam) = -param.lr * (*diff_[lname][i]);
			(*data_[lname][i]) = (*data_[lname][i]) + (*dparam);
		}
	}
	//ѧϰ�ʸ���
	if (param.lr_update)
		param.lr *= param.lr_decay;
}

void Net::evaluate_with_batch(NetParam& param)
{
	//(1).����ѵ����׼ȷ��
	shared_ptr<Blob> X_train_subset;
	shared_ptr<Blob> Y_train_subset;
	int N = X_train_->get_batch_size();
	if (N > 1000)
	{
		X_train_subset.reset(new Blob(X_train_->subBlob(0, 1000)));
		Y_train_subset.reset(new Blob(Y_train_->subBlob(0, 1000)));
	}
	else
	{
		X_train_subset = X_train_;
		Y_train_subset = Y_train_;
	}
	train_with_batch(X_train_subset, Y_train_subset, param, "TEST");  //��TEST��������ģʽ��ֻ����ǰ�򴫲�
	train_accu_ = calc_accuracy(*data_[layers_.back()][1], *data_[layers_.back()][0]);

	//(2).������֤��׼ȷ��
	train_with_batch(X_val_, Y_val_, param, "TEST");  //��TEST��������ģʽ��ֻ����ǰ�򴫲�
	val_accu_ = calc_accuracy(*data_[layers_.back()][1], *data_[layers_.back()][0]);
}

double Net::calc_accuracy(Blob& Y, Blob& Predict)
{
	//(1). ȷ����������Blob�ߴ�һ��
	vector<int> size_Y = Y.size();
	vector<int> size_P = Predict.size();
	for (int i = 0; i < 4; ++i)
	{
		assert(size_Y[i] == size_P[i]);  //���ԣ���������Blob�ĳߴ磨N,C,H,W��һ����
	}
	//(2). ��������cube�����������ҳ���ǩֵY��Ԥ��ֵPredict���ֵ����λ�ý��бȽϣ���һ�£�����ȷ����+1
	int N = Y.get_batch_size();  //��������
	int right_cnt = 0;  //��ȷ����
	for (int n = 0; n < N; ++n)
	{
		//�ο���ַ��http://arma.sourceforge.net/docs.html#index_min_and_index_max_member
		if (Y[n].index_max() == Predict[n].index_max())
			right_cnt++;
	}
	return (double)right_cnt / (double)N;   //����׼ȷ�ʣ����أ�׼ȷ��=��ȷ����/����������
}