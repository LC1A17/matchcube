#pragma once

#include "DirectXInitialize.h"
#include "Input.h"
#include "Sound.h"
#include <DirectXMath.h>

//ゲームシーン
class Screen
{
public://関数
	Screen();//コンストクラタ
	~Screen();//デストラクタ
	void Initialize(DirectXInitialize* dxini, Input* input, Sound* sound);//初期化
	void Update();//更新処理
	void Draw();//描画処理

private://変数
	DirectXInitialize* dxini = nullptr;
	Input* input = nullptr;
	Sound* sound = nullptr;

};