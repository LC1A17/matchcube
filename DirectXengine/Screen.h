#pragma once

#include "DirectXInitialize.h"
#include "Input.h"
#include "Sound.h"
#include <DirectXMath.h>

//�Q�[���V�[��
class Screen
{
public://�֐�
	Screen();//�R���X�g�N���^
	~Screen();//�f�X�g���N�^
	void Initialize(DirectXInitialize* dxini, Input* input, Sound* sound);//������
	void Update();//�X�V����
	void Draw();//�`�揈��

private://�ϐ�
	DirectXInitialize* dxini = nullptr;
	Input* input = nullptr;
	Sound* sound = nullptr;

};