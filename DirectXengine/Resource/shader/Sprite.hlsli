cbuffer cbuff0 : register(b0)
{
	float4 color;	//�F�iRGBA�j
	matrix mat;		//3D�ϊ��s��
};

//���_�V�F�[�_�[����s�N�Z���V�F�[�_�[�ւ̂����Ɏg�p����\����
struct VSOutput
{
	float4 svpos : SV_POSITION; //���_���W
	float2 uv  :TEXCOORD;		//UV�l
};