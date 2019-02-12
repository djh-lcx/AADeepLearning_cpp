#ifndef __NET_HPP__
#define __NET_HPP__

#include "Layer.hpp"
#include "Blob.hpp"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>

using std::unordered_map;
using std::string;
using std::vector;
using std::shared_ptr;

struct NetParam      //c++�У�struct��class�÷�����һ�£���Ҫ�����Ǽ̳к�����ݷ���Ȩ�ޡ�
{
	/*ѧϰ��*/
	double lr;
	/*ѧϰ��˥��ϵ��*/
	double lr_decay;
	/*�Ż��㷨,:sgd/momentum/rmsprop*/
	std::string optimizer;
	/*momentumϵ�� */
	double momentum;
	/*epoch���� */
	int num_epochs;
	/*�Ƿ�ʹ��mini-batch�ݶ��½�*/
	bool use_batch;
	/*ÿ������������*/
	int batch_size;
	/*ÿ������������������һ��׼ȷ�ʣ� */
	int eval_interval;
	/*�Ƿ����ѧϰ�ʣ�  true/false*/
	bool lr_update;
	/* �Ƿ񱣴�ģ�Ϳ��գ����ձ�����*/
	bool snap_shot;
	/*ÿ�������������ڱ���һ�ο��գ�*/
	int snapshot_interval;
	/* �Ƿ����fine-tune��ʽѵ��*/
	bool fine_tune;
	/*Ԥѵ��ģ���ļ�.gordonmodel����·��*/
	string preTrainModel;

	/*����*/
	vector <string> layers;
	/*������*/
	vector <string> ltypes;

	/*�����������, �������Ϣ*/
	unordered_map<string, Param> lparams;


	void readNetParam(string file);


};


class Net
{
public:
	void init(NetParam& param, vector<shared_ptr<Blob>> x, vector<shared_ptr<Blob>> y);
	void train(NetParam& net_param);
	void train_with_batch(shared_ptr<Blob>& X, shared_ptr<Blob>& Y, NetParam& param, string mode = "TRAIN");

	void optimizer_with_batch(NetParam& param);
	void evaluate_with_batch(NetParam& param);
	double calc_accuracy(Blob& Y, Blob& Predict);

private:
	// ѵ����
	shared_ptr<Blob> X_train_;
	shared_ptr<Blob> Y_train_;
	// ��֤��
	shared_ptr<Blob> X_val_;
	shared_ptr<Blob> Y_val_;

	vector<string> layers_; //����
	vector<string> ltypes_; //������
	double loss_;
	double train_accu_;
	double val_accu_;

	unordered_map<string, shared_ptr<Layer>> myLayers_;

	unordered_map<string, vector<shared_ptr<Blob>>> data_; //ǰ�������Ҫ�õ���Blob data_[0]=X,  data_[1]=W,data_[2] = b;
	unordered_map<string, vector<shared_ptr<Blob>>> diff_; //ǰ�������Ҫ�õ���Blob data_[0]=X,  data_[1]=W,data_[2] = b;
	unordered_map<string, vector<int>> outShapes_; //�洢ÿһ�������ߴ�
};

#endif