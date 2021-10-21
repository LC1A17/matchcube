#include "Input.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

//初期化
void Input::Initialize(HINSTANCE hInstance, HWND hwnd)
{
	HRESULT result = S_FALSE;

	result = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&dinput, nullptr);

	//キーボードデバイスの生成	
	result = dinput->CreateDevice(GUID_SysKeyboard, &devkeyboard, NULL);

	//入力データ形式のセット
	result = devkeyboard->SetDataFormat(&c_dfDIKeyboard);//標準形式

	//排他制御レベルのセット
	result = devkeyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
}

//更新処理
void Input::Update()
{
	HRESULT result = S_FALSE;

	result = devkeyboard->Acquire();//キーボード情報の取得開始

	memcpy(OldKey, Key, sizeof(Key));//前フレームのキー入力をコピー

	result = devkeyboard->GetDeviceState(sizeof(Key), Key);//全キーの入力状態を取得する
}

//キー判定
bool Input::IsKey(BYTE KeyNumber)
{
	//指定キーを押していればtrueを返す
	if (Key[KeyNumber])
	{
		return true;
	}

	//そうでなければfalseを返す
	return false;
}

//キー判定（長押し不可）
bool Input::IsKeyTrigger(BYTE KeyNumber)
{
	//前フレームがfalseで指定キーを押していればtrueを返す
	if (!OldKey[KeyNumber] && Key[KeyNumber])
	{
		return true;
	}

	//そうでなければfalseを返す
	return false;
}