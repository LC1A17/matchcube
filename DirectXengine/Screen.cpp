#include "Screen.h"

using namespace DirectX;

//�R���X�g�N���^
Screen::Screen()
{

}

//�f�X�g���N�^
Screen::~Screen()
{
	
}

//����������
void Screen::Initialize(DirectXInitialize* dxini, Input* input, Sound* sound)
{
	this->dxini = dxini;
	this->input = input;
	this->sound = sound;
}

//�X�V����
void Screen::Update()
{
	input->Update();
}

//�`�揈��
void Screen::Draw()
{
	//�R�}���h���X�g�̎擾
	ID3D12GraphicsCommandList* cmdList = dxini->GetCmdList();
}
