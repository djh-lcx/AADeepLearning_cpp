#ifndef  __BLOB_HPP__
#define __BLOB_HPP__
#include <vector>
#include <armadillo>
using std::vector;
using std::string;
using arma::cube;

enum FillType
{
	TONES = 1,  //cube����Ԫ�ض����Ϊ1
	TZEROS = 2, //cube����Ԫ�ض����Ϊ0
	TRANDU = 3,  //��Ԫ������Ϊ[0,1]�����ھ��ȷֲ������ֵ
	TRANDN = 4,  //ʹ�æ�= 0�ͦ�= 1�ı�׼��˹�ֲ�����Ԫ��
	TDEFAULT = 5
};
//�������ݴ洢�ͼ���
class Blob
{
public:
	//batch_size(0), channel(0), height(0), width(0) ����ʼ��Ϊ0
	Blob() : batch_size(0), channel(0), height(0), width(0)
	{};
	Blob(const int batch_size, const int channel, const int height, const int width, int type = TDEFAULT);
	Blob(const vector<int> shape_, int type = TDEFAULT);  //���غ���

	void print(string str = "");
	vector<cube>& get_data();
	cube& operator[] (int i);
	Blob& operator= (double val);
	Blob& operator *= (const double k);
	friend Blob operator*(Blob& A, Blob& B);  //����Ϊ��Ԫ����
	friend Blob operator*(double num, Blob& B);  //����Ϊ��Ԫ����
	friend Blob operator+(Blob& A, Blob& B);  //����Ϊ��Ԫ����
	Blob subBlob(int low_idx, int high_idx);
	Blob pad(int pad, double val = 0.0);
	Blob deletePad(int pad);
	void maxIn(double val = 0.0);
	vector<int> size() const;

	inline int get_batch_size() const
	{
		return batch_size;
	}
	inline int get_channel() const
	{
		return channel;
	}
	inline int get_height() const
	{
		return height;
	}
	inline int get_width() const
	{
		return width;
	}

private:
	void _init(const int batch_size, const int channel, const int height, const int width, int type);
private:
	int batch_size; //cube������Ҳ��������
	int channel; // ͨ����
	int height; // ��
	int width; // ��
	vector<cube> blob_data; // ���cube
};
#endif  //__BLOB_HPP__