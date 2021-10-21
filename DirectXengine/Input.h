#pragma once

#include <dinput.h>
#include <Windows.h>
#include <wrl.h>

#define DIRECTINPUT_VERSION     0x0800//DirectInputのバージョン指定

using namespace Microsoft::WRL;

//入力クラス
class Input
{
public://関数
	void Initialize(HINSTANCE hInstance, HWND hwnd);//初期化
	void Update();//更新処理
	bool IsKey(BYTE KeyNumber);//キー判定
	bool IsKeyTrigger(BYTE KeyNumber);//キー判定（長押し不可）

private://変数
	ComPtr<IDirectInput8> dinput;
	ComPtr<IDirectInputDevice8> devkeyboard;
	BYTE Key[256] = {};
	BYTE OldKey[256] = {};

};