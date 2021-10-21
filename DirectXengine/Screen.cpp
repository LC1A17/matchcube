#include "Screen.h"

using namespace DirectX;

//コンストクラタ
Screen::Screen()
{

}

//デストラクタ
Screen::~Screen()
{
	
}

//初期化処理
void Screen::Initialize(DirectXInitialize* dxini, Input* input, Sound* sound)
{
	this->dxini = dxini;
	this->input = input;
	this->sound = sound;
}

//更新処理
void Screen::Update()
{
	input->Update();
}

//描画処理
void Screen::Draw()
{
	//コマンドリストの取得
	ID3D12GraphicsCommandList* cmdList = dxini->GetCmdList();
}
