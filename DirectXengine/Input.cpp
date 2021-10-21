#include "Input.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

//������
void Input::Initialize(HINSTANCE hInstance, HWND hwnd)
{
	HRESULT result = S_FALSE;

	result = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&dinput, nullptr);

	//�L�[�{�[�h�f�o�C�X�̐���	
	result = dinput->CreateDevice(GUID_SysKeyboard, &devkeyboard, NULL);

	//���̓f�[�^�`���̃Z�b�g
	result = devkeyboard->SetDataFormat(&c_dfDIKeyboard);//�W���`��

	//�r�����䃌�x���̃Z�b�g
	result = devkeyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
}

//�X�V����
void Input::Update()
{
	HRESULT result = S_FALSE;

	result = devkeyboard->Acquire();//�L�[�{�[�h���̎擾�J�n

	memcpy(OldKey, Key, sizeof(Key));//�O�t���[���̃L�[���͂��R�s�[

	result = devkeyboard->GetDeviceState(sizeof(Key), Key);//�S�L�[�̓��͏�Ԃ��擾����
}

//�L�[����
bool Input::IsKey(BYTE KeyNumber)
{
	//�w��L�[�������Ă����true��Ԃ�
	if (Key[KeyNumber])
	{
		return true;
	}

	//�����łȂ����false��Ԃ�
	return false;
}

//�L�[����i�������s�j
bool Input::IsKeyTrigger(BYTE KeyNumber)
{
	//�O�t���[����false�Ŏw��L�[�������Ă����true��Ԃ�
	if (!OldKey[KeyNumber] && Key[KeyNumber])
	{
		return true;
	}

	//�����łȂ����false��Ԃ�
	return false;
}