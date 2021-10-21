#pragma once

#include <dinput.h>
#include <Windows.h>
#include <wrl.h>

#define DIRECTINPUT_VERSION     0x0800//DirectInput�̃o�[�W�����w��

using namespace Microsoft::WRL;

//���̓N���X
class Input
{
public://�֐�
	void Initialize(HINSTANCE hInstance, HWND hwnd);//������
	void Update();//�X�V����
	bool IsKey(BYTE KeyNumber);//�L�[����
	bool IsKeyTrigger(BYTE KeyNumber);//�L�[����i�������s�j

private://�ϐ�
	ComPtr<IDirectInput8> dinput;
	ComPtr<IDirectInputDevice8> devkeyboard;
	BYTE Key[256] = {};
	BYTE OldKey[256] = {};

};