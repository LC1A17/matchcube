#include "WindowsInitialize.h"
#include "DirectXInitialize.h"
#include "Screen.h"
#include "Sound.h"

#include <DirectXTex.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std;

//パイプラインセット
struct PipelineSet
{
	ComPtr<ID3D12PipelineState> pipelinestate;//パイプラインステートオブジェクト
	ComPtr<ID3D12RootSignature> rootsignature;//ルートシグネチャ
};

//スプライト1枚分のデータ
struct Sprite
{
	ComPtr<ID3D12Resource> vertBuff;//頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW vbView;//頂点バッファビュー
	ComPtr<ID3D12Resource> constBuff;//定数バッファ

	float rotation = 0.0f;//Z軸回りの回転角
	XMFLOAT3 position = { 0, 0, 0 };//座標
	XMMATRIX matWorld;//ワールド行列
	XMFLOAT4 color = { 1, 1, 1, 1 };//色
	UINT texNumber = 0;//テクスチャ番号
	XMFLOAT2 size = { 100, 100 };//大きさ
	XMFLOAT2 anchorpoint = { 0.5f, 0.5f };//アンカーポイント
	bool isFilpX = false;//左右反転
	bool isFilpY = false;//上下反転
	XMFLOAT2 texLeftTop = { 0, 0 };//テクスチャの左上座標
	XMFLOAT2 texSize = { 100, 100 };//テクスチャの切り出しサイズ
	bool isInvisible = false;//非表示
};

//オブジェクトのデータ
struct Object
{
	ComPtr<ID3D12Resource> vertBuff;//頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW vbView;//頂点バッファビュー
	ComPtr<ID3D12Resource> indexBuff;//インデックスバッファ
	D3D12_INDEX_BUFFER_VIEW ibView;//インデックスバッファビュー
	ComPtr<ID3D12Resource> constBuff;//定数バッファ
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescHandleCBV;//定数バッファビューのハンドル(CPU)
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescHandleCBV;//定数バッファビューのハンドル(GPU)

	//アフィン変換情報
	XMFLOAT3 scale{ 1,1,1 };//スケーリング倍列

	//Z軸回りの回転角
	XMFLOAT3 rotation = { 0, 0, 0 };
	XMFLOAT3 position{ 0, 0, 0 };//座標

	XMMATRIX matWorld;//ワールド変換行列

	XMFLOAT4 color = { 1, 1, 1, 1 };//色

	XMFLOAT3 size = { 10,10,10 };

	UINT texNumber = 0;

	XMMATRIX matView;

	XMFLOAT3 eye{ 0, 0, -100 };
	XMFLOAT3 target{ 0, 0, 0 };
	XMFLOAT3 up{ 0, 1, 0 };
	int number = 0;
};

//頂点データ構造体
struct Vertex
{
	XMFLOAT3 pos;	//XYZ座標
	XMFLOAT3 normal;//法線ベクトル
	XMFLOAT2 uv;	//UV座標
};

//定数バッファ用データ構造体
struct ConstBufferData
{
	XMFLOAT4 color;	//色
	XMMATRIX mat;	//行列
};

//スプライトの頂点データ型
struct VertexPosUv
{
	XMFLOAT3 pos;	//XYZ座標
	XMFLOAT2 uv;	//UV座標
};

const int spriteSRVCount = 512;//テクスチャの最大枚数
const int objectCount = 512;//オブジェクトの最大個数

//スプライトの共通データ
struct SpriteCommon
{
	PipelineSet pipelineSet;//パイプラインセット
	XMMATRIX matProjection;//射影行列
	ComPtr<ID3D12DescriptorHeap> descHeap;//テクスチャ用デスクリプタヒープの生成
	ComPtr<ID3D12Resource> texBuff[spriteSRVCount];//テクスチャリソース（テクスチャバッファ）の配列
};

//オブジェクトの共通データ
struct ObjectCommon
{
	PipelineSet pipelineSet;//パイプラインセット
	XMMATRIX matProjection;//射影行列
	ComPtr<ID3D12DescriptorHeap> descHeap;//テクスチャ用デスクリプタヒープの生成
	ComPtr<ID3D12Resource> texBuff[objectCount];//テクスチャリソース（テクスチャバッファ）の配列
};

PipelineSet Object3dCreateGraphicsPipeline(ID3D12Device* dev);//3Dオブジェクト用パイプライン生成
PipelineSet SpriteCreateGraphicsPipeline(ID3D12Device* dev);//スプライト用パイプライン生成
Sprite SpriteCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT, UINT texNumber, const SpriteCommon& spriteCommon, XMFLOAT2 anchorpoint, bool isFilpX, bool isFilpY);//スプライト作成
void SpriteCommonBeginDraw(const SpriteCommon& spriteCommon, ID3D12GraphicsCommandList* cmdList);//スプライト共通グラフィックコマンドのセット
void SpriteDraw(const Sprite& sprite, ID3D12GraphicsCommandList* cmdList, const SpriteCommon& spriteCommon, ID3D12Device* dev);//スプライト単体表示
SpriteCommon SpriteCommonCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT);//スプライト共通データ生成
void SpriteUpdate(Sprite& sprite, const SpriteCommon& spriteCommon);//スプライト単体更新
void SpriteCommonLoadTexture(SpriteCommon& spriteCommon, UINT texnumber, const wchar_t* filename, ID3D12Device* dev);//スプライト共通テクスチャ読み込み
void SpriteTransferVertexBuffer(const Sprite& sprite, const SpriteCommon& spriteCommon);//スプライト単体頂点バッファの転送
Object objectCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT, UINT texNumber);//オブジェクト作成
Object objectCreateFlont(ID3D12Device* dev, int window_width, int window_height, UINT texNumber);
Object objectCreateBack(ID3D12Device* dev, int window_width, int window_height, UINT texNumber);
Object objectCreateLeft(ID3D12Device* dev, int window_width, int window_height, UINT texNumber);
Object objectCreateRight(ID3D12Device* dev, int window_width, int window_height, UINT texNumber);
Object objectCreateUp(ID3D12Device* dev, int window_width, int window_height, UINT texNumber);
Object objectCreateDown(ID3D12Device* dev, int window_width, int window_height, UINT texNumber);
void ObjectCommonBeginDraw(const ObjectCommon& objectCommon, ID3D12GraphicsCommandList* cmdList);//オブジェクトグラフィックコマンドのセット
void ObjectDraw(const Object& object, ID3D12GraphicsCommandList* cmdList, const ObjectCommon& objectCommon, ID3D12Device* dev);//オブジェクト表示
ObjectCommon ObjectCommonCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT);//オブジェクトデータ生成
void ObjectUpdate(Object& object, const ObjectCommon& objectCommon);//オブジェクト更新
void ObjectCommonLoadTexture(ObjectCommon& objectCommon, UINT texNumber, const wchar_t* filename, ID3D12Device* dev);//オブジェクトテクスチャ読み込み
void ObjectTransferVertexBuffer(const Object& object, const ObjectCommon& ObjectCommon);//オブジェクト頂点バッファの転送

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	//各種初期化、生成
	WindowsInitialize* WinIni = nullptr;
	WinIni = new WindowsInitialize();
	WinIni->CreateWIN();

	DirectXInitialize* DxIni = nullptr;
	DxIni = new DirectXInitialize();
	DxIni->Initialize(WinIni);

	Input* input = nullptr;
	input = new Input();
	input->Initialize(WinIni->GetInstance(), WinIni->GetHwnd());

	Sound* sound = nullptr;
	sound = new Sound();
	sound->Initialize();

	Screen* screen = nullptr;
	screen = new Screen();
	screen->Initialize(DxIni, input, sound);

	//ここから
	Sound::SoundData soundData1 = sound->SoundLoadWave("Resource/sound/BGM.wav");
	Sound::SoundData soundData2 = sound->SoundLoadWave("Resource/sound/Clear.wav");
	Sound::SoundData soundData3 = sound->SoundLoadWave("Resource/sound/Space.wav");
	Sound::SoundData soundData5 = sound->SoundLoadWave("Resource/sound/Cursor.wav");

	PipelineSet object3dPipelineSet = Object3dCreateGraphicsPipeline(DxIni->GetDev());
	PipelineSet spritePipelineSet = SpriteCreateGraphicsPipeline(DxIni->GetDev());

	//オブジェクト
	ObjectCommon objectCommon;
	objectCommon = ObjectCommonCreate(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT);

	//テクスチャ読み込み
	ObjectCommonLoadTexture(objectCommon, 0, L"Resource/image/Floor.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 1, L"Resource/image/Floor.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 2, L"Resource/image/Stage1.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 3, L"Resource/image/Stage2.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 4, L"Resource/image/Stage3.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 5, L"Resource/image/Stage4.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 6, L"Resource/image/Stage5.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 7, L"Resource/image/Stage6.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 8, L"Resource/image/Stage7.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 9, L"Resource/image/Stage8.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 10, L"Resource/image/Stage9.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 11, L"Resource/image/Red.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 12, L"Resource/image/Green.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 13, L"Resource/image/Blue.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 14, L"Resource/image/Clear1.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 15, L"Resource/image/Clear2.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 16, L"Resource/image/Clear3.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 17, L"Resource/image/Clear4.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 18, L"Resource/image/Clear5.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 19, L"Resource/image/Clear6.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 20, L"Resource/image/Clear7.png", DxIni->GetDev());
	ObjectCommonLoadTexture(objectCommon, 21, L"Resource/image/Clear8.png", DxIni->GetDev());

	const int o_count = 93;
	Object object[o_count];

	for (int i = 0; i < o_count; i++)
	{
		//オブジェクトの番号ごとに割り当て
		if (i == 0 || i == 73 || i == 74)
		{
			object[i] = objectCreate(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, 0);
		}

		else if (i >= 1 && i <= 5)
		{
			object[1] = objectCreate(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, 1);
			object[2] = objectCreate(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, 11);
			object[3] = objectCreate(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, 1);
			object[4] = objectCreate(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, 12);
			object[5] = objectCreate(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, 13);
		}

		else if (i > 63 && i < 73)
		{
			object[i] = objectCreate(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, i - 62);
		}

		else if (i == 75 || i == 81 || i == 87)
		{
			object[i] = objectCreateFlont(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, 1);
		}

		else if (i == 76 || i == 82 || i == 88)
		{
			object[i] = objectCreateBack(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, 1);
		}

		else if (i == 77 || i == 83 || i == 89)
		{
			object[i] = objectCreateLeft(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, 1);
		}

		else if (i == 78 || i == 84 || i == 90)
		{
			object[i] = objectCreateRight(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, 1);
		}

		else if (i == 79 || i == 85 || i == 91)
		{
			object[i] = objectCreateUp(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, 1);
		}

		else if (i == 80 || i == 86 || i == 92)
		{
			object[i] = objectCreateDown(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, 1);
		}

		else
		{
			object[i] = objectCreate(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, 1);
		}

		if (i > 80)
		{
			object[i].rotation = { 45, 0, 45 };
		}

		else
		{
			object[i].rotation = { 0, 0, 0 };
		}
	}

	object[0].position = { 0, 0, -10 };
	object[75].position = { -30, 0, -10 };
	object[76].position = { -30, 0, -10 };
	object[77].position = { -30, 0, -10 };
	object[78].position = { -30, 0, -10 };
	object[79].position = { -30, 0, -10 };
	object[80].position = { -30, 0, -10 };

	//以下ステージ
	object[1].position = { -70, 30, 0 };
	object[2].position = { -60, 30, 0 };
	object[3].position = { -50, 30, 0 };
	object[4].position = { -40, 30, 0 };
	object[5].position = { -30, 30, 0 };
	object[6].position = { -20, 30, 0 };
	object[7].position = { -10, 30, 0 };
	object[8].position = { 0, 30, 0 };
	object[9].position = { 10, 30, 0 };
	object[10].position = { -70, 20, 0 };
	object[11].position = { -60, 20, 0 };
	object[12].position = { -50, 20, 0 };
	object[13].position = { -40, 20, 0 };
	object[14].position = { -30, 20, 0 };
	object[15].position = { -20, 20, 0 };
	object[16].position = { -10, 20, 0 };
	object[17].position = { 0, 20, 0 };
	object[18].position = { 10, 20, 0 };
	object[19].position = { -70, 10, 0 };
	object[20].position = { -60, 10, 0 };
	object[21].position = { -50, 10, 0 };
	object[22].position = { -40, 10, 0 };
	object[23].position = { -30, 10, 0 };
	object[24].position = { -20, 10, 0 };
	object[25].position = { -10, 10, 0 };
	object[26].position = { 0, 10, 0 };
	object[27].position = { 10, 10, 0 };
	object[28].position = { -70, 0, 0 };
	object[29].position = { -60, 0, 0 };
	object[30].position = { -50, 0, 0 };
	object[31].position = { -40, 0, 0 };
	object[32].position = { -30, 0, 0 };
	object[33].position = { -20, 0, 0 };
	object[34].position = { -10, 0, 0 };
	object[35].position = { 0, 0, 0 };
	object[36].position = { 10, 0, 0 };
	object[37].position = { -70, -10, 0 };
	object[38].position = { -60, -10, 0 };
	object[39].position = { -50, -10, 0 };
	object[40].position = { -40, -10, 0 };
	object[41].position = { -30, -10, 0 };
	object[42].position = { -20, -10, 0 };
	object[43].position = { -10, -10, 0 };
	object[44].position = { 0, -10, 0 };
	object[45].position = { 10, -10, 0 };
	object[46].position = { -70, -20, 0 };
	object[47].position = { -60, -20, 0 };
	object[48].position = { -50, -20, 0 };
	object[49].position = { -40, -20, 0 };
	object[50].position = { -30, -20, 0 };
	object[51].position = { -20, -20, 0 };
	object[52].position = { -10, -20, 0 };
	object[53].position = { 0, -20, 0 };
	object[54].position = { 10, -20, 0 };
	object[55].position = { -70, -30, 0 };
	object[56].position = { -60, -30, 0 };
	object[57].position = { -50, -30, 0 };
	object[58].position = { -40, -30, 0 };
	object[59].position = { -30, -30, 0 };
	object[60].position = { -20, -30, 0 };
	object[61].position = { -10, -30, 0 };
	object[62].position = { 0, -30, 0 };
	object[63].position = { 10, -30, 0 };
	object[64].position = { -70, 30, 0 };//ステージ1
	object[65].position = { -70, 30, 0 };//ステージ2
	object[66].position = { -70, 30, 0 };//ステージ3
	object[67].position = { -70, 30, 0 };//ステージ4
	object[68].position = { -70, 30, 0 };//ステージ5
	object[69].position = { -70, 30, 0 };//ステージ6
	object[70].position = { -70, 30, 0 };//ステージ7
	object[71].position = { -70, 30, 0 };//ステージ8
	object[72].position = { -70, 30, 0 };//ステージ9

	//消された辻本
	object[73].position = { 80, 30, 0 };
	object[74].position = { 80, -10, 0 };

	//見本と現在形
	for (int i = 81; i < 87; i++)
	{
		object[i].position = { 80,30,0 };
	}

	for (int i = 87; i < 93; i++)
	{
		object[i].position = { 80,-10,0 };
	}

	for (int i = 0; i < o_count; i++)
	{
		object[i].scale = { 0.5f, 0.5f, 0.5f };
	}

	//スプライト
	SpriteCommon spriteCommon;
	spriteCommon = SpriteCommonCreate(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT);

	//テクスチャ読み込み
	SpriteCommonLoadTexture(spriteCommon, 0, L"Resource/image/Title.png", DxIni->GetDev());
	SpriteCommonLoadTexture(spriteCommon, 1, L"Resource/image/Back.jpg", DxIni->GetDev());
	SpriteCommonLoadTexture(spriteCommon, 2, L"Resource/image/Game_Left.png", DxIni->GetDev());
	SpriteCommonLoadTexture(spriteCommon, 3, L"Resource/image/Game_Right.png", DxIni->GetDev());
	SpriteCommonLoadTexture(spriteCommon, 4, L"Resource/image/Game_Up.png", DxIni->GetDev());
	SpriteCommonLoadTexture(spriteCommon, 5, L"Resource/image/Game_Down.png", DxIni->GetDev());
	SpriteCommonLoadTexture(spriteCommon, 6, L"Resource/image/clear_screen.png", DxIni->GetDev());

	//スプライト
	const int s_count = 7;
	Sprite sprite[s_count];

	//スプライトの生成
	for (int i = 0; i < s_count; i++)
	{
		sprite[i] = SpriteCreate(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, sprite[i].texNumber, spriteCommon, sprite[i].anchorpoint, sprite[i].isFilpX, sprite[i].isFilpY);
		sprite[i].texNumber = i;
		sprite[i].rotation = 0;
	}

	sprite[0].position = { 1280 / 2, 720 / 2, 0 };
	sprite[1].position = { 1280 / 2, 720 / 2, 1000 };
	sprite[2].position = { 1280 / 2, 720 / 2, 0 };
	sprite[3].position = { 1280 / 2, 720 / 2, 0 };
	sprite[4].position = { 1280 / 2, 720 / 2, 0 };
	sprite[5].position = { 1280 / 2, 720 / 2, 0 };
	sprite[6].position = { 1280 / 2, 720 / 2, 0 };

	sprite[0].size.x = WindowsInitialize::WIN_WIDTH;
	sprite[0].size.y = WindowsInitialize::WIN_HEIGHT;

	sprite[1].size.x = 10000.0f;
	sprite[1].size.y = 10000.0f;

	sprite[2].size.x = WindowsInitialize::WIN_WIDTH;
	sprite[2].size.y = WindowsInitialize::WIN_HEIGHT;

	sprite[3].size.x = WindowsInitialize::WIN_WIDTH;
	sprite[3].size.y = WindowsInitialize::WIN_HEIGHT;

	sprite[4].size.x = WindowsInitialize::WIN_WIDTH;
	sprite[4].size.y = WindowsInitialize::WIN_HEIGHT;

	sprite[5].size.x = WindowsInitialize::WIN_WIDTH;
	sprite[5].size.y = WindowsInitialize::WIN_HEIGHT;

	sprite[6].size.x = WindowsInitialize::WIN_WIDTH;
	sprite[6].size.y = WindowsInitialize::WIN_HEIGHT;

	sprite[0].texSize = { 1024, 512 };
	sprite[1].texSize = { 812, 542 };
	sprite[2].texSize = { 1280, 720 };
	sprite[3].texSize = { 1280, 720 };
	sprite[4].texSize = { 1280, 720 };
	sprite[5].texSize = { 1280, 720 };
	sprite[6].texSize = { 1280, 720 };

	//頂点バッファに反映
	for (int i = 0; i < s_count; i++)
	{
		SpriteTransferVertexBuffer(sprite[i], spriteCommon);
	}

	//ゲームシーン
	enum Scene
	{
		Title, Game, End,
	};

	//移動方向
	enum Direction
	{
		Left, Right, Up, Down,
	};

	//色
	enum Color
	{
		Red, Green, Blue,
	};

	int SceneNum = Title;//ゲームシーン
	bool isChange = false;//ステージ変更処理
	bool isLoad = false;//ロード
	int LoadCount = 20;//ロードのウェイト
	int StageNum = 0;//0はステージセレクト。1～MaxStage
	int MaxStage = 10;//最大ステージ数
	int MoveDirection = Right;//進行方向
	bool isLeft = false;//進行方向
	bool isRight = false;//進行方向
	bool isUp = false;//進行方向
	bool isDown = false;//進行方向
	bool isRota = false;//回転しているか
	int timer = 0;//回転、移動処理の時間
	int rotaX = 0;//今のXの向き正なら右に負なら左に回転している//4で0になる(処理用)
	int rotaY = 0;//今のYの向き正なら下に負なら上に回転している//4で0になる(処理用)
	int rotaXCount = 0;//今のXの向き正なら右に負なら左に回転している
	int rotaYCount = 0;//今のYの向き正なら下に負なら上に回転している
	int maxRotaX = 5;
	int maxRotaY = 5;
	int minRotaX = -5;
	int minRotaY = -5;
	bool movePlus = false;
	bool moveMin = false;
	int MaxBlock = 73;//ステージのブロックの数
	bool isHit = false;
	bool isSound[5] = { false, false, false, false, false };
	int SoundCount[5] = { 0, 0, 0, 0, 0 };

	sound->SoundPlayWave(soundData1);
	isSound[0] = true;

	//ゲームループ
	while (true)
	{
		//メッセージループ
		if (WinIni->MessageLoop())
		{
			break;
		}

		//更新処理ここから
		screen->Update();//更新処理

		if (isSound[0] == false)
		{
			sound->SoundPlayWave(soundData1);
			isSound[0] = true;
		}

		if (isSound[0] == true)
		{
			SoundCount[0]++;

			if (SoundCount[0] == 8450)
			{
				isSound[0] = false;
				SoundCount[0] = 0;
			}
		}

		if (isSound[1] == true)
		{
			SoundCount[1]++;

			if (SoundCount[1] == 20)
			{
				isSound[1] = false;
				SoundCount[1] = 0;
			}
		}
			
		if (isSound[2] == true)
		{
			SoundCount[2]++;

			if (SoundCount[2] == 20)
			{
				isSound[2] = false;
				SoundCount[2] = 0;
			}
		}

		if (isSound[3] == true)
		{
			SoundCount[3]++;

			if (SoundCount[3] == 10)
			{
				isSound[3] = false;
				SoundCount[3] = 0;
			}
		}

		if (isSound[4] == true)
		{
			SoundCount[4]++;

			if (SoundCount[4] == 5)
			{
				isSound[4] = false;
				SoundCount[4] = 0;
			}
		}

		//1:タイトル
		if (SceneNum == Title || SceneNum == End)
		{
			//SPACEを押したら20フレーム後にステージセレクトに移行
			if (input->IsKeyTrigger(DIK_SPACE))
			{
				if (isSound[2] == false)
				{
					sound->SoundPlayWave(soundData3);
					isSound[2] = true;
				}

				isLoad = true;
			}

			if (isLoad == true && LoadCount > 0)
			{
				LoadCount--;
			}

			if (LoadCount == 0)
			{
				SceneNum = Game;
				isLoad = false;
				isChange = true;
				LoadCount = 20;
			}
		}

		//2:ゲーム画面
		else if (SceneNum == Game)
		{
			if (StageNum == 1)
			{
				object[81].color = { 1,1,1,1 };
				object[82].color = { 0,0,1,1 };//赤
				object[83].color = { 0,1,0,1 };//緑
				object[84].color = { 1,0,0,1 };//青
				object[85].color = { 1,1,1,1 };
				object[86].color = { 1,1,1,1 };
			}

			else if (StageNum == 2)
			{
				object[81].color = { 1,1,1,1 };
				object[82].color = { 0,0,1,1 };//青
				object[83].color = { 1,0,0,1 };//赤
				object[84].color = { 1,1,1,1 };
				object[85].color = { 0,1,0,1 };//緑
				object[86].color = { 1,1,1,1 };
			}

			else if (StageNum == 3)
			{
				object[81].color = { 0,1,0,1 };
				object[82].color = { 0,1,0,1 };//青
				object[83].color = { 1,0,0,1 };//赤
				object[84].color = { 1,0,0,1 };//赤
				object[85].color = { 1,1,1,1 };//緑
				object[86].color = { 0,0,1,1 };
			}

			else if (StageNum == 4)
			{
				object[81].color = { 0,0,1,1 };
				object[82].color = { 0,0,1,1 };//青
				object[83].color = { 0,1,0,1 };//赤
				object[84].color = { 0,1,0,1 };//赤
				object[85].color = { 1,0,0,1 };//緑
				object[86].color = { 1,0,0,1 };
			}

			else if (StageNum == 5)
			{
				object[81].color = { 1,0,0,1 };
				object[82].color = { 1,0,0,1 };//青
				object[83].color = { 0,1,0,1 };//赤
				object[84].color = { 0,1,0,1 };//赤
				object[85].color = { 0,0,1,1 };//緑
				object[86].color = { 0,0,1,1 };
			}

			else if (StageNum == 6)
			{
				object[81].color = { 0,0,1,1 };
				object[82].color = { 0,0,1,1 };//青
				object[83].color = { 1,0,0,1 };//赤
				object[84].color = { 1,0,0,1 };//赤
				object[85].color = { 0,1,0,1 };//緑
				object[86].color = { 0,1,0,1 };
			}

			else if (StageNum == 7)
			{
				object[81].color = { 1,0,0,1 };
				object[82].color = { 0,0,1,1 };//青
				object[83].color = { 1,1,1,1 };//赤
				object[84].color = { 0,1,0,1 };//赤
				object[85].color = { 0,1,0,1 };//緑
				object[86].color = { 0,0,1,1 };
			}

			else if (StageNum == 8)
			{
				object[81].color = { 0,0,1,1 };
				object[82].color = { 0,1,0,1 };//青
				object[83].color = { 1,0,0,1 };//赤
				object[84].color = { 0,0,1,1 };//赤
				object[85].color = { 1,0,0,1 };//緑
				object[86].color = { 0,1,0,1 };
			}

			//背景の更新
			sprite[1].position = { sprite[0].position.x, sprite[0].position.y, 0 };

			if (StageNum > 0)
			{
				//見本とコピーの回転処理
				for (int i = 81; i < 87; i++)
				{
					object[i].rotation.y += 0.5;
				}

				for (int i = 87; i < 93; i++)
				{
					object[i].rotation.y += 0.5;
				}
			}

			//SPACEを押したら回転、移動処理開始
			if (input->IsKeyTrigger(DIK_SPACE) && isRota == false && timer == 0)
			{
				if (isSound[2] == false)
				{
					sound->SoundPlayWave(soundData3);
					isSound[2] = true;
				}

				if (MoveDirection == Left)
				{
					isLeft = true;
				}

				else if (MoveDirection == Right)
				{
					isRight = true;
				}

				else if (MoveDirection == Up)
				{
					isUp = true;
				}

				else
				{
					isDown = true;
				}

				isRota = true;
			}

			//十字キーで移動方向を指定
			if (input->IsKeyTrigger(DIK_LEFT))
			{
				if (isSound[4] == false)
				{
					sound->SoundPlayWave(soundData5);
					isSound[4] = true;
				}

				MoveDirection = Left;
			}

			if (input->IsKeyTrigger(DIK_RIGHT))
			{
				if (isSound[4] == false)
				{
					sound->SoundPlayWave(soundData5);
					isSound[4] = true;
				}

				MoveDirection = Right;
			}

			if (input->IsKeyTrigger(DIK_UP))
			{
				if (isSound[4] == false)
				{
					sound->SoundPlayWave(soundData5);
					isSound[4] = true;
				}

				MoveDirection = Up;
			}

			if (input->IsKeyTrigger(DIK_DOWN))
			{
				if (isSound[4] == false)
				{
					sound->SoundPlayWave(soundData5);
					isSound[4] = true;
				}

				MoveDirection = Down;
			}

			//回転、移動処理
			//左
			if (isLeft == true && minRotaX < rotaXCount
				&& abs(object[75].rotation.x) != 90
				&& abs(object[75].rotation.x) != 270)
			{
				object[75].position.x -= 0.25f;
				object[76].position.x -= 0.25f;
				object[77].position.x -= 0.25f;
				object[78].position.x -= 0.25f;
				object[79].position.x -= 0.25f;
				object[80].position.x -= 0.25f;
				object[75].rotation.y += 2.25f;
				object[76].rotation.y += 2.25f;
				object[77].rotation.y += 2.25f;
				object[78].rotation.y += 2.25f;
				object[79].rotation.y += 2.25f;
				object[80].rotation.y += 2.25f;
				timer++;
			}

			else if (isLeft == true
				&& minRotaX >= rotaXCount
				|| abs(object[75].rotation.x) == 90
				|| abs(object[75].rotation.x) == 270)
			{
				isLeft = false;
				isRota = false;
			}

			//右
			if (isRight == true && maxRotaX > rotaXCount && abs(object[75].rotation.x) != 90 && abs(object[75].rotation.x) != 270)
			{
				object[75].position.x += 0.25f;
				object[76].position.x += 0.25f;
				object[77].position.x += 0.25f;
				object[78].position.x += 0.25f;
				object[79].position.x += 0.25f;
				object[80].position.x += 0.25f;
				object[75].rotation.y -= 2.25f;
				object[76].rotation.y -= 2.25f;
				object[77].rotation.y -= 2.25f;
				object[78].rotation.y -= 2.25f;
				object[79].rotation.y -= 2.25f;
				object[80].rotation.y -= 2.25f;
				timer++;
			}

			else if (isRight == true && maxRotaX <= rotaXCount || abs(object[75].rotation.x) == 90 || abs(object[75].rotation.x) == 270)
			{
				isRight = false;
				isRota = false;
			}

			//上
			if (isUp == true)
			{
				if (abs(object[75].rotation.y) == 0.0f && maxRotaY > rotaYCount)
				{
					object[75].rotation.x += 2.25f;
					object[76].rotation.x += 2.25f;
					object[77].rotation.x += 2.25f;
					object[78].rotation.x += 2.25f;
					object[79].rotation.x += 2.25f;
					object[80].rotation.x += 2.25f;
					object[75].position.y += 0.25f;
					object[76].position.y += 0.25f;
					object[77].position.y += 0.25f;
					object[78].position.y += 0.25f;
					object[79].position.y += 0.25f;
					object[80].position.y += 0.25f;
					timer++;
				}

				else if (abs(object[75].rotation.y) == 180.0f && maxRotaY > rotaYCount)
				{
					object[75].rotation.x -= 2.25f;
					object[76].rotation.x -= 2.25f;
					object[77].rotation.x -= 2.25f;
					object[78].rotation.x -= 2.25f;
					object[79].rotation.x -= 2.25f;
					object[80].rotation.x -= 2.25f;
					object[75].position.y += 0.25f;
					object[76].position.y += 0.25f;
					object[77].position.y += 0.25f;
					object[78].position.y += 0.25f;
					object[79].position.y += 0.25f;
					object[80].position.y += 0.25f;
					timer++;
				}

				else
				{
					isUp = false;
					isRota = false;
				}
			}

			//下
			if (isDown == true)
			{
				//Y軸の角度によって回転が異なるabsは絶対値
				if (abs(object[75].rotation.y) == 0.0f && minRotaY < rotaYCount)
				{
					object[75].rotation.x -= 2.25f;
					object[76].rotation.x -= 2.25f;
					object[77].rotation.x -= 2.25f;
					object[78].rotation.x -= 2.25f;
					object[79].rotation.x -= 2.25f;
					object[80].rotation.x -= 2.25f;
					object[75].position.y -= 0.25f;
					object[76].position.y -= 0.25f;
					object[77].position.y -= 0.25f;
					object[78].position.y -= 0.25f;
					object[79].position.y -= 0.25f;
					object[80].position.y -= 0.25f;
					timer++;
				}

				else if (abs(object[75].rotation.y) == 180.0f && minRotaY < rotaYCount)
				{
					object[75].rotation.x += 2.25f;
					object[76].rotation.x += 2.25f;
					object[77].rotation.x += 2.25f;
					object[78].rotation.x += 2.25f;
					object[79].rotation.x += 2.25f;
					object[80].rotation.x += 2.25f;
					object[75].position.y -= 0.25f;
					object[76].position.y -= 0.25f;
					object[77].position.y -= 0.25f;
					object[78].position.y -= 0.25f;
					object[79].position.y -= 0.25f;
					object[80].position.y -= 0.25f;
					timer++;
				}

				else
				{
					isDown = false;
					isRota = false;
				}
			}

			//timerが40になったら回転、移動処理終了
			if (timer > 39)
			{
				if (isLeft == true)
				{
					rotaX -= 1;
					rotaXCount -= 1;
				}
				if (isRight == true)
				{
					rotaX += 1;
					rotaXCount += 1;
				}
				if (isUp == true)
				{
					rotaY += 1;
					rotaYCount += 1;
				}
				if (isDown == true)
				{
					rotaY -= 1;
					rotaYCount -= 1;
				}

				isLeft = false;
				isRight = false;
				isUp = false;
				isDown = false;
				isRota = false;
				isHit = true;
				timer = 0;
			}

			if (abs(rotaX) == 4)
			{
				rotaX = 0;
			}

			if (abs(rotaY) == 4)
			{
				rotaY = 0;
			}

			if (abs(object[75].rotation.y) == 360.0f)
			{
				object[75].rotation.y = 0;
			}

			if (abs(object[75].rotation.x) == 360.0f)
			{
				object[75].rotation.x = 0;
			}

		}

		//ステージ変更処理
		if (isChange == true)
		{
			if (StageNum == 0)
			{
				object[1].position = { -70, 0, 0 };
				object[2].position = { -60, 0, 0 };
				object[3].position = { -50, 0, 0 };
				object[4].position = { -40, 0, 0 };
				object[5].position = { -30, 0, 0 };
				object[6].position = { -20, 0, 0 };
				object[7].position = { -10, 0, 0 };

				object[1].texNumber = 1;
				object[2].texNumber = 1;
				object[3].texNumber = 1;
				object[4].texNumber = 1;
				object[5].texNumber = 1;
				object[6].texNumber = 1;
				object[7].texNumber = 1;
				object[64].position = { -70, 10, 0 };//ステージ1
				object[65].position = { -70, -10, 0 };//ステージ2
				object[66].position = { -50, 10, 0 };//ステージ3
				object[67].position = { -50, -10, 0 };//ステージ4
				object[68].position = { -30, 10, 0 };//ステージ5
				object[69].position = { -30, -10, 0 };//ステージ6
				object[70].position = { -10, 10, 0 };//ステージ7
				object[71].position = { -10, -10, 0 };//ステージ8
				object[72].position = { 1000, 1000, 0 };//ステージ9

				//いらないブロックには認識の外側に消えていただく
				for (int i = 8; i < 64; i++)
				{
					object[i].position = { 1000, 1000, 1000 };
				}

				//プレイヤー
				for (int i = 75; i < 81; i++)
				{
					object[i].color = { 1,1,1,1 };
					rotaX = 0;
					rotaY = 0;
					rotaXCount = 0;
					rotaYCount = 0;
					object[i].rotation = { 0, 0, 0 };
					object[i].position.x = object[1].position.x;
					object[i].position.y = object[1].position.y;
				}

				for (int i = 87; i < 93; i++)
				{
					object[i].color = { 1,1,1,1 };
				}

				maxRotaX = 6;
				maxRotaY = 1;
				minRotaX = 0;
				minRotaY = -1;
				MaxBlock = 73;//ステージの最大ブロック数を指定
				isChange = false;
			}

			else if (StageNum == 1)
			{
				object[1].position = { -50, 0, 0 };
				object[2].position = { -40, 0, 0 };
				object[3].position = { -30, 0, 0 };
				object[4].position = { -20, 0, 0 };
				object[5].position = { -10, 0, 0 };

				object[1].texNumber = 1;
				object[2].texNumber = 11;
				object[3].texNumber = 1;
				object[4].texNumber = 12;
				object[5].texNumber = 13;

				//いらないブロックには認識の外側に消えていただく
				for (int i = 6; i < 73; i++)
				{
					object[i].position = { 1000, 1000, 1000 };
				}

				//プレイヤー
				for (int i = 75; i < 81; i++)
				{
					object[i].color = { 1,1,1,1 };
					rotaX = 0;
					rotaY = 0;
					rotaXCount = 0;
					rotaYCount = 0;
					object[i].rotation = { 0, 0, 0 };
					object[i].position.x = object[1].position.x;
					object[i].position.y = object[1].position.y;
				}

				for (int i = 87; i < 93; i++)
				{
					object[i].color = { 1,1,1,1 };
				}

				maxRotaX = 4;
				maxRotaY = 0;
				minRotaX = 0;
				minRotaY = 0;
				MaxBlock = 6;//ステージの最大ブロック数を指定
				isChange = false;
			}
			else if (StageNum == 2)
			{
				object[1].position = { -50, -20, 0 };
				object[2].position = { -40, -20, 0 };
				object[3].position = { -30, -20, 0 };
				object[4].position = { -20, -20, 0 };
				object[5].position = { -10, -20, 0 };
				object[6].position = { -50, -10, 0 };
				object[7].position = { -30, -10, 0 };
				object[8].position = { -10, -10, 0 };
				object[9].position = { -50, 0, 0 };
				object[10].position = { -40, 0, 0 };
				object[11].position = { -30, 0, 0 };
				object[12].position = { -20, 0, 0 };
				object[13].position = { -10, 0, 0 };

				object[1].texNumber = 1;
				object[2].texNumber = 11;
				object[3].texNumber = 1;
				object[4].texNumber = 1;
				object[5].texNumber = 1;
				object[6].texNumber = 1;
				object[7].texNumber = 12;
				object[8].texNumber = 1;
				object[9].texNumber = 1;
				object[10].texNumber = 1;
				object[11].texNumber = 1;
				object[12].texNumber = 1;
				object[13].texNumber = 13;

				//いらないブロックには認識の外側に消えていただく
				for (int i = 14; i < 73; i++)
				{
					object[i].position = { 1000, 1000, 1000 };
				}

				//プレイヤー
				for (int i = 75; i < 81; i++)
				{
					object[i].color = { 1,1,1,1 };
					rotaX = 0;
					rotaY = 0;
					rotaXCount = 0;
					rotaYCount = 0;
					object[i].rotation = { 0, 0, 0 };
					object[i].position.x = object[3].position.x;
					object[i].position.y = object[3].position.y;
				}

				for (int i = 87; i < 93; i++)
				{
					object[i].color = { 1,1,1,1 };
				}

				maxRotaX = 2;
				maxRotaY = 2;
				minRotaX = -2;
				minRotaY = 0;
				MaxBlock = 14;//ステージの最大ブロック数を指定
				isChange = false;
			}

			else if (StageNum == 3)
			{
				object[1].position = { -70, 0, 0 };
				object[2].position = { -60, 0, 0 };
				object[3].position = { -50, 0, 0 };
				object[4].position = { -40, 0, 0 };
				object[5].position = { -30, 0, 0 };
				object[6].position = { -20, 0, 0 };
				object[7].position = { -10, 0, 0 };
				object[8].position = { -70, -10, 0 };
				object[9].position = { -50, -10, 0 };
				object[10].position = { -30, -10, 0 };
				object[11].position = { -10, -10, 0 };
				object[12].position = { -70, -20, 0 };
				object[13].position = { -60, -20, 0 };
				object[14].position = { -50, -20, 0 };
				object[15].position = { -40, -20, 0 };
				object[16].position = { -30, -20, 0 };
				object[17].position = { -20, -20, 0 };
				object[18].position = { -10, -20, 0 };

				object[1].texNumber = 1;
				object[2].texNumber = 11;
				object[3].texNumber = 1;
				object[4].texNumber = 1;
				object[5].texNumber = 1;
				object[6].texNumber = 1;
				object[7].texNumber = 13;
				object[8].texNumber = 1;
				object[9].texNumber = 1;
				object[10].texNumber = 13;
				object[11].texNumber = 1;
				object[12].texNumber = 12;
				object[13].texNumber = 1;
				object[14].texNumber = 1;
				object[15].texNumber = 11;
				object[16].texNumber = 1;
				object[17].texNumber = 1;
				object[18].texNumber = 12;

				//いらないブロックには認識の外側に消えていただく
				for (int i = 19; i < 73; i++)
				{
					object[i].position = { 1000, 1000, 1000 };
				}

				//プレイヤー
				for (int i = 75; i < 81; i++)
				{
					object[i].color = { 1,1,1,1 };
					rotaX = 0;
					rotaY = 0;
					rotaXCount = 0;
					rotaYCount = 0;
					object[i].rotation = { 0, 0, 0 };
					object[i].position.x = object[3].position.x;
					object[i].position.y = object[3].position.y;
				}

				for (int i = 87; i < 93; i++)
				{
					object[i].color = { 1,1,1,1 };
				}

				maxRotaX = 4;
				maxRotaY = 0;
				minRotaX = -2;
				minRotaY = -2;
				MaxBlock = 19;//ステージの最大ブロック数を指定
				isChange = false;
			}

			else if (StageNum == 4)
			{
				object[1].position = { -50, 20, 0 };
				object[2].position = { -40, 20, 0 };
				object[3].position = { -30, 20, 0 };
				object[4].position = { -20, 20, 0 };
				object[5].position = { -10, 20, 0 };
				object[6].position = { -50, 10, 0 };
				object[7].position = { -30, 10, 0 };
				object[8].position = { -10, 10, 0 };
				object[9].position = { -50, 0, 0 };
				object[10].position = { -40, 0, 0 };
				object[11].position = { -30, 0, 0 };
				object[12].position = { -20, 0, 0 };
				object[13].position = { -10, 0, 0 };
				object[14].position = { -50, -10, 0 };
				object[15].position = { -30, -10, 0 };
				object[16].position = { -10, -10, 0 };
				object[17].position = { -50, -20, 0 };
				object[18].position = { -40, -20, 0 };
				object[19].position = { -30, -20, 0 };
				object[20].position = { -20, -20, 0 };
				object[21].position = { -10, -20, 0 };

				object[1].texNumber = 13;
				object[2].texNumber = 12;
				object[3].texNumber = 1;
				object[4].texNumber = 1;
				object[5].texNumber = 1;
				object[6].texNumber = 1;
				object[7].texNumber = 12;
				object[8].texNumber = 11;
				object[9].texNumber = 1;
				object[10].texNumber = 13;
				object[11].texNumber = 1;
				object[12].texNumber = 11;
				object[13].texNumber = 11;
				object[14].texNumber = 11;
				object[15].texNumber = 1;
				object[16].texNumber = 11;
				object[17].texNumber = 1;
				object[18].texNumber = 1;
				object[19].texNumber = 13;
				object[20].texNumber = 12;
				object[21].texNumber = 1;

				//いらないブロックには認識の外側に消えていただく
				for (int i = 22; i < 73; i++)
				{
					object[i].position = { 1000, 1000, 1000 };
				}

				//プレイヤー
				for (int i = 75; i < 81; i++)
				{
					object[i].color = { 1,1,1,1 };
					rotaX = 0;
					rotaY = 0;
					rotaXCount = 0;
					rotaYCount = 0;
					object[i].rotation = { 0, 0, 0 };
					object[i].position.x = object[11].position.x;
					object[i].position.y = object[11].position.y;
				}

				for (int i = 87; i < 93; i++)
				{
					object[i].color = { 1,1,1,1 };
				}

				maxRotaX = 2;
				maxRotaY = 2;
				minRotaX = -2;
				minRotaY = -2;
				MaxBlock = 22;//ステージの最大ブロック数を指定
				isChange = false;
			}

			else if (StageNum == 5)
			{
				object[1].position = { -70, 20, 0 };
				object[2].position = { -60, 20, 0 };
				object[3].position = { -50, 20, 0 };
				object[4].position = { -40, 20, 0 };
				object[5].position = { -30, 20, 0 };
				object[6].position = { -20, 20, 0 };
				object[7].position = { -10, 20, 0 };
				object[8].position = { -70, 10, 0 };
				object[9].position = { -50, 10, 0 };
				object[10].position = { -30, 10, 0 };
				object[11].position = { -10, 10, 0 };
				object[12].position = { -70, 0, 0 };
				object[13].position = { -60, 0, 0 };
				object[14].position = { -50, 0, 0 };
				object[15].position = { -40, 0, 0 };
				object[16].position = { -30, 0, 0 };
				object[17].position = { -20, 0, 0 };
				object[18].position = { -10, 0, 0 };
				object[19].position = { -70, -10, 0 };
				object[20].position = { -50, -10, 0 };
				object[21].position = { -30, -10, 0 };
				object[22].position = { -10, -10, 0 };
				object[23].position = { -70, -20, 0 };
				object[24].position = { -60, -20, 0 };
				object[25].position = { -50, -20, 0 };
				object[26].position = { -40, -20, 0 };
				object[27].position = { -30, -20, 0 };
				object[28].position = { -20, -20, 0 };
				object[29].position = { -10, -20, 0 };

				object[1].texNumber = 1;
				object[2].texNumber = 12;
				object[3].texNumber = 1;
				object[4].texNumber = 1;
				object[5].texNumber = 1;
				object[6].texNumber = 12;
				object[7].texNumber = 1;
				object[8].texNumber = 13;
				object[9].texNumber = 1;
				object[10].texNumber = 11;
				object[11].texNumber = 1;
				object[12].texNumber = 12;
				object[13].texNumber = 1;
				object[14].texNumber = 11;
				object[15].texNumber = 1;
				object[16].texNumber = 1;
				object[17].texNumber = 13;
				object[18].texNumber = 1;
				object[19].texNumber = 12;
				object[20].texNumber = 13;
				object[21].texNumber = 1;
				object[22].texNumber = 1;
				object[23].texNumber = 1;
				object[24].texNumber = 11;
				object[25].texNumber = 1;
				object[26].texNumber = 12;
				object[27].texNumber = 13;
				object[28].texNumber = 1;
				object[29].texNumber = 11;

				//いらないブロックには認識の外側に消えていただく
				for (int i = 30; i < 73; i++)
				{
					object[i].position = { 1000, 1000, 1000 };
				}

				//プレイヤー
				for (int i = 75; i < 81; i++)
				{
					object[i].color = { 1,1,1,1 };
					rotaX = 0;
					rotaY = 0;
					rotaXCount = 0;
					rotaYCount = 0;
					object[i].rotation = { 0, 0, 0 };
					object[i].position.x = object[1].position.x;
					object[i].position.y = object[1].position.y;
				}

				for (int i = 87; i < 93; i++)
				{
					object[i].color = { 1,1,1,1 };
				}

				maxRotaX = 6;
				maxRotaY = 0;
				minRotaX = 0;
				minRotaY = -4;
				MaxBlock = 30;//ステージの最大ブロック数を指定
				isChange = false;
			}

			else if (StageNum == 6)
			{
				object[1].position = { -70, 30, 0 };
				object[2].position = { -60, 30, 0 };
				object[3].position = { -50, 30, 0 };
				object[4].position = { -40, 30, 0 };
				object[5].position = { -30, 30, 0 };
				object[6].position = { -20, 30, 0 };
				object[7].position = { -10, 30, 0 };
				object[8].position = { -70, 20, 0 };
				object[9].position = { -50, 20, 0 };
				object[10].position = { -30, 20, 0 };
				object[11].position = { -10, 20, 0 };
				object[12].position = { -70, 10, 0 };
				object[13].position = { -60, 10, 0 };
				object[14].position = { -50, 10, 0 };
				object[15].position = { -40, 10, 0 };
				object[16].position = { -30, 10, 0 };
				object[17].position = { -20, 10, 0 };
				object[18].position = { -10, 10, 0 };
				object[19].position = { -70, 0, 0 };
				object[20].position = { -50, 0, 0 };
				object[21].position = { -30, 0, 0 };
				object[22].position = { -10, 0, 0 };
				object[23].position = { -70, -10, 0 };
				object[24].position = { -60, -10, 0 };
				object[25].position = { -50, -10, 0 };
				object[26].position = { -40, -10, 0 };
				object[27].position = { -30, -10, 0 };
				object[28].position = { -20, -10, 0 };
				object[29].position = { -10, -10, 0 };
				object[30].position = { -70, -20, 0 };
				object[31].position = { -50, -20, 0 };
				object[32].position = { -30, -20, 0 };
				object[33].position = { -10, -20, 0 };
				object[34].position = { -70, -30, 0 };
				object[35].position = { -60, -30, 0 };
				object[36].position = { -50, -30, 0 };
				object[37].position = { -40, -30, 0 };
				object[38].position = { -30, -30, 0 };
				object[39].position = { -20, -30, 0 };
				object[40].position = { -10, -30, 0 };

				object[1].texNumber = 1;
				object[2].texNumber = 13;
				object[3].texNumber = 1;
				object[4].texNumber = 11;
				object[5].texNumber = 1;
				object[6].texNumber = 13;
				object[7].texNumber = 1;
				object[8].texNumber = 12;
				object[9].texNumber = 1;
				object[10].texNumber = 11;
				object[11].texNumber = 1;
				object[12].texNumber = 12;
				object[13].texNumber = 11;
				object[14].texNumber = 1;
				object[15].texNumber = 1;
				object[16].texNumber = 13;
				object[17].texNumber = 1;
				object[18].texNumber = 11;
				object[19].texNumber = 1;
				object[20].texNumber = 1;
				object[21].texNumber = 12;
				object[22].texNumber = 1;
				object[23].texNumber = 11;
				object[24].texNumber = 1;
				object[25].texNumber = 1;
				object[26].texNumber = 11;
				object[27].texNumber = 1;
				object[28].texNumber = 1;
				object[29].texNumber = 12;
				object[30].texNumber = 1;
				object[31].texNumber = 1;
				object[32].texNumber = 13;
				object[33].texNumber = 1;
				object[34].texNumber = 12;
				object[35].texNumber = 1;
				object[36].texNumber = 13;
				object[37].texNumber = 1;
				object[38].texNumber = 1;
				object[39].texNumber = 1;
				object[40].texNumber = 11;

				//いらないブロックには認識の外側に消えていただく
				for (int i = 41; i < 73; i++)
				{
					object[i].position = { 1000, 1000, 1000 };
				}

				//プレイヤー
				for (int i = 75; i < 81; i++)
				{
					object[i].color = { 1,1,1,1 };
					rotaX = 0;
					rotaY = 0;
					rotaXCount = 0;
					rotaYCount = 0;
					object[i].rotation = { 0, 0, 0 };
					object[i].position.x = object[1].position.x;
					object[i].position.y = object[1].position.y;
				}

				for (int i = 87; i < 93; i++)
				{
					object[i].color = { 1,1,1,1 };
				}

				maxRotaX = 6;
				maxRotaY = 0;
				minRotaX = 0;
				minRotaY = -6;
				MaxBlock = 41;//ステージの最大ブロック数を指定
				isChange = false;
			}

			else if (StageNum == 7)
			{
			object[1].position = { -40, 30, 0 };
			object[2].position = { -30, 30, 0 };
			object[3].position = { -20, 30, 0 };
			object[4].position = { -40, 20, 0 };
			object[5].position = { -20, 20, 0 };
			object[6].position = { -40, 10, 0 };
			object[7].position = { -30, 10, 0 };
			object[8].position = { -20, 10, 0 };
			object[9].position = { -40, 0, 0 };
			object[10].position = { -20, 0, 0 };
			object[11].position = { -40, -10, 0 };
			object[12].position = { -30, -10, 0 };
			object[13].position = { -20, -10, 0 };
			object[14].position = { -40, -20, 0 };
			object[15].position = { -20, -20, 0 };
			object[16].position = { -40, -30, 0 };
			object[17].position = { -30, -30, 0 };
			object[18].position = { -20, -30, 0 };

			object[1].texNumber = 11;
			object[2].texNumber = 1;
			object[3].texNumber = 13;
			object[4].texNumber = 1;
			object[5].texNumber = 1;
			object[6].texNumber = 1;
			object[7].texNumber = 11;
			object[8].texNumber = 1;
			object[9].texNumber = 1;
			object[10].texNumber = 13;
			object[11].texNumber = 1;
			object[12].texNumber = 12;
			object[13].texNumber = 1;
			object[14].texNumber = 12;
			object[15].texNumber = 1;
			object[16].texNumber = 1;
			object[17].texNumber = 1;
			object[18].texNumber = 1;

			//いらないブロックには認識の外側に消えていただく
			for (int i = 19; i < 73; i++)
			{
				object[i].position = { 1000, 1000, 1000 };
			}

			//プレイヤー
			for (int i = 75; i < 81; i++)
			{
				object[i].color = { 1,1,1,1 };
				rotaX = 0;
				rotaY = 0;
				rotaXCount = 0;
				rotaYCount = 0;
				object[i].rotation = { 0, 0, 0 };
				object[i].position.x = object[6].position.x;
				object[i].position.y = object[6].position.y;
			}

			for (int i = 87; i < 93; i++)
			{
				object[i].color = { 1,1,1,1 };
			}

			object[78].color = { 1,0,0,1 };
			object[78 + 12].color = { 1,0,0,1 };

			maxRotaX = 2;
			maxRotaY = 2;
			minRotaX = 0;
			minRotaY = -4;
			MaxBlock = 19;//ステージの最大ブロック数を指定
			isChange = false;
			}

			else if (StageNum == 8)
			{
			object[1].position = { -50, 30, 0 };
			object[2].position = { -40, 30, 0 };
			object[3].position = { -30, 30, 0 };
			object[4].position = { -20, 30, 0 };
			object[5].position = { -10, 30, 0 };
			object[6].position = { -50, 20, 0 };
			object[7].position = { -30, 20, 0 };
			object[8].position = { -10, 20, 0 };
			object[9].position = { -50, 10, 0 };
			object[10].position = { -40, 10, 0 };
			object[11].position = { -30, 10, 0 };
			object[12].position = { -20, 10, 0 };
			object[13].position = { -10, 10, 0 };
			object[14].position = { -50, 0, 0 };
			object[15].position = { -30, 0, 0 };
			object[16].position = { -10, 0, 0 };
			object[17].position = { -50, -10, 0 };
			object[18].position = { -40, -10, 0 };
			object[19].position = { -30, -10, 0 };
			object[20].position = { -20, -10, 0 };
			object[21].position = { -10, -10, 0 };
			object[22].position = { -50, -20, 0 };
			object[23].position = { -30, -20, 0 };
			object[24].position = { -10, -20, 0 };
			object[25].position = { -50, -30, 0 };
			object[26].position = { -40, -30, 0 };
			object[27].position = { -30, -30, 0 };
			object[28].position = { -20, -30, 0 };
			object[29].position = { -10, -30, 0 };

			object[1].texNumber = 12;
			object[2].texNumber = 11;
			object[3].texNumber = 13;
			object[4].texNumber = 12;
			object[5].texNumber = 1;
			object[6].texNumber = 11;
			object[7].texNumber = 12;
			object[8].texNumber = 13;
			object[9].texNumber = 1;
			object[10].texNumber = 13;
			object[11].texNumber = 1;
			object[12].texNumber = 1;
			object[13].texNumber = 11;
			object[14].texNumber = 11;
			object[15].texNumber = 1;
			object[16].texNumber = 12;
			object[17].texNumber = 12;
			object[18].texNumber = 1;
			object[19].texNumber = 1;
			object[20].texNumber = 11;
			object[21].texNumber = 1;
			object[22].texNumber = 11;
			object[23].texNumber = 1;
			object[24].texNumber = 12;
			object[25].texNumber = 13;
			object[26].texNumber = 1;
			object[27].texNumber = 13;
			object[28].texNumber = 1;
			object[29].texNumber = 11;

			//いらないブロックには認識の外側に消えていただく
			for (int i = 30; i < 73; i++)
			{
				object[i].position = { 1000, 1000, 1000 };
			}

			//プレイヤー
			for (int i = 75; i < 81; i++)
			{
				object[i].color = { 1,1,1,1 };
				rotaX = 0;
				rotaY = 0;
				rotaXCount = 0;
				rotaYCount = 0;
				object[i].rotation = { 0, 0, 0 };
				object[i].position.x = object[21].position.x;
				object[i].position.y = object[21].position.y;
			}

			for (int i = 87; i < 93; i++)
			{
				object[i].color = { 1,1,1,1 };
			}

			object[75].color = { 0,1,0,1 };
			object[75 + 12].color = { 0,1,0,1 };
			object[79].color = { 0,0,1,1 };
			object[79 + 12].color = { 0,0,1,1 };

			maxRotaX = 0;
			maxRotaY = 4;
			minRotaX = -4;
			minRotaY = -2;
			MaxBlock = 30;//ステージの最大ブロック数を指定
			isChange = false;
			}

			else
			{
				//ステージ1を参考に入力

				isChange = false;
			}
		}

		if (isHit == true)
		{
			//ステージにある地面の数だけ回す(初期値もステージの最小のi)
			for (int i = 1; i < MaxBlock; i++)
			{
				//地面のオブジェクトiとの当たり判定
				if (object[i].position.x == object[75].position.x && object[i].position.y == object[75].position.y)
				{
					//地面に何もないとき
					if (object[i].texNumber == 1)
					{
						if (rotaY == 0)
						{
							if (rotaX == 0)
							{
								//赤
								if (object[76].color.y == 0 && object[76].color.z == 0)
								{
									object[i].texNumber = 11;
								}

								//緑
								else if (object[76].color.x == 0 && object[76].color.z == 0)
								{
									object[i].texNumber = 12;
								}

								//青
								else if (object[76].color.x == 0 && object[76].color.y == 0)
								{
									object[i].texNumber = 13;
								}

								object[76].color = { 1,1,1,1 };
								object[76 + 12].color = { 1,1,1,1 };
							}

							if (rotaX == 1 || rotaX == -3)
							{
								//赤
								if (object[78].color.y == 0 && object[78].color.z == 0)
								{
									object[i].texNumber = 11;
								}

								//緑
								else if (object[78].color.x == 0 && object[78].color.z == 0)
								{
									object[i].texNumber = 12;
								}

								//青
								else if (object[78].color.x == 0 && object[78].color.y == 0)
								{
									object[i].texNumber = 13;
								}

								object[78].color = { 1,1,1,1 };
								object[78 + 12].color = { 1,1,1,1 };
							}

							if (abs(rotaX) == 2)
							{
								//赤
								if (object[75].color.y == 0 && object[75].color.z == 0)
								{
									object[i].texNumber = 11;
								}

								//緑
								else if (object[75].color.x == 0 && object[75].color.z == 0)
								{
									object[i].texNumber = 12;
								}

								//青
								else if (object[75].color.x == 0 && object[75].color.y == 0)
								{
									object[i].texNumber = 13;
								}

								object[75].color = { 1,1,1,1 };
								object[75 + 12].color = { 1,1,1,1 };
							}

							if (rotaX == 3 || rotaX == -1)
							{
								//赤
								if (object[77].color.y == 0 && object[77].color.z == 0)
								{
									object[i].texNumber = 11;
								}

								//緑
								else if (object[77].color.x == 0 && object[77].color.z == 0)
								{
									object[i].texNumber = 12;
								}

								//青
								else if (object[77].color.x == 0 && object[77].color.y == 0)
								{
									object[i].texNumber = 13;
								}

								object[77].color = { 1,1,1,1 };
								object[77 + 12].color = { 1,1,1,1 };
							}
						}

						if (rotaY == 1 || rotaY == -3)
						{
							if (rotaX == 0)
							{
								//赤
								if (object[79].color.y == 0 && object[79].color.z == 0)
								{
									object[i].texNumber = 11;
								}

								//緑
								else if (object[79].color.x == 0 && object[79].color.z == 0)
								{
									object[i].texNumber = 12;
								}

								//青
								else if (object[79].color.x == 0 && object[79].color.y == 0)
								{
									object[i].texNumber = 13;
								}

								object[79].color = { 1,1,1,1 };
								object[79 + 12].color = { 1,1,1,1 };
							}

							if (abs(rotaX) == 2)
							{
								//赤
								if (object[79].color.y == 0 && object[79].color.z == 0)
								{
									object[i].texNumber = 11;
								}

								//緑
								else if (object[79].color.x == 0 && object[79].color.z == 0)
								{
									object[i].texNumber = 12;
								}

								//青
								else if (object[79].color.x == 0 && object[79].color.y == 0)
								{
									object[i].texNumber = 13;
								}

								object[79].color = { 1,1,1,1 };
								object[79 + 12].color = { 1,1,1,1 };
							}
						}

						if (abs(rotaY) == 2)
						{
							if (rotaX == 0)
							{
								//赤
								if (object[75].color.y == 0 && object[75].color.z == 0)
								{
									object[i].texNumber = 11;
								}

								//緑
								else if (object[75].color.x == 0 && object[75].color.z == 0)
								{
									object[i].texNumber = 12;
								}

								//青
								else if (object[75].color.x == 0 && object[75].color.y == 0)
								{
									object[i].texNumber = 13;
								}

								object[75].color = { 1,1,1,1 };
								object[75 + 12].color = { 1,1,1,1 };
							}

							if (rotaX == 1 || rotaX == -3)
							{
								//赤
								if (object[78].color.y == 0 && object[78].color.z == 0)
								{
									object[i].texNumber = 11;
								}

								//緑
								else if (object[78].color.x == 0 && object[78].color.z == 0)
								{
									object[i].texNumber = 12;
								}

								//青
								else if (object[78].color.x == 0 && object[78].color.y == 0)
								{
									object[i].texNumber = 13;
								}

								object[78].color = { 1,1,1,1 };
								object[78 + 12].color = { 1,1,1,1 };
							}

							if (abs(rotaX) == 2)
							{
								//赤
								if (object[76].color.y == 0 && object[76].color.z == 0)
								{
									object[i].texNumber = 11;
								}

								//緑
								else if (object[76].color.x == 0 && object[76].color.z == 0)
								{
									object[i].texNumber = 12;
								}

								//青
								else if (object[76].color.x == 0 && object[76].color.y == 0)
								{
									object[i].texNumber = 13;
								}

								object[76].color = { 1,1,1,1 };
								object[76 + 12].color = { 1,1,1,1 };
							}

							if (rotaX == 3 || rotaX == -1)
							{
								//赤
								if (object[77].color.y == 0 && object[77].color.z == 0)
								{
									object[i].texNumber = 11;
								}

								//緑
								else if (object[77].color.x == 0 && object[77].color.z == 0)
								{
									object[i].texNumber = 12;
								}

								//青
								else if (object[77].color.x == 0 && object[77].color.y == 0)
								{
									object[i].texNumber = 13;
								}

								object[77].color = { 1,1,1,1 };
								object[77 + 12].color = { 1,1,1,1 };
							}
						}

						if (rotaY == 3 || rotaY == -1)
						{
							if (rotaX == 0)
							{
								//赤
								if (object[80].color.y == 0 && object[80].color.z == 0)
								{
									object[i].texNumber = 11;
								}

								//緑
								else if (object[80].color.x == 0 && object[80].color.z == 0)
								{
									object[i].texNumber = 12;
								}

								//青
								else if (object[80].color.x == 0 && object[80].color.y == 0)
								{
									object[i].texNumber = 13;
								}

								object[80].color = { 1,1,1,1 };
								object[80 + 12].color = { 1,1,1,1 };
							}

							if (abs(rotaX) == 2)
							{
								//赤
								if (object[80].color.y == 0 && object[80].color.z == 0)
								{
									object[i].texNumber = 11;
								}

								//緑
								else if (object[80].color.x == 0 && object[80].color.z == 0)
								{
									object[i].texNumber = 12;
								}

								//青
								else if (object[80].color.x == 0 && object[80].color.y == 0)
								{
									object[i].texNumber = 13;
								}

								object[80].color = { 1,1,1,1 };
								object[80 + 12].color = { 1,1,1,1 };
							}
						}
					}

					//地面の色が赤の時
					else if (object[i].texNumber == 11)
					{
						if (rotaY == 0)
						{
							if (rotaX == 0)
							{
								if (object[76].color.x == 1 && object[76].color.y == 1 && object[76].color.z == 1)
								{
									object[i].texNumber = 1;
									object[76].color = { 1,0,0,1 };
									object[76 + 12].color = { 1,0,0,1 };
								}
							}

							if (rotaX == 1 || rotaX == -3)
							{
								if (object[78].color.x == 1 && object[78].color.y == 1 && object[78].color.z == 1)
								{
									object[i].texNumber = 1;
									object[78].color = { 1,0,0,1 };
									object[78 + 12].color = { 1,0,0,1 };
								}
							}

							if (abs(rotaX) == 2)
							{
								if (object[75].color.x == 1 && object[75].color.y == 1 && object[75].color.z == 1)
								{
									object[i].texNumber = 1;
									object[75].color = { 1,0,0,1 };
									object[75 + 12].color = { 1,0,0,1 };
								}
							}

							if (rotaX == 3 || rotaX == -1)
							{
								if (object[77].color.x == 1 && object[77].color.y == 1 && object[77].color.z == 1)
								{
									object[i].texNumber = 1;
									object[77].color = { 1,0,0,1 };
									object[77 + 12].color = { 1,0,0,1 };
								}
							}
						}

						if (rotaY == 1 || rotaY == -3)
						{
							if (rotaX == 0)
							{
								if (object[79].color.x == 1 && object[79].color.y == 1 && object[79].color.z == 1)
								{
									object[i].texNumber = 1;
									object[79].color = { 1,0,0,1 };
									object[79 + 12].color = { 1,0,0,1 };
								}
							}

							if (abs(rotaX) == 2)
							{
								if (object[79].color.x == 1 && object[79].color.y == 1 && object[79].color.z == 1)
								{
									object[i].texNumber = 1;
									object[79].color = { 1,0,0,1 };
									object[79 + 12].color = { 1,0,0,1 };
								}
							}
						}

						if (abs(rotaY) == 2)
						{
							if (rotaX == 0)
							{
								if (object[75].color.x == 1 && object[75].color.y == 1 && object[75].color.z == 1)
								{
									object[i].texNumber = 1;
									object[75].color = { 1,0,0,1 };
									object[75 + 12].color = { 1,0,0,1 };
								}
							}
							if (rotaX == 1 || rotaX == -3)
							{
								if (object[78].color.x == 1 && object[78].color.y == 1 && object[78].color.z == 1)
								{
									object[i].texNumber = 1;
									object[78].color = { 1,0,0,1 };
									object[78 + 12].color = { 1,0,0,1 };
								}
							}
							if (abs(rotaX) == 2)
							{
								if (object[76].color.x == 1 && object[76].color.y == 1 && object[76].color.z == 1)
								{
									object[i].texNumber = 1;
									object[76].color = { 1,0,0,1 };
									object[76 + 12].color = { 1,0,0,1 };
								}
							}
							if (rotaX == 3 || rotaX == -1)
							{
								if (object[77].color.x == 1 && object[77].color.y == 1 && object[77].color.z == 1)
								{
									object[i].texNumber = 1;
									object[77].color = { 1,0,0,1 };
									object[77 + 12].color = { 1,0,0,1 };
								}
							}
						}

						if (rotaY == 3 || rotaY == -1)
						{
							if (rotaX == 0)
							{
								if (object[80].color.x == 1 && object[80].color.y == 1 && object[80].color.z == 1)
								{
									object[i].texNumber = 1;
									object[80].color = { 1,0,0,1 };
									object[80 + 12].color = { 1,0,0,1 };
								}
							}

							if (abs(rotaX) == 2)
							{
								if (object[80].color.x == 1 && object[80].color.y == 1 && object[80].color.z == 1)
								{
									object[i].texNumber = 1;
									object[80].color = { 1,0,0,1 };
									object[80 + 12].color = { 1,0,0,1 };
								}
							}
						}
					}

					//地面の色が緑の時
					else if (object[i].texNumber == 12)
					{
						if (rotaY == 0)
						{
							if (rotaX == 0)
							{
								if (object[76].color.x == 1 && object[76].color.y == 1 && object[76].color.z == 1)
								{
									object[i].texNumber = 1;
									object[76].color = { 0,1,0,1 };
									object[76 + 12].color = { 0,1,0,1 };
								}
							}

							if (rotaX == 1 || rotaX == -3)
							{
								if (object[78].color.x == 1 && object[78].color.y == 1 && object[78].color.z == 1)
								{
									object[i].texNumber = 1;
									object[78].color = { 0,1,0,1 };
									object[78 + 12].color = { 0,1,0,1 };
								}
							}
							if (abs(rotaX) == 2)
							{
								if (object[75].color.x == 1 && object[75].color.y == 1 && object[75].color.z == 1)
								{
									object[i].texNumber = 1;
									object[75].color = { 0,1,0,1 };
									object[75 + 12].color = { 0,1,0,1 };
								}
							}
							if (rotaX == 3 || rotaX == -1)
							{
								if (object[77].color.x == 1 && object[77].color.y == 1 && object[77].color.z == 1)
								{
									object[i].texNumber = 1;
									object[77].color = { 0,1,0,1 };
									object[77 + 12].color = { 0,1,0,1 };
								}
							}
						}

						if (rotaY == 1 || rotaY == -3)
						{
							if (rotaX == 0)
							{
								if (object[79].color.x == 1 && object[79].color.y == 1 && object[79].color.z == 1)
								{
									object[i].texNumber = 1;
									object[79].color = { 0,1,0,1 };
									object[79 + 12].color = { 0,1,0,1 };
								}
							}
							if (abs(rotaX) == 2)
							{
								if (object[79].color.x == 1 && object[79].color.y == 1 && object[79].color.z == 1)
								{
									object[i].texNumber = 1;
									object[79].color = { 0,1,0,1 };
									object[79 + 12].color = { 0,1,0,1 };
								}
							}
						}

						if (abs(rotaY) == 2)
						{
							if (rotaX == 0)
							{
								if (object[75].color.x == 1 && object[75].color.y == 1 && object[75].color.z == 1)
								{
									object[i].texNumber = 1;
									object[75].color = { 0,1,0,1 };
									object[75 + 12].color = { 0,1,0,1 };
								}
							}
							if (rotaX == 1 || rotaX == -3)
							{
								if (object[78].color.x == 1 && object[78].color.y == 1 && object[78].color.z == 1)
								{
									object[i].texNumber = 1;
									object[78].color = { 0,1,0,1 };
									object[78 + 12].color = { 0,1,0,1 };
								}
							}
							if (abs(rotaX) == 2)
							{
								if (object[76].color.x == 1 && object[76].color.y == 1 && object[76].color.z == 1)
								{
									object[i].texNumber = 1;
									object[76].color = { 0,1,0,1 };
									object[76 + 12].color = { 0,1,0,1 };
								}
							}
							if (rotaX == 3 || rotaX == -1)
							{
								if (object[77].color.x == 1 && object[77].color.y == 1 && object[77].color.z == 1)
								{
									object[i].texNumber = 1;
									object[77].color = { 0,1,0,1 };
									object[77 + 12].color = { 0,1,0,1 };
								}
							}
						}

						if (rotaY == 3 || rotaY == -1)
						{
							if (rotaX == 0)
							{
								if (object[80].color.x == 1 && object[80].color.y == 1 && object[80].color.z == 1)
								{
									object[i].texNumber = 1;
									object[80].color = { 0,1,0,1 };
									object[80 + 12].color = { 0,1,0,1 };
								}
							}
							if (abs(rotaX) == 2)
							{
								if (object[80].color.x == 1 && object[80].color.y == 1 && object[80].color.z == 1)
								{
									object[i].texNumber = 1;
									object[80].color = { 0,1,0,1 };
									object[80 + 12].color = { 0,1,0,1 };
								}
							}
						}
					}

					//地面の色が青の時
					else if (object[i].texNumber == 13)
					{
						if (rotaY == 0)
						{
							if (rotaX == 0)
							{
								if (object[76].color.x == 1 && object[76].color.y == 1 && object[76].color.z == 1)
								{
									object[i].texNumber = 1;
									object[76].color = { 0,0,1,1 };
									object[76 + 12].color = { 0,0,1,1 };
								}
							}

							if (rotaX == 1 || rotaX == -3)
							{
								if (object[78].color.x == 1 && object[78].color.y == 1 && object[78].color.z == 1)
								{
									object[i].texNumber = 1;
									object[78].color = { 0,0,1,1 };
									object[78 + 12].color = { 0,0,1,1 };
								}
							}
							if (abs(rotaX) == 2)
							{
								if (object[75].color.x == 1 && object[75].color.y == 1 && object[75].color.z == 1)
								{
									object[i].texNumber = 1;
									object[75].color = { 0,0,1,1 };
									object[75 + 12].color = { 0,0,1,1 };
								}
							}

							if (rotaX == 3 || rotaX == -1)
							{
								if (object[77].color.x == 1 && object[77].color.y == 1 && object[77].color.z == 1)
								{
									object[i].texNumber = 1;
									object[77].color = { 0,0,1,1 };
									object[77 + 12].color = { 0,0,1,1 };
								}
							}
						}

						if (rotaY == 1 || rotaY == -3)
						{
							if (rotaX == 0)
							{
								if (object[79].color.x == 1 && object[79].color.y == 1 && object[79].color.z == 1)
								{
									object[i].texNumber = 1;
									object[79].color = { 0,0,1,1 };
									object[79 + 12].color = { 0,0,1,1 };
								}
							}
							if (abs(rotaX) == 2)
							{
								if (object[79].color.x == 1 && object[79].color.y == 1 && object[79].color.z == 1)
								{
									object[i].texNumber = 1;
									object[79].color = { 0,0,1,1 };
									object[79 + 12].color = { 0,0,1,1 };
								}
							}
						}

						if (abs(rotaY) == 2)
						{
							if (rotaX == 0)
							{
								if (object[75].color.x == 1 && object[75].color.y == 1 && object[75].color.z == 1)
								{
									object[i].texNumber = 1;
									object[75].color = { 0,0,1,1 };
									object[75 + 12].color = { 0,0,1,1 };
								}
							}

							if (rotaX == 1 || rotaX == -3)
							{
								if (object[78].color.x == 1 && object[78].color.y == 1 && object[78].color.z == 1)
								{
									object[i].texNumber = 1;
									object[78].color = { 0,0,1,1 };
									object[78 + 12].color = { 0,0,1,1 };
								}
							}
							if (abs(rotaX) == 2)
							{
								if (object[76].color.x == 1 && object[76].color.y == 1 && object[76].color.z == 1)
								{
									object[i].texNumber = 1;
									object[76].color = { 0,0,1,1 };
									object[76 + 12].color = { 0,0,1,1 };
								}
							}

							if (rotaX == 3 || rotaX == -1)
							{
								if (object[77].color.x == 1 && object[77].color.y == 1 && object[77].color.z == 1)
								{
									object[i].texNumber = 1;
									object[77].color = { 0,0,1,1 };
									object[77 + 12].color = { 0,0,1,1 };
								}
							}
						}

						if (rotaY == 3 || rotaY == -1)
						{
							if (rotaX == 0)
							{
								if (object[80].color.x == 1 && object[80].color.y == 1 && object[80].color.z == 1)
								{
									object[i].texNumber = 1;
									object[80].color = { 0,0,1,1 };
									object[80 + 12].color = { 0,0,1,1 };
								}
							}

							if (abs(rotaX) == 2)
							{
								if (object[80].color.x == 1 && object[80].color.y == 1 && object[80].color.z == 1)
								{
									object[i].texNumber = 1;
									object[80].color = { 0,0,1,1 };
									object[80 + 12].color = { 0,0,1,1 };
								}
							}
						}
					}

					isHit = false;
				}

				//ステージ0において、地面のオブジェクトiに数字がついていて、そのブロックの上に乗っているとき
				if (StageNum == 0 && object[i].position.x == object[75].position.x && object[i].position.y == object[75].position.y
					&& object[i].texNumber != 1)
				{
					//数字が1の時
					if (object[i].texNumber == 2 || object[i].texNumber == 14)
					{
						StageNum = 1;
						isChange = true;
					}

					//数字が2の時
					else if (object[i].texNumber == 3 || object[i].texNumber == 15)
					{
						StageNum = 2;
						isChange = true;
					}

					//数字が3の時
					else if (object[i].texNumber == 4 || object[i].texNumber == 16)
					{
						StageNum = 3;
						isChange = true;
					}

					//数字が4の時
					else if (object[i].texNumber == 5 || object[i].texNumber == 17)
					{
						StageNum = 4;
						isChange = true;
					}

					//数字が5の時
					else if (object[i].texNumber == 6 || object[i].texNumber == 18)
					{
						StageNum = 5;
						isChange = true;
					}

					//数字が6の時
					else if (object[i].texNumber == 7 || object[i].texNumber == 19)
					{
						StageNum = 6;
						isChange = true;
					}

					//数字が7の時
					else if (object[i].texNumber == 8 || object[i].texNumber == 20)
					{
						StageNum = 7;
						isChange = true;
					}

					//数字が8の時
					else if (object[i].texNumber == 9 || object[i].texNumber == 21)
					{
						StageNum = 8;
						isChange = true;
					}

					//数字が9の時
					else if (object[i].texNumber == 10)
					{
						StageNum = 9;
						isChange = true;
					}
				}
			}
		}

		//クリア処理
		if (StageNum == 1)
		{
			if (object[78].color.y == 0 && object[78].color.z == 0)
			{
				if ((object[76].color.x == 0 && object[76].color.y == 0) && (object[77].color.x == 0 && object[77].color.z == 0))
				{
					if (isSound[1] == false)
					{
						sound->SoundPlayWave(soundData2);
						isSound[1] = true;
					}

					object[64].texNumber = 14;
					SceneNum = End;
					isChange = false;//ステージ変更処理
					isLoad = false;//ロード
					LoadCount = 20;//ロードのウェイト
					StageNum = 0;//0はステージセレクト。
					MoveDirection = Right;//進行方向
					isLeft = false;//進行方向
					isRight = false;//進行方向
					isUp = false;//進行方向
					isDown = false;//進行方向
					isRota = false;//回転しているか
					timer = 0;//回転、移動処理の時間
					rotaX = 0;//今のXの向き正なら右に負なら左に回転している
					rotaY = 0;//今のYの向き正なら下に負なら上に回転している
				}
			}
		}

		else if (StageNum == 2)
		{
			if (object[77].color.y == 0 && object[77].color.z == 0)
			{
				if ((object[79].color.x == 0 && object[79].color.z == 0) && (object[76].color.x == 0 && object[76].color.y == 0))
				{
					if (isSound[1] == false)
					{
						sound->SoundPlayWave(soundData2);
						isSound[1] = true;
					}

					object[65].texNumber = 15;
					SceneNum = End;
					isChange = false;//ステージ変更処理
					isLoad = false;//ロード
					LoadCount = 20;//ロードのウェイト
					StageNum = 0;//0はステージセレクト。
					MoveDirection = Right;//進行方向
					isLeft = false;//進行方向
					isRight = false;//進行方向
					isUp = false;//進行方向
					isDown = false;//進行方向
					isRota = false;//回転しているか
					timer = 0;//回転、移動処理の時間
					rotaX = 0;//今のXの向き正なら右に負なら左に回転している
					rotaY = 0;//今のYの向き正なら下に負なら上に回転している
				}
			}
		}

		else if (StageNum == 3)
		{
			if (object[79].color.x == 1 && object[79].color.y == 1 && object[79].color.z == 1)
			{
				if ((object[75].color.x == 0 && object[75].color.z == 0) && (object[76].color.x == 0 && object[76].color.z == 0) && (object[77].color.y == 0 && object[77].color.z == 0) && (object[78].color.y == 0 && object[78].color.z == 0) && (object[80].color.x == 0 && object[80].color.y == 0))
				{
					if (isSound[1] == false)
					{
						sound->SoundPlayWave(soundData2);
						isSound[1] = true;
					}

					object[66].texNumber = 16;
					SceneNum = End;
					isChange = false;//ステージ変更処理
					isLoad = false;//ロード
					LoadCount = 20;//ロードのウェイト
					StageNum = 0;//0はステージセレクト。
					MoveDirection = Right;//進行方向
					isLeft = false;//進行方向
					isRight = false;//進行方向
					isUp = false;//進行方向
					isDown = false;//進行方向
					isRota = false;//回転しているか
					timer = 0;//回転、移動処理の時間
					rotaX = 0;//今のXの向き正なら右に負なら左に回転している
					rotaY = 0;//今のYの向き正なら下に負なら上に回転している
				}
			}
		}

		else if (StageNum == 4)
		{
			if ((object[75].color.x == 0 && object[75].color.y == 0) && (object[76].color.x == 0 && object[76].color.y == 0) && (object[77].color.x == 0 && object[77].color.z == 0) && (object[78].color.x == 0 && object[78].color.z == 0) && (object[79].color.y == 0 && object[79].color.z == 0) && (object[80].color.y == 0 && object[80].color.z == 0))
			{
				if (isSound[1] == false)
				{
					sound->SoundPlayWave(soundData2);
					isSound[1] = true;
				}

				object[67].texNumber = 17;
				SceneNum = End;
				isChange = false;//ステージ変更処理
				isLoad = false;//ロード
				LoadCount = 20;//ロードのウェイト
				StageNum = 0;//0はステージセレクト。
				MoveDirection = Right;//進行方向
				isLeft = false;//進行方向
				isRight = false;//進行方向
				isUp = false;//進行方向
				isDown = false;//進行方向
				isRota = false;//回転しているか
				timer = 0;//回転、移動処理の時間
				rotaX = 0;//今のXの向き正なら右に負なら左に回転している
				rotaY = 0;//今のYの向き正なら下に負なら上に回転している
			}
		}

		else if (StageNum == 5)
		{
			if ((object[75].color.x == 0 && object[75].color.y == 0) && (object[76].color.x == 0 && object[76].color.y == 0) && (object[77].color.x == 0 && object[77].color.z == 0) && (object[78].color.x == 0 && object[78].color.z == 0) && (object[79].color.y == 0 && object[79].color.z == 0) && (object[80].color.y == 0 && object[80].color.z == 0))
			{
				if (isSound[1] == false)
				{
					sound->SoundPlayWave(soundData2);
					isSound[1] = true;
				}

				object[68].texNumber = 18;
				SceneNum = End;
				isChange = false;//ステージ変更処理
				isLoad = false;//ロード
				LoadCount = 20;//ロードのウェイト
				StageNum = 0;//0はステージセレクト。
				MoveDirection = Right;//進行方向
				isLeft = false;//進行方向
				isRight = false;//進行方向
				isUp = false;//進行方向
				isDown = false;//進行方向
				isRota = false;//回転しているか
				timer = 0;//回転、移動処理の時間
				rotaX = 0;//今のXの向き正なら右に負なら左に回転している
				rotaY = 0;//今のYの向き正なら下に負なら上に回転している
			}
		}

		else if (StageNum == 6)
		{
		if ((object[75].color.x == 0 && object[75].color.y == 0) && (object[76].color.x == 0 && object[76].color.y == 0) && (object[77].color.x == 0 && object[77].color.z == 0) && (object[78].color.x == 0 && object[78].color.z == 0) && (object[79].color.y == 0 && object[79].color.z == 0) && (object[80].color.y == 0 && object[80].color.z == 0))
		{
			if (isSound[1] == false)
			{
				sound->SoundPlayWave(soundData2);
				isSound[1] = true;
			}

			object[69].texNumber = 19;
			SceneNum = End;
			isChange = false;//ステージ変更処理
			isLoad = false;//ロード
			LoadCount = 20;//ロードのウェイト
			StageNum = 0;//0はステージセレクト。
			MoveDirection = Right;//進行方向
			isLeft = false;//進行方向
			isRight = false;//進行方向
			isUp = false;//進行方向
			isDown = false;//進行方向
			isRota = false;//回転しているか
			timer = 0;//回転、移動処理の時間
			rotaX = 0;//今のXの向き正なら右に負なら左に回転している
			rotaY = 0;//今のYの向き正なら下に負なら上に回転している
		}
		}

		else if (StageNum == 7)
		{
		if (object[77].color.x == 1 && object[77].color.y == 1 && object[77].color.z == 1)
		{
			if ((object[75].color.y == 0 && object[75].color.z == 0) && (object[76].color.x == 0 && object[76].color.y == 0) && (object[78].color.x == 0 && object[78].color.z == 0) && (object[79].color.x == 0 && object[79].color.z == 0) && (object[80].color.x == 0 && object[80].color.y == 0))
			{
				if (isSound[1] == false)
				{
					sound->SoundPlayWave(soundData2);
					isSound[1] = true;
				}

				object[70].texNumber = 20;
				SceneNum = End;
				isChange = false;//ステージ変更処理
				isLoad = false;//ロード
				LoadCount = 20;//ロードのウェイト
				StageNum = 0;//0はステージセレクト。
				MoveDirection = Right;//進行方向
				isLeft = false;//進行方向
				isRight = false;//進行方向
				isUp = false;//進行方向
				isDown = false;//進行方向
				isRota = false;//回転しているか
				timer = 0;//回転、移動処理の時間
				rotaX = 0;//今のXの向き正なら右に負なら左に回転している
				rotaY = 0;//今のYの向き正なら下に負なら上に回転している
			}
		}
		}

		else if (StageNum == 8)
		{
		if ((object[75].color.x == 0 && object[75].color.y == 0) && (object[76].color.x == 0 && object[76].color.z == 0) && (object[77].color.y == 0 && object[77].color.z == 0) && (object[78].color.x == 0 && object[78].color.y == 0) && (object[79].color.y == 0 && object[79].color.z == 0) && (object[80].color.x == 0 && object[80].color.z == 0))
		{
			if (isSound[1] == false)
			{
				sound->SoundPlayWave(soundData2);
				isSound[1] = true;
			}

			object[71].texNumber = 21;
			SceneNum = End;
			isChange = false;//ステージ変更処理
			isLoad = false;//ロード
			LoadCount = 20;//ロードのウェイト
			StageNum = 0;//0はステージセレクト。
			MoveDirection = Right;//進行方向
			isLeft = false;//進行方向
			isRight = false;//進行方向
			isUp = false;//進行方向
			isDown = false;//進行方向
			isRota = false;//回転しているか
			timer = 0;//回転、移動処理の時間
			rotaX = 0;//今のXの向き正なら右に負なら左に回転している
			rotaY = 0;//今のYの向き正なら下に負なら上に回転している
		}
		}

		else
		{
			//ここに処理を入力
		}

		//スプライトの更新
		for (int i = 0; i < s_count; i++)
		{
			SpriteUpdate(sprite[i], spriteCommon);
		}

		//オブジェクトの更新
		for (int i = 0; i < o_count; i++)
		{
			ObjectUpdate(object[i], objectCommon);
		}

		//更新処理ここまで

		DxIni->BeforeDraw();//描画開始

		//描画処理ここから
		screen->Draw();

		ID3D12GraphicsCommandList* cmdList = DxIni->GetCmdList();//コマンドリストの取得
		SpriteCommonBeginDraw(spriteCommon, cmdList);//スプライト共通コマンド

		//スプライト描画

		//タイトルor背景、UI、HUD
		if (SceneNum == Title)
		{
			SpriteDraw(sprite[0], cmdList, spriteCommon, DxIni->GetDev());
		}

		else if (SceneNum == End)
		{
			SpriteDraw(sprite[6], cmdList, spriteCommon, DxIni->GetDev());
		}

		else
		{
			SpriteDraw(sprite[1], cmdList, spriteCommon, DxIni->GetDev());

			if (MoveDirection == Left)
			{
				SpriteDraw(sprite[2], cmdList, spriteCommon, DxIni->GetDev());
			}

			else if (MoveDirection == Right)
			{
				SpriteDraw(sprite[3], cmdList, spriteCommon, DxIni->GetDev());
			}

			else if (MoveDirection == Up)
			{
				SpriteDraw(sprite[4], cmdList, spriteCommon, DxIni->GetDev());
			}

			else
			{
				SpriteDraw(sprite[5], cmdList, spriteCommon, DxIni->GetDev());
			}
		}

		ObjectCommonBeginDraw(objectCommon, cmdList);//オブジェクト共通コマンド

		//変更箇所ここから
		//オブジェクト描画
		if (SceneNum == Game)
		{
			for (int i = 1; i < MaxBlock; i++)
			{
				ObjectDraw(object[i], cmdList, objectCommon, DxIni->GetDev());
			}

			for (int i = 75; i < 81; i++)
			{
				ObjectDraw(object[i], cmdList, objectCommon, DxIni->GetDev());
			}

			if (StageNum > 0)
			{
				for (int i = 81; i < o_count; i++)
				{
					ObjectDraw(object[i], cmdList, objectCommon, DxIni->GetDev());
				}
			}
		}
		//変更箇所ここまで

		//描画処理ここまで

		DxIni->AfterDraw();//描画終了

		//ESCで強制終了
		if (input->IsKey(DIK_ESCAPE))
		{
			break;
		}
	}

	//各種解放
	delete(DxIni);
	delete(input);
	delete(sound);
	delete(screen);
	WinIni->DeleteWIN();//ウィンドウの破棄
	delete(WinIni);

	return 0;
}

//3Dオブジェクト用パイプライン生成
PipelineSet Object3dCreateGraphicsPipeline(ID3D12Device* dev)
{
	HRESULT result = S_FALSE;

	ComPtr<ID3DBlob> vsBlob;//頂点シェーダオブジェクト
	ComPtr<ID3DBlob>psBlob;//ピクセルシェーダオブジェクト
	ComPtr<ID3DBlob>errorBlob;//エラーオブジェクト

	//頂点シェーダーの読み込みとコンパイル
	result = D3DCompileFromFile(
		L"Resource/shader/BasicVS.hlsl",				//シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,				//インクルード可能にする
		"main", "vs_5_0",								//エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//デバッグ用設定
		0,
		&vsBlob, &errorBlob);

	//エラー表示
	if (FAILED(result))
	{
		//errorBlobからエラー内容をstring型にコピー
		string errstr;
		errstr.resize(errorBlob->GetBufferSize());

		copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
		errstr += "\n";

		//エラー内容を出力ウィンドウに表示
		OutputDebugStringA(errstr.c_str());
		exit(1);
	}

	//ピクセルシェーダの読み込みとコンパイル
	result = D3DCompileFromFile(
		L"Resource/shader/BasicPS.hlsl",				//シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,				//インクルード可能にする
		"main", "ps_5_0",								//エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//デバッグ用設定
		0,
		&psBlob, &errorBlob);

	//エラー表示
	if (FAILED(result))
	{
		//errorBlobからエラー内容をstring型にコピー
		string errstr;
		errstr.resize(errorBlob->GetBufferSize());

		copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
		errstr += "\n";

		//エラー内容を出力ウィンドウに表示
		OutputDebugStringA(errstr.c_str());
		exit(1);
	}

	//頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{
			"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{
			"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		}
	};//1行で書いたほうが見やすい

	//グラフィックスパイプライン設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};

	//頂点シェーダー、ピクセルシェーダ
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

	//標準的な設定(背面カリング、塗りつぶし、深度クリッピング有効)
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	//ラスタライザステート
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//標準設定

	//レンダ―ターゲットのブレンド設定
	D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = gpipeline.BlendState.RenderTarget[0];
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;//標準設定
	blenddesc.BlendEnable = true;//ブレンドを有効にする
	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;//加算
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;//ソースの値を100%使う
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;//デストの値を0%使う

	//半透明合成
	blenddesc.BlendOp = D3D12_BLEND_OP_ADD;//加算
	blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;//ソースのアルファ値
	blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//1.0f-ソースのアルファ値
	gpipeline.InputLayout.pInputElementDescs = inputLayout;
	gpipeline.InputLayout.NumElements = _countof(inputLayout);
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpipeline.NumRenderTargets = 1;//描画対象は1つ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～255指定のRGBA
	gpipeline.SampleDesc.Count = 1;//1ピクセルにつき1回サンプリング

	//デプスステンシルステートの設定
	//標準的な設定(深度テストを行う、書き込み許可、深度が小さければ合格)
	gpipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//デスクリプタテーブルの設定
	CD3DX12_DESCRIPTOR_RANGE descRangeSRV;
	descRangeSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);//t0レジスタ

	//ルートパラメータの設定
	CD3DX12_ROOT_PARAMETER rootparams[2];
	rootparams[0].InitAsConstantBufferView(0);//定数バッファビューとして初期化(b0レジスタ)
	rootparams[1].InitAsDescriptorTable(1, &descRangeSRV);//テクスチャ用

	//テクスチャサンプラーの設定
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0);

	PipelineSet pipelineSet;

	//ルートシグネチャの設定
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Init_1_0(_countof(rootparams), rootparams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> rootSigBlob;

	//バージョン自動判定でのシリアライズ
	result = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);

	//ルートシグネチャの生成
	result = dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&pipelineSet.rootsignature));

	//パイプラインにルートシグネチャをセット
	gpipeline.pRootSignature = pipelineSet.rootsignature.Get();

	//パイプラインステートの生成
	result = dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&pipelineSet.pipelinestate));

	return pipelineSet;
}

//スプライト用パイプライン生成
PipelineSet SpriteCreateGraphicsPipeline(ID3D12Device* dev)
{
	HRESULT result = S_FALSE;

	ComPtr<ID3DBlob> vsBlob;//頂点シェーダオブジェクト
	ComPtr<ID3DBlob>psBlob;//ピクセルシェーダオブジェクト
	ComPtr<ID3DBlob>errorBlob;//エラーオブジェクト

	//頂点シェーダーの読み込みとコンパイル
	result = D3DCompileFromFile(
		L"Resource/shader/SpriteVS.hlsl",				//シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,				//インクルード可能にする
		"main", "vs_5_0",								//エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//デバッグ用設定
		0,
		&vsBlob, &errorBlob);

	//エラー表示
	if (FAILED(result))
	{
		//errorBlobからエラー内容をstring型にコピー
		string errstr;
		errstr.resize(errorBlob->GetBufferSize());

		copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
		errstr += "\n";

		//エラー内容を出力ウィンドウに表示
		OutputDebugStringA(errstr.c_str());
		exit(1);
	}

	//ピクセルシェーダの読み込みとコンパイル
	result = D3DCompileFromFile(
		L"Resource/shader/SpritePS.hlsl",				//シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,				//インクルード可能にする
		"main", "ps_5_0",								//エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//デバッグ用設定
		0,
		&psBlob, &errorBlob);

	//エラー表示
	if (FAILED(result))
	{
		//errorBlobからエラー内容をstring型にコピー
		string errstr;
		errstr.resize(errorBlob->GetBufferSize());

		copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
		errstr += "\n";

		//エラー内容を出力ウィンドウに表示
		OutputDebugStringA(errstr.c_str());
		exit(1);
	}

	//頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{
			"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		}
	};//1行で書いたほうが見やすい

	//グラフィックスパイプライン設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};

	//頂点シェーダー、ピクセルシェーダ
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

	//標準的な設定(背面カリング、塗りつぶし、深度クリッピング有効)
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);//一旦標準値をリセット
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//背面カリングなし

	//ラスタライザステート
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//標準設定

	//レンダ―ターゲットのブレンド設定
	D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = gpipeline.BlendState.RenderTarget[0];
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;//標準設定
	blenddesc.BlendEnable = true;//ブレンドを有効にする
	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;//加算
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;//ソースの値を100%使う
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;//デストの値を0%使う

	//半透明合成
	blenddesc.BlendOp = D3D12_BLEND_OP_ADD;//加算
	blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;//ソースのアルファ値
	blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//1.0f-ソースのアルファ値
	gpipeline.InputLayout.pInputElementDescs = inputLayout;
	gpipeline.InputLayout.NumElements = _countof(inputLayout);
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpipeline.NumRenderTargets = 1;//描画対象は1つ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～255指定のRGBA
	gpipeline.SampleDesc.Count = 1;//1ピクセルにつき1回サンプリング

	//デプスステンシルステートの設定
	//標準的な設定(深度テストを行う、書き込み許可、深度が小さければ合格)
	gpipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);//一旦標準値をリセット
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;//常に上書きルール
	gpipeline.DepthStencilState.DepthEnable = false;//深度テストをしない
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//デスクリプタテーブルの設定
	CD3DX12_DESCRIPTOR_RANGE descRangeSRV;
	descRangeSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);//t0レジスタ

	//ルートパラメータの設定
	CD3DX12_ROOT_PARAMETER rootparams[2];
	rootparams[0].InitAsConstantBufferView(0);//定数バッファビューとして初期化(b0レジスタ)
	rootparams[1].InitAsDescriptorTable(1, &descRangeSRV);//テクスチャ用

	//テクスチャサンプラーの設定
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0);

	PipelineSet pipelineSet;

	//ルートシグネチャの設定
	ComPtr<ID3D12RootSignature> rootsignature;
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Init_1_0(_countof(rootparams), rootparams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> rootSigBlob;

	//バージョン自動判定でのシリアライズ
	result = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);

	//ルートシグネチャの生成
	result = dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&pipelineSet.rootsignature));

	//パイプラインにルートシグネチャをセット
	gpipeline.pRootSignature = pipelineSet.rootsignature.Get();

	//パイプラインステートの生成
	ComPtr<ID3D12PipelineState> pipelinestate;
	result = dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&pipelineSet.pipelinestate));

	return pipelineSet;
}

//スプライト作成
Sprite SpriteCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT, UINT texNumber, const SpriteCommon& spriteCommon, XMFLOAT2 anchorpoint, bool isFilpX, bool isFilpY)
{
	HRESULT result = S_FALSE;

	//新しいスプライトを作る
	Sprite sprite{};

	//頂点データ
	VertexPosUv vertices[] =
	{
		{{0.0f, 100.0f, 0.0f}, {0.0f, 1.0f}},	//左下
		{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},		//左上
		{{100.0f, 100.0f, 0.0f}, {1.0f, 1.0f}},	//右下
		{{100.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},	//右上
	};

	//テクスチャ番号をコピー
	sprite.texNumber = texNumber;

	//頂点バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&sprite.vertBuff));

	//指定番号が読み込み済みなら
	if (spriteCommon.texBuff[sprite.texNumber])
	{
		//テクスチャ番号取得
		D3D12_RESOURCE_DESC resDesc = spriteCommon.texBuff[sprite.texNumber]->GetDesc();

		//スプライトの大きさを画像の解像度に合わせる
		sprite.size = { (float)resDesc.Width, (float)resDesc.Height };
	}

	//アンカーポイントをコピー
	sprite.anchorpoint = anchorpoint;

	//反転フラグをコピー
	sprite.isFilpX = isFilpX;
	sprite.isFilpY = isFilpY;

	//頂点バッファの転送
	SpriteTransferVertexBuffer(sprite, spriteCommon);

	//頂点バッファへのデータ転送
	VertexPosUv* vertMap = nullptr;
	result = sprite.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	sprite.vertBuff->Unmap(0, nullptr);

	//頂点バッファビューの作成
	sprite.vbView.BufferLocation = sprite.vertBuff->GetGPUVirtualAddress();
	sprite.vbView.SizeInBytes = sizeof(vertices);
	sprite.vbView.StrideInBytes = sizeof(vertices[0]);

	//定数バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&sprite.constBuff)
	);

	//定数バッファ転送
	ConstBufferData* constMap = nullptr;
	result = sprite.constBuff->Map(0, nullptr, (void**)&constMap);
	constMap->color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	//平行投影行列
	constMap->mat = XMMatrixOrthographicOffCenterLH(
		0.0f, WIN_WIDTH, WIN_HEIGHT, 0.0f, 0.0f, 1.0f);
	sprite.constBuff->Unmap(0, nullptr);

	return sprite;
}

//スプライト共通グラフィックコマンドのセット
void SpriteCommonBeginDraw(const SpriteCommon& spriteCommon, ID3D12GraphicsCommandList* cmdList)
{
	//パイプラインとルートシグネチャの設定
	cmdList->SetPipelineState(spriteCommon.pipelineSet.pipelinestate.Get());
	cmdList->SetGraphicsRootSignature(spriteCommon.pipelineSet.rootsignature.Get());

	//プリミティブ形状を設定
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//デスクリプタヒープをセット
	ID3D12DescriptorHeap* ppHeaps[] = { spriteCommon.descHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
}

//スプライト単体表示
void SpriteDraw(const Sprite& sprite, ID3D12GraphicsCommandList* cmdList, const SpriteCommon& spriteCommon, ID3D12Device* dev)
{
	//非表示フラグがtrueなら
	if (sprite.isInvisible)
	{
		//描画せず抜ける
		return;
	}

	//頂点バッファをセット
	cmdList->IASetVertexBuffers(0, 1, &sprite.vbView);

	//頂点バッファをセット
	cmdList->SetGraphicsRootConstantBufferView(0, sprite.constBuff->GetGPUVirtualAddress());

	//シェーダリソースビューをセット
	cmdList->SetGraphicsRootDescriptorTable(1,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(
			spriteCommon.descHeap->GetGPUDescriptorHandleForHeapStart(),
			sprite.texNumber,
			dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));

	//ポリゴンの描画（４頂点で四角形）
	cmdList->DrawInstanced(4, 1, 0, 0);
}

//スプライト共通データ生成
SpriteCommon SpriteCommonCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT)
{
	HRESULT result = S_FALSE;

	//新たなスプライト共通データを作成
	SpriteCommon spriteCommon{};

	//スプライト用パイプライン生成
	spriteCommon.pipelineSet = SpriteCreateGraphicsPipeline(dev);

	//並行行列の射影行列生成
	spriteCommon.matProjection = XMMatrixOrthographicOffCenterLH(
		0.0f, (float)WIN_WIDTH, (float)WIN_HEIGHT, 0.0f, 0.0f, 1.0f);

	//デスクリプタヒープを生成
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NumDescriptors = spriteSRVCount;
	result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&spriteCommon.descHeap));

	return spriteCommon;
}

//スプライト単体更新
void SpriteUpdate(Sprite& sprite, const SpriteCommon& spriteCommon)
{
	//ワールド行列の更新
	sprite.matWorld = XMMatrixIdentity();

	//Z軸回転
	sprite.matWorld *= XMMatrixRotationZ(XMConvertToRadians(sprite.rotation));

	//平行移動
	sprite.matWorld *= XMMatrixTranslation(sprite.position.x, sprite.position.y, sprite.position.z);

	//定数バッファの転送
	ConstBufferData* constMap = nullptr;
	HRESULT result = sprite.constBuff->Map(0, nullptr, (void**)&constMap);
	constMap->mat = sprite.matWorld * spriteCommon.matProjection;
	constMap->color = sprite.color;
	sprite.constBuff->Unmap(0, nullptr);
}

//スプライト共通テクスチャ読み込み
void SpriteCommonLoadTexture(SpriteCommon& spriteCommon, UINT texNumber, const wchar_t* filename, ID3D12Device* dev)
{
	//異常な番号の指定を検出
	assert(texNumber <= spriteSRVCount - 1);

	HRESULT result = S_FALSE;

	//WICテクスチャのロード
	TexMetadata metadata{};
	ScratchImage scratchIng{};

	result = LoadFromWICFile(
		filename,
		WIC_FLAGS_NONE,
		&metadata, scratchIng);

	const Image* img = scratchIng.GetImage(0, 0, 0);

	//リソース設定
	CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		metadata.width,
		(UINT)metadata.height,
		(UINT16)metadata.arraySize,
		(UINT16)metadata.mipLevels);

	//テクスチャバッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0),
		D3D12_HEAP_FLAG_NONE,
		&texresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&spriteCommon.texBuff[texNumber])
	);

	//テクスチャバッファにデータ転送
	result = spriteCommon.texBuff[texNumber]->WriteToSubresource(
		0,
		nullptr,				//全領域にコピー
		img->pixels,			//元データアドレス
		(UINT)img->rowPitch,	//1ラインサイズ
		(UINT)img->slicePitch	//全ラインサイズ
	);

	//シェーダリソースビュー設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};//設定構造体
	srvDesc.Format = metadata.format;//RGBA
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;

	//ヒープのtexnumber番目にシェーダリソースビュー作成
	dev->CreateShaderResourceView(
		spriteCommon.texBuff[texNumber].Get(),	//ビューと関連付けるバッファ
		&srvDesc,								//テクスチャ設定情報
		CD3DX12_CPU_DESCRIPTOR_HANDLE(spriteCommon.descHeap->GetCPUDescriptorHandleForHeapStart(), texNumber,
			dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		)
	);
}

//スプライト単体頂点バッファの転送
void SpriteTransferVertexBuffer(const Sprite& sprite, const SpriteCommon& spriteCommon)
{
	HRESULT result = S_FALSE;

	//頂点データ
	VertexPosUv vertices[] =
	{
		{{}, {0.0f, 1.0f}},//左下
		{{}, {0.0f, 0.0f}},//左上
		{{}, {1.0f, 1.0f}},//右下
		{{}, {1.0f, 0.0f}},//右上
	};

	//左下、左上、右下、右上
	enum { LB, LT, RB, RT };

	float left = (0.0f - sprite.anchorpoint.x) * sprite.size.x;
	float right = (1.0f - sprite.anchorpoint.x) * sprite.size.x;
	float top = (0.0f - sprite.anchorpoint.y) * sprite.size.y;
	float bottom = (1.0f - sprite.anchorpoint.y) * sprite.size.y;

	if (sprite.isFilpX)
	{
		left = -left;
		right = -right;
	}

	if (sprite.isFilpY)
	{
		top = -top;
		bottom = -bottom;
	}

	if (spriteCommon.texBuff[sprite.texNumber])
	{
		//テクスチャ情報取得
		D3D12_RESOURCE_DESC resDesc = spriteCommon.texBuff[sprite.texNumber]->GetDesc();

		float tex_left = sprite.texLeftTop.x / resDesc.Width;
		float tex_right = (sprite.texLeftTop.x + sprite.texSize.x) / resDesc.Width;
		float tex_top = sprite.texLeftTop.y / resDesc.Height;
		float tex_bottom = (sprite.texLeftTop.y + sprite.texSize.y) / resDesc.Height;

		vertices[LB].uv = { tex_left, tex_bottom };
		vertices[LT].uv = { tex_left, tex_top };
		vertices[RB].uv = { tex_right, tex_bottom };
		vertices[RT].uv = { tex_right, tex_top };
	}

	vertices[LB].pos = { left, bottom, 0.0f }; //左下
	vertices[LT].pos = { left, top, 0.0f }; //左上
	vertices[RB].pos = { right, bottom, 0.0f }; //右下
	vertices[RT].pos = { right, top, 0.0f }; //右上

	//頂点バッファへのデータ転送
	VertexPosUv* vertMap = nullptr;
	result = sprite.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	sprite.vertBuff->Unmap(0, nullptr);
}

//オブジェクト作成
Object objectCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT, UINT texNumber)
{
	HRESULT result = S_FALSE;

	Object object{};

	Vertex vertices[] =
	{
		//前
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},	//左下0
		{{-object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 0.0f}},	//左上1
		{{object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},	//右下2
		{{object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 0.0f}},		//右上3

		//後
		{{object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 0.0f}},		//右下
		{{object.size.x, object.size.y, object.size.z}, {}, {0.0f, 1.0f}},		//右上
		{{-object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 0.0f}},	//左下
		{{-object.size.x, object.size.y, object.size.z}, {}, {1.0f, 1.0f}},		//左上

		//左
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},	//左下
		{{-object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 0.0f}},	//左上
		{{-object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},	//右下
		{{-object.size.x, object.size.y, object.size.z}, {}, {1.0f, 0.0f}},		//右上

		//右
		{{object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},		//右下
		{{object.size.x, object.size.y, object.size.z}, {}, {0.0f, 0.0f}},		//右上
		{{object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},	//左下
		{{object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 0.0f}},		//左上

		//上
		{{-object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},	//左下
		{{-object.size.x, object.size.y, object.size.z}, {}, {0.0f, 0.0f}},		//左上
		{{object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},		//右下
		{{object.size.x, object.size.y, object.size.z}, {}, {1.0f, 0.0f}},		//右上

		//下
		{{object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 0.0f}},	//右下
		{{object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 1.0f}},		//右上
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 0.0f}},	//左下
		{{-object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 1.0f}},	//左上
	};

	unsigned short indices[] =
	{
		0, 1, 2, //三角形1つ目
		2, 1, 3, //三角形2つ目

		//後
		4, 5, 6, //三角形1つ目
		6, 5, 7, //三角形2つ目

		//左
		8, 9, 10, //三角形1つ目
		10, 9, 11,//三角形2つ目

		//右
		12, 13, 14, //三角形1つ目
		14, 13, 15, //三角形2つ目

		//上
		16, 17, 18, //三角形1つ目
		18, 17, 19, //三角形2つ目

		//下
		20, 21, 22, //三角形1つ目
		22, 21, 23, //三角形2つ目
	};

	for (int i = 0; i < _countof(indices) / 3; i++)
	{//三角形1つ毎に計算していく
		//三角形のインデックスを取り出して、一時的な変数に入れる
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];

		//三角形を構成する頂点座標をベクトルに代入
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);

		//p0→piベクトル、p0→p2ベクトルを計算 (ベクトルの減算)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);

		// 外積は両面から垂直なベクトル
		XMVECTOR normal = XMVector3Cross(v1, v2);

		// 正規化 (長さを1にする)
		normal = XMVector3Normalize(normal);

		//求めた法線を頂点データに代入
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	object.texNumber = texNumber;

	//頂点バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.vertBuff));

	//頂点バッファへのデータ転送
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);

	//頂点バッファビューの作成
	object.vbView.BufferLocation = object.vertBuff->GetGPUVirtualAddress();
	object.vbView.SizeInBytes = sizeof(vertices);
	object.vbView.StrideInBytes = sizeof(vertices[0]);

	//インデックスバッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.indexBuff));

	//インデックスバッファのデータ転送
	unsigned short* indexMap = nullptr;
	result = object.indexBuff->Map(0, nullptr, (void**)&indexMap);
	memcpy(indexMap, indices, sizeof(indices));
	object.indexBuff->Unmap(0, nullptr);

	object.ibView.BufferLocation = object.indexBuff->GetGPUVirtualAddress();
	object.ibView.Format = DXGI_FORMAT_R16_UINT;
	object.ibView.SizeInBytes = sizeof(indices);

	//定数バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.constBuff)
	);

	//定数バッファ転送
	ConstBufferData* constMap = nullptr;
	result = object.constBuff->Map(0, nullptr, (void**)&constMap);

	//ワールド行列の更新
	object.matWorld = XMMatrixIdentity();//単位行列

	//拡大行列
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);

	//回転行列
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));

	//平行移動行列
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//ビューの変換行列
	object.matView = XMMatrixLookAtLH(XMLoadFloat3(&object.eye), XMLoadFloat3(&object.target), XMLoadFloat3(&object.up));

	constMap->mat = object.matWorld * object.matView;
	constMap->color = object.color;

	object.constBuff->Unmap(0, nullptr);

	return object;
}

Object objectCreateFlont(ID3D12Device* dev, int window_width, int window_height, UINT texNumber)
{
	HRESULT result = S_FALSE;

	Object object{};

	Vertex vertices[] =
	{
		//前
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//左下0
		{{-object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 0.0f}},//左上 1
		{{object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//右下 2
		{{object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 0.0f}},//右上 3
	};

	unsigned short indices[] =
	{
		0, 1, 2, //三角形1つ目
		2, 1, 3, //三角形2つ目
	};

	for (int i = 0; i < _countof(indices) / 3; i++)
	{// 三角形1つ毎に計算していく
		// 三角形のインデックスを取り出して、一時的な変数に入れる
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];
		//三角形を構成する頂点座標をベクトルに代入
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
		//p0→piベクトル、p0→p2ベクトルを計算 (ベクトルの減算)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);
		// 外積は両面から垂直なベクトル
		XMVECTOR normal = XMVector3Cross(v1, v2);
		// 正規化 (長さを1にする)
		normal = XMVector3Normalize(normal);
		// 求めた法線を頂点データに代入
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	object.texNumber = texNumber;

	//頂点バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.vertBuff));

	// 頂点バッファへのデータ転送
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);

	// 頂点バッファビューの作成
	object.vbView.BufferLocation = object.vertBuff->GetGPUVirtualAddress();
	object.vbView.SizeInBytes = sizeof(vertices);
	object.vbView.StrideInBytes = sizeof(vertices[0]);

	//インデックスバッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.indexBuff));

	//インデックスバッファのデータ転送
	unsigned short* indexMap = nullptr;
	result = object.indexBuff->Map(0, nullptr, (void**)&indexMap);
	memcpy(indexMap, indices, sizeof(indices));
	object.indexBuff->Unmap(0, nullptr);

	object.ibView.BufferLocation = object.indexBuff->GetGPUVirtualAddress();
	object.ibView.Format = DXGI_FORMAT_R16_UINT;
	object.ibView.SizeInBytes = sizeof(indices);

	//定数バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.constBuff)
	);

	//定数バッファ転送
	ConstBufferData* constMap = nullptr;
	result = object.constBuff->Map(0, nullptr, (void**)&constMap);

	//ワールド行列の更新
	object.matWorld = XMMatrixIdentity(); //単位行列
	//拡大行列
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);
	//回転行列
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));
	//平行移動行列
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//ビューの変換行列
	object.matView = XMMatrixLookAtLH(XMLoadFloat3(&object.eye), XMLoadFloat3(&object.target), XMLoadFloat3(&object.up));

	constMap->mat = object.matWorld * object.matView;
	constMap->color = object.color;

	object.constBuff->Unmap(0, nullptr);

	return object;
}

Object objectCreateBack(ID3D12Device* dev, int window_width, int window_height, UINT texNumber)
{
	HRESULT result = S_FALSE;

	Object object{};

	Vertex vertices[] =
	{
		//後
		{{object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//右下
		{{object.size.x, object.size.y, object.size.z}, {}, {0.0f, 1.0f}},//右上
		{{-object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//左下
		{{-object.size.x, object.size.y, object.size.z}, {}, {1.0f, 1.0f}},//左上
	};

	unsigned short indices[] =
	{
		0, 1, 2, //三角形1つ目
		2, 1, 3, //三角形2つ目
	};

	for (int i = 0; i < _countof(indices) / 3; i++)
	{// 三角形1つ毎に計算していく
		// 三角形のインデックスを取り出して、一時的な変数に入れる
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];
		//三角形を構成する頂点座標をベクトルに代入
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
		//p0→piベクトル、p0→p2ベクトルを計算 (ベクトルの減算)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);
		// 外積は両面から垂直なベクトル
		XMVECTOR normal = XMVector3Cross(v1, v2);
		// 正規化 (長さを1にする)
		normal = XMVector3Normalize(normal);
		// 求めた法線を頂点データに代入
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	object.texNumber = texNumber;

	//頂点バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.vertBuff));

	// 頂点バッファへのデータ転送
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);

	// 頂点バッファビューの作成
	object.vbView.BufferLocation = object.vertBuff->GetGPUVirtualAddress();
	object.vbView.SizeInBytes = sizeof(vertices);
	object.vbView.StrideInBytes = sizeof(vertices[0]);

	//インデックスバッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.indexBuff));

	//インデックスバッファのデータ転送
	unsigned short* indexMap = nullptr;
	result = object.indexBuff->Map(0, nullptr, (void**)&indexMap);
	memcpy(indexMap, indices, sizeof(indices));
	object.indexBuff->Unmap(0, nullptr);

	object.ibView.BufferLocation = object.indexBuff->GetGPUVirtualAddress();
	object.ibView.Format = DXGI_FORMAT_R16_UINT;
	object.ibView.SizeInBytes = sizeof(indices);

	//定数バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.constBuff)
	);

	//定数バッファ転送
	ConstBufferData* constMap = nullptr;
	result = object.constBuff->Map(0, nullptr, (void**)&constMap);

	//ワールド行列の更新
	object.matWorld = XMMatrixIdentity(); //単位行列
	//拡大行列
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);
	//回転行列
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));
	//平行移動行列
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//ビューの変換行列
	object.matView = XMMatrixLookAtLH(XMLoadFloat3(&object.eye), XMLoadFloat3(&object.target), XMLoadFloat3(&object.up));

	constMap->mat = object.matWorld * object.matView;
	constMap->color = object.color;

	object.constBuff->Unmap(0, nullptr);

	return object;
}

Object objectCreateLeft(ID3D12Device* dev, int window_width, int window_height, UINT texNumber)
{
	HRESULT result = S_FALSE;

	Object object{};

	Vertex vertices[] =
	{
		//左
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//左下
		{{-object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//左上
		{{-object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//右下
		{{-object.size.x, object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//右上
	};

	unsigned short indices[] =
	{
		0, 1, 2, //三角形1つ目
		2, 1, 3, //三角形2つ目
	};

	for (int i = 0; i < _countof(indices) / 3; i++)
	{// 三角形1つ毎に計算していく
		// 三角形のインデックスを取り出して、一時的な変数に入れる
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];
		//三角形を構成する頂点座標をベクトルに代入
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
		//p0→piベクトル、p0→p2ベクトルを計算 (ベクトルの減算)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);
		// 外積は両面から垂直なベクトル
		XMVECTOR normal = XMVector3Cross(v1, v2);
		// 正規化 (長さを1にする)
		normal = XMVector3Normalize(normal);
		// 求めた法線を頂点データに代入
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	object.texNumber = texNumber;

	//頂点バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.vertBuff));

	// 頂点バッファへのデータ転送
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);

	// 頂点バッファビューの作成
	object.vbView.BufferLocation = object.vertBuff->GetGPUVirtualAddress();
	object.vbView.SizeInBytes = sizeof(vertices);
	object.vbView.StrideInBytes = sizeof(vertices[0]);

	//インデックスバッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.indexBuff));

	//インデックスバッファのデータ転送
	unsigned short* indexMap = nullptr;
	result = object.indexBuff->Map(0, nullptr, (void**)&indexMap);
	memcpy(indexMap, indices, sizeof(indices));
	object.indexBuff->Unmap(0, nullptr);

	object.ibView.BufferLocation = object.indexBuff->GetGPUVirtualAddress();
	object.ibView.Format = DXGI_FORMAT_R16_UINT;
	object.ibView.SizeInBytes = sizeof(indices);

	//定数バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.constBuff)
	);

	//定数バッファ転送
	ConstBufferData* constMap = nullptr;
	result = object.constBuff->Map(0, nullptr, (void**)&constMap);

	//ワールド行列の更新
	object.matWorld = XMMatrixIdentity(); //単位行列
	//拡大行列
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);
	//回転行列
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));
	//平行移動行列
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//ビューの変換行列
	object.matView = XMMatrixLookAtLH(XMLoadFloat3(&object.eye), XMLoadFloat3(&object.target), XMLoadFloat3(&object.up));

	constMap->mat = object.matWorld * object.matView;
	constMap->color = object.color;

	object.constBuff->Unmap(0, nullptr);

	return object;
}

Object objectCreateRight(ID3D12Device* dev, int window_width, int window_height, UINT texNumber)
{
	HRESULT result = S_FALSE;

	Object object{};

	Vertex vertices[] =
	{
		//右
		{{object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//右下
		{{object.size.x, object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//右上
		{{object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//左下
		{{object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//左上
	};

	unsigned short indices[] =
	{
		0, 1, 2, //三角形1つ目
		2, 1, 3, //三角形2つ目
	};

	for (int i = 0; i < _countof(indices) / 3; i++)
	{// 三角形1つ毎に計算していく
		// 三角形のインデックスを取り出して、一時的な変数に入れる
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];
		//三角形を構成する頂点座標をベクトルに代入
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
		//p0→piベクトル、p0→p2ベクトルを計算 (ベクトルの減算)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);
		// 外積は両面から垂直なベクトル
		XMVECTOR normal = XMVector3Cross(v1, v2);
		// 正規化 (長さを1にする)
		normal = XMVector3Normalize(normal);
		// 求めた法線を頂点データに代入
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	object.texNumber = texNumber;

	//頂点バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.vertBuff));

	// 頂点バッファへのデータ転送
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);

	// 頂点バッファビューの作成
	object.vbView.BufferLocation = object.vertBuff->GetGPUVirtualAddress();
	object.vbView.SizeInBytes = sizeof(vertices);
	object.vbView.StrideInBytes = sizeof(vertices[0]);

	//インデックスバッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.indexBuff));

	//インデックスバッファのデータ転送
	unsigned short* indexMap = nullptr;
	result = object.indexBuff->Map(0, nullptr, (void**)&indexMap);
	memcpy(indexMap, indices, sizeof(indices));
	object.indexBuff->Unmap(0, nullptr);

	object.ibView.BufferLocation = object.indexBuff->GetGPUVirtualAddress();
	object.ibView.Format = DXGI_FORMAT_R16_UINT;
	object.ibView.SizeInBytes = sizeof(indices);

	//定数バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.constBuff)
	);

	//定数バッファ転送
	ConstBufferData* constMap = nullptr;
	result = object.constBuff->Map(0, nullptr, (void**)&constMap);

	//ワールド行列の更新
	object.matWorld = XMMatrixIdentity(); //単位行列
	//拡大行列
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);
	//回転行列
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));
	//平行移動行列
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//ビューの変換行列
	object.matView = XMMatrixLookAtLH(XMLoadFloat3(&object.eye), XMLoadFloat3(&object.target), XMLoadFloat3(&object.up));

	constMap->mat = object.matWorld * object.matView;
	constMap->color = object.color;

	object.constBuff->Unmap(0, nullptr);

	return object;
}

Object objectCreateUp(ID3D12Device* dev, int window_width, int window_height, UINT texNumber)
{
	HRESULT result = S_FALSE;

	Object object{};

	Vertex vertices[] =
	{
		//上
		{{-object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//左下
		{{-object.size.x, object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//左上
		{{object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//右下
		{{object.size.x, object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//右上
	};

	unsigned short indices[] =
	{
		0, 1, 2, //三角形1つ目
		2, 1, 3, //三角形2つ目
	};

	for (int i = 0; i < _countof(indices) / 3; i++)
	{// 三角形1つ毎に計算していく
		// 三角形のインデックスを取り出して、一時的な変数に入れる
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];
		//三角形を構成する頂点座標をベクトルに代入
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
		//p0→piベクトル、p0→p2ベクトルを計算 (ベクトルの減算)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);
		// 外積は両面から垂直なベクトル
		XMVECTOR normal = XMVector3Cross(v1, v2);
		// 正規化 (長さを1にする)
		normal = XMVector3Normalize(normal);
		// 求めた法線を頂点データに代入
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	object.texNumber = texNumber;

	//頂点バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.vertBuff));

	// 頂点バッファへのデータ転送
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);

	// 頂点バッファビューの作成
	object.vbView.BufferLocation = object.vertBuff->GetGPUVirtualAddress();
	object.vbView.SizeInBytes = sizeof(vertices);
	object.vbView.StrideInBytes = sizeof(vertices[0]);

	//インデックスバッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.indexBuff));

	//インデックスバッファのデータ転送
	unsigned short* indexMap = nullptr;
	result = object.indexBuff->Map(0, nullptr, (void**)&indexMap);
	memcpy(indexMap, indices, sizeof(indices));
	object.indexBuff->Unmap(0, nullptr);

	object.ibView.BufferLocation = object.indexBuff->GetGPUVirtualAddress();
	object.ibView.Format = DXGI_FORMAT_R16_UINT;
	object.ibView.SizeInBytes = sizeof(indices);

	//定数バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.constBuff)
	);

	//定数バッファ転送
	ConstBufferData* constMap = nullptr;
	result = object.constBuff->Map(0, nullptr, (void**)&constMap);

	//ワールド行列の更新
	object.matWorld = XMMatrixIdentity(); //単位行列
	//拡大行列
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);
	//回転行列
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));
	//平行移動行列
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//ビューの変換行列
	object.matView = XMMatrixLookAtLH(XMLoadFloat3(&object.eye), XMLoadFloat3(&object.target), XMLoadFloat3(&object.up));

	constMap->mat = object.matWorld * object.matView;
	constMap->color = object.color;

	object.constBuff->Unmap(0, nullptr);

	return object;
}

Object objectCreateDown(ID3D12Device* dev, int window_width, int window_height, UINT texNumber)
{
	HRESULT result = S_FALSE;

	Object object{};

	Vertex vertices[] =
	{
		//下
		{{object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 0.0f}},//右下
		{{object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 1.0f}},//右上
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 0.0f}},//左下
		{{-object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 1.0f}},//左上
	};

	unsigned short indices[] =
	{
		0, 1, 2, //三角形1つ目
		2, 1, 3, //三角形2つ目
	};

	for (int i = 0; i < _countof(indices) / 3; i++)
	{// 三角形1つ毎に計算していく
		// 三角形のインデックスを取り出して、一時的な変数に入れる
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];
		//三角形を構成する頂点座標をベクトルに代入
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
		//p0→piベクトル、p0→p2ベクトルを計算 (ベクトルの減算)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);
		// 外積は両面から垂直なベクトル
		XMVECTOR normal = XMVector3Cross(v1, v2);
		// 正規化 (長さを1にする)
		normal = XMVector3Normalize(normal);
		// 求めた法線を頂点データに代入
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	object.texNumber = texNumber;

	//頂点バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.vertBuff));

	// 頂点バッファへのデータ転送
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);

	// 頂点バッファビューの作成
	object.vbView.BufferLocation = object.vertBuff->GetGPUVirtualAddress();
	object.vbView.SizeInBytes = sizeof(vertices);
	object.vbView.StrideInBytes = sizeof(vertices[0]);

	//インデックスバッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.indexBuff));

	//インデックスバッファのデータ転送
	unsigned short* indexMap = nullptr;
	result = object.indexBuff->Map(0, nullptr, (void**)&indexMap);
	memcpy(indexMap, indices, sizeof(indices));
	object.indexBuff->Unmap(0, nullptr);

	object.ibView.BufferLocation = object.indexBuff->GetGPUVirtualAddress();
	object.ibView.Format = DXGI_FORMAT_R16_UINT;
	object.ibView.SizeInBytes = sizeof(indices);

	//定数バッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.constBuff)
	);

	//定数バッファ転送
	ConstBufferData* constMap = nullptr;
	result = object.constBuff->Map(0, nullptr, (void**)&constMap);

	//ワールド行列の更新
	object.matWorld = XMMatrixIdentity(); //単位行列
	//拡大行列
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);
	//回転行列
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));
	//平行移動行列
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//ビューの変換行列
	object.matView = XMMatrixLookAtLH(XMLoadFloat3(&object.eye), XMLoadFloat3(&object.target), XMLoadFloat3(&object.up));

	constMap->mat = object.matWorld * object.matView;
	constMap->color = object.color;

	object.constBuff->Unmap(0, nullptr);

	return object;
}

//オブジェクトグラフィックコマンドのセット
void ObjectCommonBeginDraw(const ObjectCommon& objectCommon, ID3D12GraphicsCommandList* cmdList)
{
	//パイプラインとルートシグネチャの設定
	cmdList->SetPipelineState(objectCommon.pipelineSet.pipelinestate.Get());
	cmdList->SetGraphicsRootSignature(objectCommon.pipelineSet.rootsignature.Get());

	//プリミティブ形状を設定
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//デスクリプタヒープをセット
	ID3D12DescriptorHeap* ppHeaps[] = { objectCommon.descHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
}

//オブジェクト表示
void ObjectDraw(const Object& object, ID3D12GraphicsCommandList* cmdList, const ObjectCommon& objectCommon, ID3D12Device* dev)
{
	//頂点バッファをセット
	cmdList->IASetVertexBuffers(0, 1, &object.vbView);

	cmdList->IASetIndexBuffer(&object.ibView);

	//定数バッファをセット
	cmdList->SetGraphicsRootConstantBufferView(0, object.constBuff->GetGPUVirtualAddress());


	//シェーダリソースビューをセット
	cmdList->SetGraphicsRootDescriptorTable(1,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(
			objectCommon.descHeap->GetGPUDescriptorHandleForHeapStart(),
			object.texNumber,
			dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));

	//ポリゴンの描画（４頂点で四角形）
	cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}

//オブジェクトデータ生成
ObjectCommon ObjectCommonCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT)
{
	HRESULT result = S_FALSE;

	//新たなスプライト生成共通データを生成
	ObjectCommon objectCommon{};

	//オブジェクト用パイプライン生成
	objectCommon.pipelineSet = Object3dCreateGraphicsPipeline(dev);

	//射影変換行列(透視投影)
	objectCommon.matProjection = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(60.0f),		//上下画角60度
		(float)WIN_WIDTH / WIN_HEIGHT,	//アスペクト比（画面横幅 / 画面縦幅）
		0.1f, 2000.0f					//前端、奥端
	);

	//デスクリプタヒープを生成
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NumDescriptors = objectCount;
	result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&objectCommon.descHeap));

	//生成したスプライト共通データを返す
	return objectCommon;
}

//オブジェクト更新
void ObjectUpdate(Object& object, const ObjectCommon& objectCommon)
{
	//ワールド行列の更新
	object.matWorld = XMMatrixIdentity();

	//拡大行列
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);

	//Z軸回転行列
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));

	//平行行列
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//ビューの変換行列
	object.matView = XMMatrixLookAtLH(XMLoadFloat3(&object.eye), XMLoadFloat3(&object.target), XMLoadFloat3(&object.up));

	//定数バッファの転送
	ConstBufferData* constMap = nullptr;
	HRESULT result = object.constBuff->Map(0, nullptr, (void**)&constMap);
	constMap->mat = object.matWorld * object.matView * objectCommon.matProjection;
	constMap->color = object.color;
	object.constBuff->Unmap(0, nullptr);
}

//オブジェクトテクスチャ読み込み
void ObjectCommonLoadTexture(ObjectCommon& objectCommon, UINT texNumber, const wchar_t* filename, ID3D12Device* dev)
{
	//異常な番号の指定を検出
	assert(texNumber <= spriteSRVCount - 1);

	HRESULT result = S_FALSE;

	//WICテクスチャのロード
	TexMetadata metadata{};
	ScratchImage scratchIng{};

	result = LoadFromWICFile(
		filename,
		WIC_FLAGS_NONE,
		&metadata, scratchIng);

	const Image* img = scratchIng.GetImage(0, 0, 0);

	//リソース設定
	CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		metadata.width,
		(UINT)metadata.height,
		(UINT16)metadata.arraySize,
		(UINT16)metadata.mipLevels);

	//テクスチャバッファの生成
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0),
		D3D12_HEAP_FLAG_NONE,
		&texresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&objectCommon.texBuff[texNumber])
	);

	//テクスチャバッファにデータ転送
	result = objectCommon.texBuff[texNumber]->WriteToSubresource(
		0,
		nullptr,				//全領域にコピー
		img->pixels,			//元データアドレス
		(UINT)img->rowPitch,	//1ラインサイズ
		(UINT)img->slicePitch	//全ラインサイズ
	);

	//シェーダリソースビュー設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};//設定構造体
	srvDesc.Format = metadata.format;//RGBA
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;

	//ヒープのtexnumber番目にシェーダリソースビュー作成
	dev->CreateShaderResourceView(
		objectCommon.texBuff[texNumber].Get(),//ビューと関連付けるバッファ
		&srvDesc,//テクスチャ設定情報
		CD3DX12_CPU_DESCRIPTOR_HANDLE(objectCommon.descHeap->GetCPUDescriptorHandleForHeapStart(), texNumber,
			dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		)
	);
}

//オブジェクト頂点バッファの転送
void ObjectTransferVertexBuffer(const Object& object, const ObjectCommon& ObjectCommon)
{
	HRESULT result = S_FALSE;

	Vertex vertices[] =
	{
		//前
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//左下0
		{{-object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 0.0f}},//左上 1
		{{object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//右下 2
		{{object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 0.0f}},//右上 3

		//後
		{{object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//右下
		{{object.size.x, object.size.y, object.size.z}, {}, {0.0f, 1.0f}},//右上
		{{-object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//左下
		{{-object.size.x, object.size.y, object.size.z}, {}, {1.0f, 1.0f}},//左上

		//左
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//左下
		{{-object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//左上
		{{-object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//右下
		{{-object.size.x, object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//右上

		//右
		{{object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//右下
		{{object.size.x, object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//右上
		{{object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//左下
		{{object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//左上

		//上
		{{-object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//左下
		{{-object.size.x, object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//左上
		{{object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//右下
		{{object.size.x, object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//右上

		//下
		{{object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 0.0f}},//右下
		{{object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 1.0f}},//右上
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 0.0f}},//左下
		{{-object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 1.0f}},//左上
	};

	// 頂点バッファへのデータ転送
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);
}