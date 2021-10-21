#pragma once

#include <Windows.h>

//Windows
class WindowsInitialize
{
public://変数
	static const wchar_t TitleName[];//ゲームタイトル
	static const int WIN_WIDTH;//ウィンドウの横幅
	static const int WIN_HEIGHT;//ウィンドウの縦幅
	
public://関数
	HWND GetHwnd();//ウィンドウハンドルの取得
	HINSTANCE GetInstance();//ウィンドウクラスの取得
	static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);//ウィンドウプロシージャ
	void CreateWIN();//ウィンドウの生成
	void DeleteWIN();//ウィンドウ破棄
	bool MessageLoop();//メッセージループ

private://変数
	WNDCLASSEX win{};//ウィンドウクラス
	HWND hwnd = nullptr;//ウィンドウハンドル

};