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

//�p�C�v���C���Z�b�g
struct PipelineSet
{
	ComPtr<ID3D12PipelineState> pipelinestate;//�p�C�v���C���X�e�[�g�I�u�W�F�N�g
	ComPtr<ID3D12RootSignature> rootsignature;//���[�g�V�O�l�`��
};

//�X�v���C�g1�����̃f�[�^
struct Sprite
{
	ComPtr<ID3D12Resource> vertBuff;//���_�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW vbView;//���_�o�b�t�@�r���[
	ComPtr<ID3D12Resource> constBuff;//�萔�o�b�t�@

	float rotation = 0.0f;//Z�����̉�]�p
	XMFLOAT3 position = { 0, 0, 0 };//���W
	XMMATRIX matWorld;//���[���h�s��
	XMFLOAT4 color = { 1, 1, 1, 1 };//�F
	UINT texNumber = 0;//�e�N�X�`���ԍ�
	XMFLOAT2 size = { 100, 100 };//�傫��
	XMFLOAT2 anchorpoint = { 0.5f, 0.5f };//�A���J�[�|�C���g
	bool isFilpX = false;//���E���]
	bool isFilpY = false;//�㉺���]
	XMFLOAT2 texLeftTop = { 0, 0 };//�e�N�X�`���̍�����W
	XMFLOAT2 texSize = { 100, 100 };//�e�N�X�`���̐؂�o���T�C�Y
	bool isInvisible = false;//��\��
};

//�I�u�W�F�N�g�̃f�[�^
struct Object
{
	ComPtr<ID3D12Resource> vertBuff;//���_�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW vbView;//���_�o�b�t�@�r���[
	ComPtr<ID3D12Resource> indexBuff;//�C���f�b�N�X�o�b�t�@
	D3D12_INDEX_BUFFER_VIEW ibView;//�C���f�b�N�X�o�b�t�@�r���[
	ComPtr<ID3D12Resource> constBuff;//�萔�o�b�t�@
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescHandleCBV;//�萔�o�b�t�@�r���[�̃n���h��(CPU)
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescHandleCBV;//�萔�o�b�t�@�r���[�̃n���h��(GPU)

	//�A�t�B���ϊ����
	XMFLOAT3 scale{ 1,1,1 };//�X�P�[�����O�{��

	//Z�����̉�]�p
	XMFLOAT3 rotation = { 0, 0, 0 };
	XMFLOAT3 position{ 0, 0, 0 };//���W

	XMMATRIX matWorld;//���[���h�ϊ��s��

	XMFLOAT4 color = { 1, 1, 1, 1 };//�F

	XMFLOAT3 size = { 10,10,10 };

	UINT texNumber = 0;

	XMMATRIX matView;

	XMFLOAT3 eye{ 0, 0, -100 };
	XMFLOAT3 target{ 0, 0, 0 };
	XMFLOAT3 up{ 0, 1, 0 };
	int number = 0;
};

//���_�f�[�^�\����
struct Vertex
{
	XMFLOAT3 pos;	//XYZ���W
	XMFLOAT3 normal;//�@���x�N�g��
	XMFLOAT2 uv;	//UV���W
};

//�萔�o�b�t�@�p�f�[�^�\����
struct ConstBufferData
{
	XMFLOAT4 color;	//�F
	XMMATRIX mat;	//�s��
};

//�X�v���C�g�̒��_�f�[�^�^
struct VertexPosUv
{
	XMFLOAT3 pos;	//XYZ���W
	XMFLOAT2 uv;	//UV���W
};

const int spriteSRVCount = 512;//�e�N�X�`���̍ő喇��
const int objectCount = 512;//�I�u�W�F�N�g�̍ő��

//�X�v���C�g�̋��ʃf�[�^
struct SpriteCommon
{
	PipelineSet pipelineSet;//�p�C�v���C���Z�b�g
	XMMATRIX matProjection;//�ˉe�s��
	ComPtr<ID3D12DescriptorHeap> descHeap;//�e�N�X�`���p�f�X�N���v�^�q�[�v�̐���
	ComPtr<ID3D12Resource> texBuff[spriteSRVCount];//�e�N�X�`�����\�[�X�i�e�N�X�`���o�b�t�@�j�̔z��
};

//�I�u�W�F�N�g�̋��ʃf�[�^
struct ObjectCommon
{	
	PipelineSet pipelineSet;//�p�C�v���C���Z�b�g
	XMMATRIX matProjection;//�ˉe�s��
	ComPtr<ID3D12DescriptorHeap> descHeap;//�e�N�X�`���p�f�X�N���v�^�q�[�v�̐���
	ComPtr<ID3D12Resource> texBuff[objectCount];//�e�N�X�`�����\�[�X�i�e�N�X�`���o�b�t�@�j�̔z��
};

PipelineSet Object3dCreateGraphicsPipeline(ID3D12Device* dev);//3D�I�u�W�F�N�g�p�p�C�v���C������
PipelineSet SpriteCreateGraphicsPipeline(ID3D12Device* dev);//�X�v���C�g�p�p�C�v���C������
Sprite SpriteCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT, UINT texNumber, const SpriteCommon& spriteCommon, XMFLOAT2 anchorpoint, bool isFilpX, bool isFilpY);//�X�v���C�g�쐬
void SpriteCommonBeginDraw(const SpriteCommon& spriteCommon, ID3D12GraphicsCommandList* cmdList);//�X�v���C�g���ʃO���t�B�b�N�R�}���h�̃Z�b�g
void SpriteDraw(const Sprite& sprite, ID3D12GraphicsCommandList* cmdList, const SpriteCommon& spriteCommon, ID3D12Device* dev);//�X�v���C�g�P�̕\��
SpriteCommon SpriteCommonCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT);//�X�v���C�g���ʃf�[�^����
void SpriteUpdate(Sprite& sprite, const SpriteCommon& spriteCommon);//�X�v���C�g�P�̍X�V
void SpriteCommonLoadTexture(SpriteCommon& spriteCommon, UINT texnumber, const wchar_t* filename, ID3D12Device* dev);//�X�v���C�g���ʃe�N�X�`���ǂݍ���
void SpriteTransferVertexBuffer(const Sprite& sprite, const SpriteCommon& spriteCommon);//�X�v���C�g�P�̒��_�o�b�t�@�̓]��
Object objectCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT, UINT texNumber);//�I�u�W�F�N�g�쐬
Object objectCreateFlont(ID3D12Device* dev, int window_width, int window_height, UINT texNumber);
Object objectCreateBack(ID3D12Device* dev, int window_width, int window_height, UINT texNumber);
Object objectCreateLeft(ID3D12Device* dev, int window_width, int window_height, UINT texNumber);
Object objectCreateRight(ID3D12Device* dev, int window_width, int window_height, UINT texNumber);
Object objectCreateUp(ID3D12Device* dev, int window_width, int window_height, UINT texNumber);
Object objectCreateDown(ID3D12Device* dev, int window_width, int window_height, UINT texNumber);
void ObjectCommonBeginDraw(const ObjectCommon& objectCommon, ID3D12GraphicsCommandList* cmdList);//�I�u�W�F�N�g�O���t�B�b�N�R�}���h�̃Z�b�g
void ObjectDraw(const Object& object, ID3D12GraphicsCommandList* cmdList, const ObjectCommon& objectCommon, ID3D12Device* dev);//�I�u�W�F�N�g�\��
ObjectCommon ObjectCommonCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT);//�I�u�W�F�N�g�f�[�^����
void ObjectUpdate(Object& object, const ObjectCommon& objectCommon);//�I�u�W�F�N�g�X�V
void ObjectCommonLoadTexture(ObjectCommon& objectCommon, UINT texNumber, const wchar_t* filename, ID3D12Device* dev);//�I�u�W�F�N�g�e�N�X�`���ǂݍ���
void ObjectTransferVertexBuffer(const Object& object, const ObjectCommon& ObjectCommon);//�I�u�W�F�N�g���_�o�b�t�@�̓]��

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	//�e�평�����A����
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

	//��������
	Sound::SoundData soundData1 = sound->SoundLoadWave("Resource/sound/yoshida.wav");

	PipelineSet object3dPipelineSet = Object3dCreateGraphicsPipeline(DxIni->GetDev());
	PipelineSet spritePipelineSet = SpriteCreateGraphicsPipeline(DxIni->GetDev());

	//�I�u�W�F�N�g
	ObjectCommon objectCommon;
	objectCommon = ObjectCommonCreate(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT);

	//�e�N�X�`���ǂݍ���
	ObjectCommonLoadTexture(objectCommon, 0, L"Resource/image/tjmt.png", DxIni->GetDev());
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

	const int o_count = 93;
	Object object[o_count];

	for (int i = 0; i < o_count; i++)
	{
		//�I�u�W�F�N�g�̔ԍ����ƂɊ��蓖��
		if (i == 0 || i == 64 || i == 65)
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

		else if (i > 65 && i < 75)
		{
			object[i] = objectCreate(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT, i - 64);
		}

		else if (i == 75 || i == 81||i == 87)
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

		if (i == 64 || i == 65 || i > 80)
		{
			object[i].rotation = { 45, 0, 45 };
		}

		else
		{
			object[i].rotation = { 0, 0, 0 };
		}
	}

	object[0].position = { 0, 0, -10 };//�v���C���[
	object[75].position = { -30, 0, -10 };
	object[76].position = { -30, 0, -10 };
	object[77].position = { -30, 0, -10 };
	object[78].position = { -30, 0, -10 };
	object[79].position = { -30, 0, -10 };
	object[80].position = { -30, 0, -10 };

	//�ȉ��X�e�[�W
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
	object[66].position = { -70, 30, 0 };
	object[67].position = { -70, 30, 0 };
	object[68].position = { -70, 30, 0 };
	object[69].position = { -70, 30, 0 };
	object[70].position = { -70, 30, 0 };
	object[71].position = { -70, 30, 0 };
	object[72].position = { -70, 30, 0 };
	object[73].position = { -70, 30, 0 };
	object[74].position = { -70, 30, 0 };
	
	//�����ꂽ�Җ{
	object[64].position = { 80, 30, 0 };
	object[65].position = { 80, -10, 0 };
	//���{�ƌ��݌`
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

	//�X�v���C�g
	SpriteCommon spriteCommon;
	spriteCommon = SpriteCommonCreate(DxIni->GetDev(), WindowsInitialize::WIN_WIDTH, WindowsInitialize::WIN_HEIGHT);

	//�e�N�X�`���ǂݍ���
	SpriteCommonLoadTexture(spriteCommon, 0, L"Resource/image/Title.png", DxIni->GetDev());
	SpriteCommonLoadTexture(spriteCommon, 1, L"Resource/image/Back.jpg", DxIni->GetDev());
	SpriteCommonLoadTexture(spriteCommon, 2, L"Resource/image/Game_Left.png", DxIni->GetDev());
	SpriteCommonLoadTexture(spriteCommon, 3, L"Resource/image/Game_Right.png", DxIni->GetDev());
	SpriteCommonLoadTexture(spriteCommon, 4, L"Resource/image/Game_Up.png", DxIni->GetDev());
	SpriteCommonLoadTexture(spriteCommon, 5, L"Resource/image/Game_Down.png", DxIni->GetDev());
	SpriteCommonLoadTexture(spriteCommon, 6, L"Resource/image/Red.png", DxIni->GetDev());
	SpriteCommonLoadTexture(spriteCommon, 7, L"Resource/image/Green.png", DxIni->GetDev());
	SpriteCommonLoadTexture(spriteCommon, 8, L"Resource/image/Blue.png", DxIni->GetDev());

	//�X�v���C�g
	const int s_count = 7;
	Sprite sprite[s_count];

	//�X�v���C�g�̐���
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

	sprite[0].texSize = { 1024, 512 };
	sprite[1].texSize = { 812, 542 };
	sprite[2].texSize = { 1280, 720 };
	sprite[3].texSize = { 1280, 720 };
	sprite[4].texSize = { 1280, 720 };
	sprite[5].texSize = { 1280, 720 };

	//���_�o�b�t�@�ɔ��f
	for (int i = 0; i < s_count; i++)
	{
		SpriteTransferVertexBuffer(sprite[i], spriteCommon);
	}

	//�Q�[���V�[��
	enum Scene
	{
		Title, Game, End,
	};

	//�ړ�����
	enum Direction
	{
		Left, Right, Up, Down,
	};

	//�F
	enum Color
	{
		Red, Green, Blue,
	};

	int SceneNum = Title;//�Q�[���V�[��
	bool isChange = false;//�X�e�[�W�ύX����
	bool isLoad = false;//���[�h
	int LoadCount = 20;//���[�h�̃E�F�C�g
	int StageNum = 1;//0�̓X�e�[�W�Z���N�g�B1�`MaxStage
	int MaxStage = 10;//�ő�X�e�[�W��
	int MoveDirection = Right;//�i�s����
	bool isLeft = false;//�i�s����
	bool isRight = false;//�i�s����
	bool isUp = false;//�i�s����
	bool isDown = false;//�i�s����
	bool isRota = false;//��]���Ă��邩
	int timer = 0;//��]�A�ړ������̎���
	int rotaX = 0;//����X�̌������Ȃ�E�ɕ��Ȃ獶�ɉ�]���Ă���
	int rotaY = 0;//����Y�̌������Ȃ牺�ɕ��Ȃ��ɉ�]���Ă���

	//�Q�[�����[�v
	while (true)
	{
		//���b�Z�[�W���[�v
		if (WinIni->MessageLoop())
		{
			break;
		}

		//�X�V������������
		screen->Update();//�X�V����

		//1:�^�C�g��
		if (SceneNum == Title)
		{
			//SPACE����������20�t���[����ɃX�e�[�W�Z���N�g�Ɉڍs
			if (input->IsKeyTrigger(DIK_SPACE))
			{
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
				LoadCount = 20;
				for (int i = 75; i < 81; i++)
				{
					object[i].position.x = object[1].position.x;
					object[i].position.y = object[1].position.y;
				}
			}
		}

		//2:�Q�[�����
		else if (SceneNum == Game)
		{
			object[81].color = {0,0,1,1};
			object[83].color = {0,1,0,1};
			object[84].color = {1,0,0,1};
			//�w�i�̍X�V
			sprite[1].position = { sprite[0].position.x, sprite[0].position.y, 0 };

			if (StageNum > 0)
			{
				//���{�ƃR�s�[�̉�]����
				for (int i = 81; i < 87; i++)
				{
					object[i].rotation.y += 0.5;
				}
				for (int i = 87; i < 93; i++)
				{
					object[i].rotation.y += 0.5;
				}
				
				//�f�o�b�O�p�X�e�[�W�ύX����
				if (input->IsKeyTrigger(DIK_A))
				{
					if (StageNum > 0)
					{
						isChange = true;
						StageNum--;
					}
				}

				if (input->IsKeyTrigger(DIK_D))
				{
					if (StageNum < MaxStage)
					{
						isChange = true;
						StageNum++;
					}
				}
			}

			//SPACE�����������]�A�ړ������J�n
			if (input->IsKeyTrigger(DIK_SPACE) && isRota == false)
			{
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

			//�\���L�[�ňړ��������w��
			if (input->IsKey(DIK_LEFT))
			{
				MoveDirection = Left;
			}

			if (input->IsKey(DIK_RIGHT))
			{
				MoveDirection = Right;
			}

			if (input->IsKey(DIK_UP))
			{
				MoveDirection = Up;
			}

			if (input->IsKey(DIK_DOWN))
			{
				MoveDirection = Down;
			}

			//��]�A�ړ�����
			//��
			if (isLeft == true)
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
				if (timer == 1)
				{
					rotaX -= 1;
				}
			}

			else if (isLeft == true && object[75].rotation.y >= 180)
			{
				isLeft = false;
				isRota = false;
			}

			//�E
			if (isRight == true)
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
				if (timer == 1)
				{
					rotaX += 1;
				}
			}

			else if (isRight == true && object[75].rotation.y <= -180)
			{
				isRight = false;
				isRota = false;
			}

			//��
			if (isUp == true && object[75].position.y < 40)
			{
				if (abs(object[75].rotation.y) == 0.0f)
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
					if (timer == 1)
					{
						rotaY += 1;
					}
				}

				if (abs(object[75].rotation.y) == 180.0f)
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
					if (timer == 1)
					{
						rotaY -= 1;
					}
				}
			}

			else if (isUp == true && object[75].position.y >= -40)
			{
				isUp = false;
				isRota = true;
			}

			//��
			if (isDown == true && object[75].position.y > -40)
			{
				//Y���̊p�x�ɂ���ĉ�]���قȂ�abs�͐�Βl
				if (abs(object[75].rotation.y) == 0.0f)
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
					if (timer == 1)
					{
						rotaY -= 1;
					}
				}

				if (abs(object[75].rotation.y) == 180.0f)
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
					if (timer == 1)
					{
						rotaY += 1;
					}
				}

			}

			else if (isDown == true && object[75].position.y <= 40)
			{
				isDown = false;
				isRota = false;
			}

			//timer��40�ɂȂ������]�A�ړ������I��
			if (timer > 39)
			{
				isLeft = false;
				isRight = false;
				isUp = false;
				isDown = false;
				isRota = false;
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

		//�X�e�[�W�ύX����
		if (isChange == true)
		{
			if (StageNum == 0)
			{
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
				isChange = false;
			}

			else if (StageNum == 1)
			{
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
				isChange = false;
			}

			else if (StageNum == 2)
			{
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
				isChange = false;
			}

			else
			{
				isChange = false;
			}
		}

		//�X�e�[�W�ɂ���n�ʂ̐�������(�����l���X�e�[�W�̍ŏ���i)
		for (int i = 1; i < 6; i++)
		{
			//�n�ʂ̃I�u�W�F�N�gi�ɐF�����Ă��āA���̃u���b�N�̏�ɏ���Ă���Ƃ�
			if (object[i].position.x == object[75].position.x && object[i].position.y == object[75].position.y 
				&& object[i].texNumber != 1)
			{
				//�n�ʂ̐F���Ԃ̎�
				if (object[i].texNumber == 11)
				{
					if (rotaY == 0)
					{
						if (rotaX == 0)
						{
							object[76].color = { 1,0,0,1 };
						}
						if (rotaX == 1)
						{
							object[78].color = { 1,0,0,1 };
						}
						if (rotaX == 2)
						{
							object[75].color = { 1,0,0,1 };
						}
						if (rotaX == 3)
						{
							object[77].color = { 1,0,0,1 };
						}
					}
					if (rotaY == 1 || rotaY == -3)
					{
						if (rotaX == 0)
						{
							object[80].color = { 1,0,0,1 };
						}
						if (rotaX == 2)
						{
							object[80].color = { 1,0,0,1 };
						}
					}
					if (abs(rotaY) == 2)
					{
						if (rotaX == 0)
						{
							object[75].color = { 1,0,0,1 };
						}
						if (rotaX == 1)
						{
							object[78].color = { 1,0,0,1 };
						}
						if (rotaX == 2)
						{
							object[76].color = { 1,0,0,1 };
						}
						if (rotaX == 3)
						{
							object[77].color = { 1,0,0,1 };
						}
					}
					if (rotaY == 3 || rotaY == -1)
					{
						if (rotaX == 0)
						{
							object[79].color = { 1,0,0,1 };
						}
						if (rotaX == 2)
						{
							object[79].color = { 1,0,0,1 };
						}
					}
				}

				//�n�ʂ̐F���΂̎�
				if (object[i].texNumber == 12)
				{
					if (rotaY == 0)
					{
						if (rotaX == 0)
						{
							object[76].color = { 0,1,0,1 };
						}
						if (rotaX == 1)
						{
							object[78].color = { 0,1,0,1 };
						}
						if (rotaX == 2)
						{
							object[75].color = { 0,1,0,1 };
						}
						if (rotaX == 3)
						{
							object[77].color = { 0,1,0,1 };
						}
					}
					if (rotaY == 1 || rotaY == -3)
					{
						if (rotaX == 0)
						{
							object[80].color = { 0,1,0,1 };
						}
						if (rotaX == 2)
						{
							object[80].color = { 0,1,0,1 };
						}
					}
					if (abs(rotaY) == 2)
					{
						if (rotaX == 0)
						{
							object[75].color = { 0,1,0,1 };
						}
						if (rotaX == 1)
						{
							object[78].color = { 0,1,0,1 };
						}
						if (rotaX == 2)
						{
							object[76].color = { 0,1,0,1 };
						}
						if (rotaX == 3)
						{
							object[77].color = { 0,1,0,1 };
						}
					}
					if (rotaY == 3 || rotaY == -1)
					{
						if (rotaX == 0)
						{
							object[79].color = { 0,1,0,1 };
						}
						if (rotaX == 2)
						{
							object[79].color = { 0,1,0,1 };
						}
					}
				}
				//�n�ʂ̐F���̎�
				if (object[i].texNumber == 13)
				{
					if (rotaY == 0)
					{
						if (rotaX == 0)
						{
							object[76].color = { 0,0,1,1 };
						}
						if (rotaX == 1)
						{
							object[78].color = { 0,0,1,1 };
						}
						if (rotaX == 2)
						{
							object[75].color = { 0,0,1,1 };
						}
						if (rotaX == 3)
						{
							object[77].color = { 0,0,1,1 };
						}
					}
					if (rotaY == 1 || rotaY == -3)
					{
						if (rotaX == 0)
						{
							object[80].color = { 0,0,1,1 };
						}
						if (rotaX == 2)
						{
							object[80].color = { 0,0,1,1 };
						}
					}
					if (abs(rotaY) == 2)
					{
						if (rotaX == 0)
						{
							object[75].color = { 0,0,1,1 };
						}
						if (rotaX == 1)
						{
							object[78].color = { 0,0,1,1 };
						}
						if (rotaX == 2)
						{
							object[76].color = { 0,0,1,1 };
						}
						if (rotaX == 3)
						{
							object[77].color = { 0,0,1,1 };
						}
					}
					if (rotaY == 3 || rotaY == -1)
					{
						if (rotaX == 0)
						{
							object[79].color = { 0,0,1,1 };
						}
						if (rotaX == 2)
						{
							object[79].color = { 0,0,1,1 };
						}
					}
				}
			}
		}

		//�X�v���C�g�̍X�V
		for (int i = 0; i < s_count; i++)
		{
			SpriteUpdate(sprite[i], spriteCommon);
		}

		//�I�u�W�F�N�g�̍X�V
		for (int i = 0; i < o_count; i++)
		{
			ObjectUpdate(object[i], objectCommon);
		}
		
		//�X�V���������܂�

		DxIni->BeforeDraw();//�`��J�n

		//�`�揈����������
		screen->Draw();

		ID3D12GraphicsCommandList* cmdList = DxIni->GetCmdList();//�R�}���h���X�g�̎擾
		SpriteCommonBeginDraw(spriteCommon, cmdList);//�X�v���C�g���ʃR�}���h

		//�X�v���C�g�`��

		//�^�C�g��or�w�i�AUI�AHUD
		if (SceneNum == Title)
		{
			SpriteDraw(sprite[0], cmdList, spriteCommon, DxIni->GetDev());
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

		ObjectCommonBeginDraw(objectCommon, cmdList);//�I�u�W�F�N�g���ʃR�}���h
		
		//�I�u�W�F�N�g�`��
		if (SceneNum == Game)
		{
			for (int i = 1; i < 6; i++)
			{
				ObjectDraw(object[i], cmdList, objectCommon, DxIni->GetDev());
			}
			for (int i = 66; i < o_count; i++)
			{
				ObjectDraw(object[i], cmdList, objectCommon, DxIni->GetDev());
			}
		}
		//�`�揈�������܂�

		DxIni->AfterDraw();//�`��I��

		//ESC�ŋ����I��
		if (input->IsKey(DIK_ESCAPE))
		{
			break;
		}
	}

	//�e����
	delete(DxIni);
	delete(input);
	delete(sound);
	delete(screen);
	WinIni->DeleteWIN();//�E�B���h�E�̔j��
	delete(WinIni);

	return 0;
}

//3D�I�u�W�F�N�g�p�p�C�v���C������
PipelineSet Object3dCreateGraphicsPipeline(ID3D12Device* dev)
{
	HRESULT result = S_FALSE;

	ComPtr<ID3DBlob> vsBlob;//���_�V�F�[�_�I�u�W�F�N�g
	ComPtr<ID3DBlob>psBlob;//�s�N�Z���V�F�[�_�I�u�W�F�N�g
	ComPtr<ID3DBlob>errorBlob;//�G���[�I�u�W�F�N�g

	//���_�V�F�[�_�[�̓ǂݍ��݂ƃR���p�C��
	result = D3DCompileFromFile(
		L"Resource/shader/BasicVS.hlsl",				//�V�F�[�_�t�@�C����
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,				//�C���N���[�h�\�ɂ���
		"main", "vs_5_0",								//�G���g���[�|�C���g���A�V�F�[�_�[���f���w��
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//�f�o�b�O�p�ݒ�
		0,
		&vsBlob, &errorBlob);

	//�G���[�\��
	if (FAILED(result))
	{
		//errorBlob����G���[���e��string�^�ɃR�s�[
		string errstr;
		errstr.resize(errorBlob->GetBufferSize());

		copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
		errstr += "\n";

		//�G���[���e���o�̓E�B���h�E�ɕ\��
		OutputDebugStringA(errstr.c_str());
		exit(1);
	}

	//�s�N�Z���V�F�[�_�̓ǂݍ��݂ƃR���p�C��
	result = D3DCompileFromFile(
		L"Resource/shader/BasicPS.hlsl",				//�V�F�[�_�t�@�C����
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,				//�C���N���[�h�\�ɂ���
		"main", "ps_5_0",								//�G���g���[�|�C���g���A�V�F�[�_�[���f���w��
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//�f�o�b�O�p�ݒ�
		0,
		&psBlob, &errorBlob);

	//�G���[�\��
	if (FAILED(result))
	{
		//errorBlob����G���[���e��string�^�ɃR�s�[
		string errstr;
		errstr.resize(errorBlob->GetBufferSize());

		copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
		errstr += "\n";

		//�G���[���e���o�̓E�B���h�E�ɕ\��
		OutputDebugStringA(errstr.c_str());
		exit(1);
	}

	//���_���C�A�E�g
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
	};//1�s�ŏ������ق������₷��

	//�O���t�B�b�N�X�p�C�v���C���ݒ�
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};

	//���_�V�F�[�_�[�A�s�N�Z���V�F�[�_
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

	//�W���I�Ȑݒ�(�w�ʃJ�����O�A�h��Ԃ��A�[�x�N���b�s���O�L��)
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	//���X�^���C�U�X�e�[�g
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//�W���ݒ�

	//�����_�\�^�[�Q�b�g�̃u�����h�ݒ�
	D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = gpipeline.BlendState.RenderTarget[0];
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;//�W���ݒ�
	blenddesc.BlendEnable = true;//�u�����h��L���ɂ���
	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;//���Z
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;//�\�[�X�̒l��100%�g��
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;//�f�X�g�̒l��0%�g��

	//����������
	blenddesc.BlendOp = D3D12_BLEND_OP_ADD;//���Z
	blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;//�\�[�X�̃A���t�@�l
	blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//1.0f-�\�[�X�̃A���t�@�l
	gpipeline.InputLayout.pInputElementDescs = inputLayout;
	gpipeline.InputLayout.NumElements = _countof(inputLayout);
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpipeline.NumRenderTargets = 1;//�`��Ώۂ�1��
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`255�w���RGBA
	gpipeline.SampleDesc.Count = 1;//1�s�N�Z���ɂ�1��T���v�����O

	//�f�v�X�X�e���V���X�e�[�g�̐ݒ�
	//�W���I�Ȑݒ�(�[�x�e�X�g���s���A�������݋��A�[�x����������΍��i)
	gpipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//�f�X�N���v�^�e�[�u���̐ݒ�
	CD3DX12_DESCRIPTOR_RANGE descRangeSRV;
	descRangeSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);//t0���W�X�^

	//���[�g�p�����[�^�̐ݒ�
	CD3DX12_ROOT_PARAMETER rootparams[2];
	rootparams[0].InitAsConstantBufferView(0);//�萔�o�b�t�@�r���[�Ƃ��ď�����(b0���W�X�^)
	rootparams[1].InitAsDescriptorTable(1, &descRangeSRV);//�e�N�X�`���p

	//�e�N�X�`���T���v���[�̐ݒ�
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0);

	PipelineSet pipelineSet;

	//���[�g�V�O�l�`���̐ݒ�
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Init_1_0(_countof(rootparams), rootparams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> rootSigBlob;

	//�o�[�W������������ł̃V���A���C�Y
	result = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);

	//���[�g�V�O�l�`���̐���
	result = dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&pipelineSet.rootsignature));

	//�p�C�v���C���Ƀ��[�g�V�O�l�`�����Z�b�g
	gpipeline.pRootSignature = pipelineSet.rootsignature.Get();

	//�p�C�v���C���X�e�[�g�̐���
	result = dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&pipelineSet.pipelinestate));

	return pipelineSet;
}

//�X�v���C�g�p�p�C�v���C������
PipelineSet SpriteCreateGraphicsPipeline(ID3D12Device* dev)
{
	HRESULT result = S_FALSE;

	ComPtr<ID3DBlob> vsBlob;//���_�V�F�[�_�I�u�W�F�N�g
	ComPtr<ID3DBlob>psBlob;//�s�N�Z���V�F�[�_�I�u�W�F�N�g
	ComPtr<ID3DBlob>errorBlob;//�G���[�I�u�W�F�N�g

	//���_�V�F�[�_�[�̓ǂݍ��݂ƃR���p�C��
	result = D3DCompileFromFile(
		L"Resource/shader/SpriteVS.hlsl",				//�V�F�[�_�t�@�C����
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,				//�C���N���[�h�\�ɂ���
		"main", "vs_5_0",								//�G���g���[�|�C���g���A�V�F�[�_�[���f���w��
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//�f�o�b�O�p�ݒ�
		0,
		&vsBlob, &errorBlob);

	//�G���[�\��
	if (FAILED(result))
	{
		//errorBlob����G���[���e��string�^�ɃR�s�[
		string errstr;
		errstr.resize(errorBlob->GetBufferSize());

		copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
		errstr += "\n";

		//�G���[���e���o�̓E�B���h�E�ɕ\��
		OutputDebugStringA(errstr.c_str());
		exit(1);
	}

	//�s�N�Z���V�F�[�_�̓ǂݍ��݂ƃR���p�C��
	result = D3DCompileFromFile(
		L"Resource/shader/SpritePS.hlsl",				//�V�F�[�_�t�@�C����
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,				//�C���N���[�h�\�ɂ���
		"main", "ps_5_0",								//�G���g���[�|�C���g���A�V�F�[�_�[���f���w��
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//�f�o�b�O�p�ݒ�
		0,
		&psBlob, &errorBlob);

	//�G���[�\��
	if (FAILED(result))
	{
		//errorBlob����G���[���e��string�^�ɃR�s�[
		string errstr;
		errstr.resize(errorBlob->GetBufferSize());

		copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
		errstr += "\n";

		//�G���[���e���o�̓E�B���h�E�ɕ\��
		OutputDebugStringA(errstr.c_str());
		exit(1);
	}

	//���_���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{
			"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		}
	};//1�s�ŏ������ق������₷��

	//�O���t�B�b�N�X�p�C�v���C���ݒ�
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};

	//���_�V�F�[�_�[�A�s�N�Z���V�F�[�_
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

	//�W���I�Ȑݒ�(�w�ʃJ�����O�A�h��Ԃ��A�[�x�N���b�s���O�L��)
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);//��U�W���l�����Z�b�g
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//�w�ʃJ�����O�Ȃ�

	//���X�^���C�U�X�e�[�g
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//�W���ݒ�

	//�����_�\�^�[�Q�b�g�̃u�����h�ݒ�
	D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = gpipeline.BlendState.RenderTarget[0];
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;//�W���ݒ�
	blenddesc.BlendEnable = true;//�u�����h��L���ɂ���
	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;//���Z
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;//�\�[�X�̒l��100%�g��
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;//�f�X�g�̒l��0%�g��

	//����������
	blenddesc.BlendOp = D3D12_BLEND_OP_ADD;//���Z
	blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;//�\�[�X�̃A���t�@�l
	blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//1.0f-�\�[�X�̃A���t�@�l
	gpipeline.InputLayout.pInputElementDescs = inputLayout;
	gpipeline.InputLayout.NumElements = _countof(inputLayout);
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpipeline.NumRenderTargets = 1;//�`��Ώۂ�1��
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`255�w���RGBA
	gpipeline.SampleDesc.Count = 1;//1�s�N�Z���ɂ�1��T���v�����O

	//�f�v�X�X�e���V���X�e�[�g�̐ݒ�
	//�W���I�Ȑݒ�(�[�x�e�X�g���s���A�������݋��A�[�x����������΍��i)
	gpipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);//��U�W���l�����Z�b�g
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;//��ɏ㏑�����[��
	gpipeline.DepthStencilState.DepthEnable = false;//�[�x�e�X�g�����Ȃ�
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//�f�X�N���v�^�e�[�u���̐ݒ�
	CD3DX12_DESCRIPTOR_RANGE descRangeSRV;
	descRangeSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);//t0���W�X�^

	//���[�g�p�����[�^�̐ݒ�
	CD3DX12_ROOT_PARAMETER rootparams[2];
	rootparams[0].InitAsConstantBufferView(0);//�萔�o�b�t�@�r���[�Ƃ��ď�����(b0���W�X�^)
	rootparams[1].InitAsDescriptorTable(1, &descRangeSRV);//�e�N�X�`���p

	//�e�N�X�`���T���v���[�̐ݒ�
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0);

	PipelineSet pipelineSet;

	//���[�g�V�O�l�`���̐ݒ�
	ComPtr<ID3D12RootSignature> rootsignature;
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Init_1_0(_countof(rootparams), rootparams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> rootSigBlob;

	//�o�[�W������������ł̃V���A���C�Y
	result = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);

	//���[�g�V�O�l�`���̐���
	result = dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&pipelineSet.rootsignature));

	//�p�C�v���C���Ƀ��[�g�V�O�l�`�����Z�b�g
	gpipeline.pRootSignature = pipelineSet.rootsignature.Get();

	//�p�C�v���C���X�e�[�g�̐���
	ComPtr<ID3D12PipelineState> pipelinestate;
	result = dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&pipelineSet.pipelinestate));

	return pipelineSet;
}

//�X�v���C�g�쐬
Sprite SpriteCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT, UINT texNumber, const SpriteCommon& spriteCommon, XMFLOAT2 anchorpoint, bool isFilpX, bool isFilpY)
{
	HRESULT result = S_FALSE;

	//�V�����X�v���C�g�����
	Sprite sprite{};

	//���_�f�[�^
	VertexPosUv vertices[] =
	{
		{{0.0f, 100.0f, 0.0f}, {0.0f, 1.0f}},	//����
		{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},		//����
		{{100.0f, 100.0f, 0.0f}, {1.0f, 1.0f}},	//�E��
		{{100.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},	//�E��
	};

	//�e�N�X�`���ԍ����R�s�[
	sprite.texNumber = texNumber;

	//���_�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&sprite.vertBuff));

	//�w��ԍ����ǂݍ��ݍς݂Ȃ�
	if (spriteCommon.texBuff[sprite.texNumber])
	{
		//�e�N�X�`���ԍ��擾
		D3D12_RESOURCE_DESC resDesc = spriteCommon.texBuff[sprite.texNumber]->GetDesc();

		//�X�v���C�g�̑傫�����摜�̉𑜓x�ɍ��킹��
		sprite.size = { (float)resDesc.Width, (float)resDesc.Height };
	}

	//�A���J�[�|�C���g���R�s�[
	sprite.anchorpoint = anchorpoint;

	//���]�t���O���R�s�[
	sprite.isFilpX = isFilpX;
	sprite.isFilpY = isFilpY;

	//���_�o�b�t�@�̓]��
	SpriteTransferVertexBuffer(sprite, spriteCommon);

	//���_�o�b�t�@�ւ̃f�[�^�]��
	VertexPosUv* vertMap = nullptr;
	result = sprite.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	sprite.vertBuff->Unmap(0, nullptr);

	//���_�o�b�t�@�r���[�̍쐬
	sprite.vbView.BufferLocation = sprite.vertBuff->GetGPUVirtualAddress();
	sprite.vbView.SizeInBytes = sizeof(vertices);
	sprite.vbView.StrideInBytes = sizeof(vertices[0]);

	//�萔�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&sprite.constBuff)
	);

	//�萔�o�b�t�@�]��
	ConstBufferData* constMap = nullptr;
	result = sprite.constBuff->Map(0, nullptr, (void**)&constMap);
	constMap->color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	//���s���e�s��
	constMap->mat = XMMatrixOrthographicOffCenterLH(
		0.0f, WIN_WIDTH, WIN_HEIGHT, 0.0f, 0.0f, 1.0f);
	sprite.constBuff->Unmap(0, nullptr);

	return sprite;
}

//�X�v���C�g���ʃO���t�B�b�N�R�}���h�̃Z�b�g
void SpriteCommonBeginDraw(const SpriteCommon& spriteCommon, ID3D12GraphicsCommandList* cmdList)
{
	//�p�C�v���C���ƃ��[�g�V�O�l�`���̐ݒ�
	cmdList->SetPipelineState(spriteCommon.pipelineSet.pipelinestate.Get());
	cmdList->SetGraphicsRootSignature(spriteCommon.pipelineSet.rootsignature.Get());

	//�v���~�e�B�u�`���ݒ�
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//�f�X�N���v�^�q�[�v���Z�b�g
	ID3D12DescriptorHeap* ppHeaps[] = { spriteCommon.descHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
}

//�X�v���C�g�P�̕\��
void SpriteDraw(const Sprite& sprite, ID3D12GraphicsCommandList* cmdList, const SpriteCommon& spriteCommon, ID3D12Device* dev)
{
	//��\���t���O��true�Ȃ�
	if (sprite.isInvisible)
	{
		//�`�悹��������
		return;
	}

	//���_�o�b�t�@���Z�b�g
	cmdList->IASetVertexBuffers(0, 1, &sprite.vbView);

	//���_�o�b�t�@���Z�b�g
	cmdList->SetGraphicsRootConstantBufferView(0, sprite.constBuff->GetGPUVirtualAddress());

	//�V�F�[�_���\�[�X�r���[���Z�b�g
	cmdList->SetGraphicsRootDescriptorTable(1,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(
			spriteCommon.descHeap->GetGPUDescriptorHandleForHeapStart(),
			sprite.texNumber,
			dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));

	//�|���S���̕`��i�S���_�Ŏl�p�`�j
	cmdList->DrawInstanced(4, 1, 0, 0);
}

//�X�v���C�g���ʃf�[�^����
SpriteCommon SpriteCommonCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT)
{
	HRESULT result = S_FALSE;

	//�V���ȃX�v���C�g���ʃf�[�^���쐬
	SpriteCommon spriteCommon{};

	//�X�v���C�g�p�p�C�v���C������
	spriteCommon.pipelineSet = SpriteCreateGraphicsPipeline(dev);

	//���s�s��̎ˉe�s�񐶐�
	spriteCommon.matProjection = XMMatrixOrthographicOffCenterLH(
		0.0f, (float)WIN_WIDTH, (float)WIN_HEIGHT, 0.0f, 0.0f, 1.0f);

	//�f�X�N���v�^�q�[�v�𐶐�
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NumDescriptors = spriteSRVCount;
	result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&spriteCommon.descHeap));

	return spriteCommon;
}

//�X�v���C�g�P�̍X�V
void SpriteUpdate(Sprite& sprite, const SpriteCommon& spriteCommon)
{
	//���[���h�s��̍X�V
	sprite.matWorld = XMMatrixIdentity();

	//Z����]
	sprite.matWorld *= XMMatrixRotationZ(XMConvertToRadians(sprite.rotation));

	//���s�ړ�
	sprite.matWorld *= XMMatrixTranslation(sprite.position.x, sprite.position.y, sprite.position.z);

	//�萔�o�b�t�@�̓]��
	ConstBufferData* constMap = nullptr;
	HRESULT result = sprite.constBuff->Map(0, nullptr, (void**)&constMap);
	constMap->mat = sprite.matWorld * spriteCommon.matProjection;
	constMap->color = sprite.color;
	sprite.constBuff->Unmap(0, nullptr);
}

//�X�v���C�g���ʃe�N�X�`���ǂݍ���
void SpriteCommonLoadTexture(SpriteCommon& spriteCommon, UINT texNumber, const wchar_t* filename, ID3D12Device* dev)
{
	//�ُ�Ȕԍ��̎w������o
	assert(texNumber <= spriteSRVCount - 1);

	HRESULT result = S_FALSE;

	//WIC�e�N�X�`���̃��[�h
	TexMetadata metadata{};
	ScratchImage scratchIng{};

	result = LoadFromWICFile(
		filename,
		WIC_FLAGS_NONE,
		&metadata, scratchIng);

	const Image* img = scratchIng.GetImage(0, 0, 0);

	//���\�[�X�ݒ�
	CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		metadata.width,
		(UINT)metadata.height,
		(UINT16)metadata.arraySize,
		(UINT16)metadata.mipLevels);

	//�e�N�X�`���o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0),
		D3D12_HEAP_FLAG_NONE,
		&texresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&spriteCommon.texBuff[texNumber])
	);

	//�e�N�X�`���o�b�t�@�Ƀf�[�^�]��
	result = spriteCommon.texBuff[texNumber]->WriteToSubresource(
		0,
		nullptr,				//�S�̈�ɃR�s�[
		img->pixels,			//���f�[�^�A�h���X
		(UINT)img->rowPitch,	//1���C���T�C�Y
		(UINT)img->slicePitch	//�S���C���T�C�Y
	);

	//�V�F�[�_���\�[�X�r���[�ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};//�ݒ�\����
	srvDesc.Format = metadata.format;//RGBA
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
	srvDesc.Texture2D.MipLevels = 1;

	//�q�[�v��texnumber�ԖڂɃV�F�[�_���\�[�X�r���[�쐬
	dev->CreateShaderResourceView(
		spriteCommon.texBuff[texNumber].Get(),	//�r���[�Ɗ֘A�t����o�b�t�@
		&srvDesc,								//�e�N�X�`���ݒ���
		CD3DX12_CPU_DESCRIPTOR_HANDLE(spriteCommon.descHeap->GetCPUDescriptorHandleForHeapStart(), texNumber,
			dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		)
	);
}

//�X�v���C�g�P�̒��_�o�b�t�@�̓]��
void SpriteTransferVertexBuffer(const Sprite& sprite, const SpriteCommon& spriteCommon)
{
	HRESULT result = S_FALSE;

	//���_�f�[�^
	VertexPosUv vertices[] =
	{
		{{}, {0.0f, 1.0f}},//����
		{{}, {0.0f, 0.0f}},//����
		{{}, {1.0f, 1.0f}},//�E��
		{{}, {1.0f, 0.0f}},//�E��
	};

	//�����A����A�E���A�E��
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
		//�e�N�X�`�����擾
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

	vertices[LB].pos = { left, bottom, 0.0f }; //����
	vertices[LT].pos = { left, top, 0.0f }; //����
	vertices[RB].pos = { right, bottom, 0.0f }; //�E��
	vertices[RT].pos = { right, top, 0.0f }; //�E��

	//���_�o�b�t�@�ւ̃f�[�^�]��
	VertexPosUv* vertMap = nullptr;
	result = sprite.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	sprite.vertBuff->Unmap(0, nullptr);
}

//�I�u�W�F�N�g�쐬
Object objectCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT, UINT texNumber)
{
	HRESULT result = S_FALSE;

	Object object{};

	Vertex vertices[] =
	{
		//�O
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},	//����0
		{{-object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 0.0f}},	//����1
		{{object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},	//�E��2
		{{object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 0.0f}},		//�E��3

		//��
		{{object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 0.0f}},		//�E��
		{{object.size.x, object.size.y, object.size.z}, {}, {0.0f, 1.0f}},		//�E��
		{{-object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 0.0f}},	//����
		{{-object.size.x, object.size.y, object.size.z}, {}, {1.0f, 1.0f}},		//����

		//��
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},	//����
		{{-object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 0.0f}},	//����
		{{-object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},	//�E��
		{{-object.size.x, object.size.y, object.size.z}, {}, {1.0f, 0.0f}},		//�E��

		//�E
		{{object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},		//�E��
		{{object.size.x, object.size.y, object.size.z}, {}, {0.0f, 0.0f}},		//�E��
		{{object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},	//����
		{{object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 0.0f}},		//����

		//��
		{{-object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},	//����
		{{-object.size.x, object.size.y, object.size.z}, {}, {0.0f, 0.0f}},		//����
		{{object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},		//�E��
		{{object.size.x, object.size.y, object.size.z}, {}, {1.0f, 0.0f}},		//�E��

		//��
		{{object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 0.0f}},	//�E��
		{{object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 1.0f}},		//�E��
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 0.0f}},	//����
		{{-object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 1.0f}},	//����
	};

	unsigned short indices[] =
	{
		0, 1, 2, //�O�p�`1��
		2, 1, 3, //�O�p�`2��

		//��
		4, 5, 6, //�O�p�`1��
		6, 5, 7, //�O�p�`2��

		//��
		8, 9, 10, //�O�p�`1��
		10, 9, 11,//�O�p�`2��

		//�E
		12, 13, 14, //�O�p�`1��
		14, 13, 15, //�O�p�`2��

		//��
		16, 17, 18, //�O�p�`1��
		18, 17, 19, //�O�p�`2��

		//��
		20, 21, 22, //�O�p�`1��
		22, 21, 23, //�O�p�`2��
	};

	for (int i = 0; i < _countof(indices) / 3; i++)
	{//�O�p�`1���Ɍv�Z���Ă���
		//�O�p�`�̃C���f�b�N�X�����o���āA�ꎞ�I�ȕϐ��ɓ����
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];

		//�O�p�`���\�����钸�_���W���x�N�g���ɑ��
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);

		//p0��pi�x�N�g���Ap0��p2�x�N�g�����v�Z (�x�N�g���̌��Z)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);

		// �O�ς͗��ʂ��琂���ȃx�N�g��
		XMVECTOR normal = XMVector3Cross(v1, v2);

		// ���K�� (������1�ɂ���)
		normal = XMVector3Normalize(normal);

		//���߂��@���𒸓_�f�[�^�ɑ��
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	object.texNumber = texNumber;

	//���_�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.vertBuff));

	//���_�o�b�t�@�ւ̃f�[�^�]��
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);

	//���_�o�b�t�@�r���[�̍쐬
	object.vbView.BufferLocation = object.vertBuff->GetGPUVirtualAddress();
	object.vbView.SizeInBytes = sizeof(vertices);
	object.vbView.StrideInBytes = sizeof(vertices[0]);

	//�C���f�b�N�X�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.indexBuff));

	//�C���f�b�N�X�o�b�t�@�̃f�[�^�]��
	unsigned short* indexMap = nullptr;
	result = object.indexBuff->Map(0, nullptr, (void**)&indexMap);
	memcpy(indexMap, indices, sizeof(indices));
	object.indexBuff->Unmap(0, nullptr);

	object.ibView.BufferLocation = object.indexBuff->GetGPUVirtualAddress();
	object.ibView.Format = DXGI_FORMAT_R16_UINT;
	object.ibView.SizeInBytes = sizeof(indices);

	//�萔�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.constBuff)
	);

	//�萔�o�b�t�@�]��
	ConstBufferData* constMap = nullptr;
	result = object.constBuff->Map(0, nullptr, (void**)&constMap);

	//���[���h�s��̍X�V
	object.matWorld = XMMatrixIdentity();//�P�ʍs��

	//�g��s��
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);

	//��]�s��
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));

	//���s�ړ��s��
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//�r���[�̕ϊ��s��
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
		//�O
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//����0
		{{-object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 0.0f}},//���� 1
		{{object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//�E�� 2
		{{object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 0.0f}},//�E�� 3
	};

	unsigned short indices[] =
	{
		0, 1, 2, //�O�p�`1��
		2, 1, 3, //�O�p�`2��
	};

	for (int i = 0; i < _countof(indices) / 3; i++)
	{// �O�p�`1���Ɍv�Z���Ă���
		// �O�p�`�̃C���f�b�N�X�����o���āA�ꎞ�I�ȕϐ��ɓ����
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];
		//�O�p�`���\�����钸�_���W���x�N�g���ɑ��
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
		//p0��pi�x�N�g���Ap0��p2�x�N�g�����v�Z (�x�N�g���̌��Z)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);
		// �O�ς͗��ʂ��琂���ȃx�N�g��
		XMVECTOR normal = XMVector3Cross(v1, v2);
		// ���K�� (������1�ɂ���)
		normal = XMVector3Normalize(normal);
		// ���߂��@���𒸓_�f�[�^�ɑ��
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	object.texNumber = texNumber;

	//���_�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.vertBuff));

	// ���_�o�b�t�@�ւ̃f�[�^�]��
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);

	// ���_�o�b�t�@�r���[�̍쐬
	object.vbView.BufferLocation = object.vertBuff->GetGPUVirtualAddress();
	object.vbView.SizeInBytes = sizeof(vertices);
	object.vbView.StrideInBytes = sizeof(vertices[0]);

	//�C���f�b�N�X�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.indexBuff));

	//�C���f�b�N�X�o�b�t�@�̃f�[�^�]��
	unsigned short* indexMap = nullptr;
	result = object.indexBuff->Map(0, nullptr, (void**)&indexMap);
	memcpy(indexMap, indices, sizeof(indices));
	object.indexBuff->Unmap(0, nullptr);

	object.ibView.BufferLocation = object.indexBuff->GetGPUVirtualAddress();
	object.ibView.Format = DXGI_FORMAT_R16_UINT;
	object.ibView.SizeInBytes = sizeof(indices);

	//�萔�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.constBuff)
	);

	//�萔�o�b�t�@�]��
	ConstBufferData* constMap = nullptr;
	result = object.constBuff->Map(0, nullptr, (void**)&constMap);

	//���[���h�s��̍X�V
	object.matWorld = XMMatrixIdentity(); //�P�ʍs��
	//�g��s��
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);
	//��]�s��
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));
	//���s�ړ��s��
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//�r���[�̕ϊ��s��
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
		//��
		{{object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//�E��
		{{object.size.x, object.size.y, object.size.z}, {}, {0.0f, 1.0f}},//�E��
		{{-object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//����
		{{-object.size.x, object.size.y, object.size.z}, {}, {1.0f, 1.0f}},//����
	};

	unsigned short indices[] =
	{
		0, 1, 2, //�O�p�`1��
		2, 1, 3, //�O�p�`2��
	};

	for (int i = 0; i < _countof(indices) / 3; i++)
	{// �O�p�`1���Ɍv�Z���Ă���
		// �O�p�`�̃C���f�b�N�X�����o���āA�ꎞ�I�ȕϐ��ɓ����
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];
		//�O�p�`���\�����钸�_���W���x�N�g���ɑ��
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
		//p0��pi�x�N�g���Ap0��p2�x�N�g�����v�Z (�x�N�g���̌��Z)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);
		// �O�ς͗��ʂ��琂���ȃx�N�g��
		XMVECTOR normal = XMVector3Cross(v1, v2);
		// ���K�� (������1�ɂ���)
		normal = XMVector3Normalize(normal);
		// ���߂��@���𒸓_�f�[�^�ɑ��
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	object.texNumber = texNumber;

	//���_�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.vertBuff));

	// ���_�o�b�t�@�ւ̃f�[�^�]��
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);

	// ���_�o�b�t�@�r���[�̍쐬
	object.vbView.BufferLocation = object.vertBuff->GetGPUVirtualAddress();
	object.vbView.SizeInBytes = sizeof(vertices);
	object.vbView.StrideInBytes = sizeof(vertices[0]);

	//�C���f�b�N�X�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.indexBuff));

	//�C���f�b�N�X�o�b�t�@�̃f�[�^�]��
	unsigned short* indexMap = nullptr;
	result = object.indexBuff->Map(0, nullptr, (void**)&indexMap);
	memcpy(indexMap, indices, sizeof(indices));
	object.indexBuff->Unmap(0, nullptr);

	object.ibView.BufferLocation = object.indexBuff->GetGPUVirtualAddress();
	object.ibView.Format = DXGI_FORMAT_R16_UINT;
	object.ibView.SizeInBytes = sizeof(indices);

	//�萔�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.constBuff)
	);

	//�萔�o�b�t�@�]��
	ConstBufferData* constMap = nullptr;
	result = object.constBuff->Map(0, nullptr, (void**)&constMap);

	//���[���h�s��̍X�V
	object.matWorld = XMMatrixIdentity(); //�P�ʍs��
	//�g��s��
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);
	//��]�s��
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));
	//���s�ړ��s��
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//�r���[�̕ϊ��s��
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
		//��
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//����
		{{-object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//����
		{{-object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//�E��
		{{-object.size.x, object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//�E��
	};

	unsigned short indices[] =
	{
		0, 1, 2, //�O�p�`1��
		2, 1, 3, //�O�p�`2��
	};

	for (int i = 0; i < _countof(indices) / 3; i++)
	{// �O�p�`1���Ɍv�Z���Ă���
		// �O�p�`�̃C���f�b�N�X�����o���āA�ꎞ�I�ȕϐ��ɓ����
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];
		//�O�p�`���\�����钸�_���W���x�N�g���ɑ��
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
		//p0��pi�x�N�g���Ap0��p2�x�N�g�����v�Z (�x�N�g���̌��Z)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);
		// �O�ς͗��ʂ��琂���ȃx�N�g��
		XMVECTOR normal = XMVector3Cross(v1, v2);
		// ���K�� (������1�ɂ���)
		normal = XMVector3Normalize(normal);
		// ���߂��@���𒸓_�f�[�^�ɑ��
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	object.texNumber = texNumber;

	//���_�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.vertBuff));

	// ���_�o�b�t�@�ւ̃f�[�^�]��
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);

	// ���_�o�b�t�@�r���[�̍쐬
	object.vbView.BufferLocation = object.vertBuff->GetGPUVirtualAddress();
	object.vbView.SizeInBytes = sizeof(vertices);
	object.vbView.StrideInBytes = sizeof(vertices[0]);

	//�C���f�b�N�X�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.indexBuff));

	//�C���f�b�N�X�o�b�t�@�̃f�[�^�]��
	unsigned short* indexMap = nullptr;
	result = object.indexBuff->Map(0, nullptr, (void**)&indexMap);
	memcpy(indexMap, indices, sizeof(indices));
	object.indexBuff->Unmap(0, nullptr);

	object.ibView.BufferLocation = object.indexBuff->GetGPUVirtualAddress();
	object.ibView.Format = DXGI_FORMAT_R16_UINT;
	object.ibView.SizeInBytes = sizeof(indices);

	//�萔�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.constBuff)
	);

	//�萔�o�b�t�@�]��
	ConstBufferData* constMap = nullptr;
	result = object.constBuff->Map(0, nullptr, (void**)&constMap);

	//���[���h�s��̍X�V
	object.matWorld = XMMatrixIdentity(); //�P�ʍs��
	//�g��s��
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);
	//��]�s��
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));
	//���s�ړ��s��
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//�r���[�̕ϊ��s��
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
		//�E
		{{object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//�E��
		{{object.size.x, object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//�E��
		{{object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//����
		{{object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//����
	};

	unsigned short indices[] =
	{
		0, 1, 2, //�O�p�`1��
		2, 1, 3, //�O�p�`2��
	};

	for (int i = 0; i < _countof(indices) / 3; i++)
	{// �O�p�`1���Ɍv�Z���Ă���
		// �O�p�`�̃C���f�b�N�X�����o���āA�ꎞ�I�ȕϐ��ɓ����
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];
		//�O�p�`���\�����钸�_���W���x�N�g���ɑ��
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
		//p0��pi�x�N�g���Ap0��p2�x�N�g�����v�Z (�x�N�g���̌��Z)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);
		// �O�ς͗��ʂ��琂���ȃx�N�g��
		XMVECTOR normal = XMVector3Cross(v1, v2);
		// ���K�� (������1�ɂ���)
		normal = XMVector3Normalize(normal);
		// ���߂��@���𒸓_�f�[�^�ɑ��
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	object.texNumber = texNumber;

	//���_�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.vertBuff));

	// ���_�o�b�t�@�ւ̃f�[�^�]��
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);

	// ���_�o�b�t�@�r���[�̍쐬
	object.vbView.BufferLocation = object.vertBuff->GetGPUVirtualAddress();
	object.vbView.SizeInBytes = sizeof(vertices);
	object.vbView.StrideInBytes = sizeof(vertices[0]);

	//�C���f�b�N�X�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.indexBuff));

	//�C���f�b�N�X�o�b�t�@�̃f�[�^�]��
	unsigned short* indexMap = nullptr;
	result = object.indexBuff->Map(0, nullptr, (void**)&indexMap);
	memcpy(indexMap, indices, sizeof(indices));
	object.indexBuff->Unmap(0, nullptr);

	object.ibView.BufferLocation = object.indexBuff->GetGPUVirtualAddress();
	object.ibView.Format = DXGI_FORMAT_R16_UINT;
	object.ibView.SizeInBytes = sizeof(indices);

	//�萔�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.constBuff)
	);

	//�萔�o�b�t�@�]��
	ConstBufferData* constMap = nullptr;
	result = object.constBuff->Map(0, nullptr, (void**)&constMap);

	//���[���h�s��̍X�V
	object.matWorld = XMMatrixIdentity(); //�P�ʍs��
	//�g��s��
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);
	//��]�s��
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));
	//���s�ړ��s��
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//�r���[�̕ϊ��s��
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
		//��
		{{-object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//����
		{{-object.size.x, object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//����
		{{object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//�E��
		{{object.size.x, object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//�E��
	};

	unsigned short indices[] =
	{
		0, 1, 2, //�O�p�`1��
		2, 1, 3, //�O�p�`2��
	};

	for (int i = 0; i < _countof(indices) / 3; i++)
	{// �O�p�`1���Ɍv�Z���Ă���
		// �O�p�`�̃C���f�b�N�X�����o���āA�ꎞ�I�ȕϐ��ɓ����
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];
		//�O�p�`���\�����钸�_���W���x�N�g���ɑ��
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
		//p0��pi�x�N�g���Ap0��p2�x�N�g�����v�Z (�x�N�g���̌��Z)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);
		// �O�ς͗��ʂ��琂���ȃx�N�g��
		XMVECTOR normal = XMVector3Cross(v1, v2);
		// ���K�� (������1�ɂ���)
		normal = XMVector3Normalize(normal);
		// ���߂��@���𒸓_�f�[�^�ɑ��
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	object.texNumber = texNumber;

	//���_�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.vertBuff));

	// ���_�o�b�t�@�ւ̃f�[�^�]��
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);

	// ���_�o�b�t�@�r���[�̍쐬
	object.vbView.BufferLocation = object.vertBuff->GetGPUVirtualAddress();
	object.vbView.SizeInBytes = sizeof(vertices);
	object.vbView.StrideInBytes = sizeof(vertices[0]);

	//�C���f�b�N�X�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.indexBuff));

	//�C���f�b�N�X�o�b�t�@�̃f�[�^�]��
	unsigned short* indexMap = nullptr;
	result = object.indexBuff->Map(0, nullptr, (void**)&indexMap);
	memcpy(indexMap, indices, sizeof(indices));
	object.indexBuff->Unmap(0, nullptr);

	object.ibView.BufferLocation = object.indexBuff->GetGPUVirtualAddress();
	object.ibView.Format = DXGI_FORMAT_R16_UINT;
	object.ibView.SizeInBytes = sizeof(indices);

	//�萔�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.constBuff)
	);

	//�萔�o�b�t�@�]��
	ConstBufferData* constMap = nullptr;
	result = object.constBuff->Map(0, nullptr, (void**)&constMap);

	//���[���h�s��̍X�V
	object.matWorld = XMMatrixIdentity(); //�P�ʍs��
	//�g��s��
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);
	//��]�s��
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));
	//���s�ړ��s��
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//�r���[�̕ϊ��s��
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
		//��
		{{object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 0.0f}},//�E��
		{{object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 1.0f}},//�E��
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 0.0f}},//����
		{{-object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 1.0f}},//����
	};

	unsigned short indices[] =
	{
		0, 1, 2, //�O�p�`1��
		2, 1, 3, //�O�p�`2��
	};

	for (int i = 0; i < _countof(indices) / 3; i++)
	{// �O�p�`1���Ɍv�Z���Ă���
		// �O�p�`�̃C���f�b�N�X�����o���āA�ꎞ�I�ȕϐ��ɓ����
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];
		//�O�p�`���\�����钸�_���W���x�N�g���ɑ��
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
		//p0��pi�x�N�g���Ap0��p2�x�N�g�����v�Z (�x�N�g���̌��Z)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);
		// �O�ς͗��ʂ��琂���ȃx�N�g��
		XMVECTOR normal = XMVector3Cross(v1, v2);
		// ���K�� (������1�ɂ���)
		normal = XMVector3Normalize(normal);
		// ���߂��@���𒸓_�f�[�^�ɑ��
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

	object.texNumber = texNumber;

	//���_�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.vertBuff));

	// ���_�o�b�t�@�ւ̃f�[�^�]��
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);

	// ���_�o�b�t�@�r���[�̍쐬
	object.vbView.BufferLocation = object.vertBuff->GetGPUVirtualAddress();
	object.vbView.SizeInBytes = sizeof(vertices);
	object.vbView.StrideInBytes = sizeof(vertices[0]);

	//�C���f�b�N�X�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.indexBuff));

	//�C���f�b�N�X�o�b�t�@�̃f�[�^�]��
	unsigned short* indexMap = nullptr;
	result = object.indexBuff->Map(0, nullptr, (void**)&indexMap);
	memcpy(indexMap, indices, sizeof(indices));
	object.indexBuff->Unmap(0, nullptr);

	object.ibView.BufferLocation = object.indexBuff->GetGPUVirtualAddress();
	object.ibView.Format = DXGI_FORMAT_R16_UINT;
	object.ibView.SizeInBytes = sizeof(indices);

	//�萔�o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object.constBuff)
	);

	//�萔�o�b�t�@�]��
	ConstBufferData* constMap = nullptr;
	result = object.constBuff->Map(0, nullptr, (void**)&constMap);

	//���[���h�s��̍X�V
	object.matWorld = XMMatrixIdentity(); //�P�ʍs��
	//�g��s��
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);
	//��]�s��
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));
	//���s�ړ��s��
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//�r���[�̕ϊ��s��
	object.matView = XMMatrixLookAtLH(XMLoadFloat3(&object.eye), XMLoadFloat3(&object.target), XMLoadFloat3(&object.up));

	constMap->mat = object.matWorld * object.matView;
	constMap->color = object.color;

	object.constBuff->Unmap(0, nullptr);

	return object;
}

//�I�u�W�F�N�g�O���t�B�b�N�R�}���h�̃Z�b�g
void ObjectCommonBeginDraw(const ObjectCommon& objectCommon, ID3D12GraphicsCommandList* cmdList)
{
	//�p�C�v���C���ƃ��[�g�V�O�l�`���̐ݒ�
	cmdList->SetPipelineState(objectCommon.pipelineSet.pipelinestate.Get());
	cmdList->SetGraphicsRootSignature(objectCommon.pipelineSet.rootsignature.Get());

	//�v���~�e�B�u�`���ݒ�
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//�f�X�N���v�^�q�[�v���Z�b�g
	ID3D12DescriptorHeap* ppHeaps[] = { objectCommon.descHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
}

//�I�u�W�F�N�g�\��
void ObjectDraw(const Object& object, ID3D12GraphicsCommandList* cmdList, const ObjectCommon& objectCommon, ID3D12Device* dev)
{
	//���_�o�b�t�@���Z�b�g
	cmdList->IASetVertexBuffers(0, 1, &object.vbView);

	cmdList->IASetIndexBuffer(&object.ibView);

	//�萔�o�b�t�@���Z�b�g
	cmdList->SetGraphicsRootConstantBufferView(0, object.constBuff->GetGPUVirtualAddress());


	//�V�F�[�_���\�[�X�r���[���Z�b�g
	cmdList->SetGraphicsRootDescriptorTable(1,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(
			objectCommon.descHeap->GetGPUDescriptorHandleForHeapStart(),
			object.texNumber,
			dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));

	//�|���S���̕`��i�S���_�Ŏl�p�`�j
	cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}

//�I�u�W�F�N�g�f�[�^����
ObjectCommon ObjectCommonCreate(ID3D12Device* dev, int WIN_WIDTH, int WIN_HEIGHT)
{
	HRESULT result = S_FALSE;

	//�V���ȃX�v���C�g�������ʃf�[�^�𐶐�
	ObjectCommon objectCommon{};

	//�I�u�W�F�N�g�p�p�C�v���C������
	objectCommon.pipelineSet = Object3dCreateGraphicsPipeline(dev);

	//�ˉe�ϊ��s��(�������e)
	objectCommon.matProjection = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(60.0f),		//�㉺��p60�x
		(float)WIN_WIDTH / WIN_HEIGHT,	//�A�X�y�N�g��i��ʉ��� / ��ʏc���j
		0.1f, 2000.0f					//�O�[�A���[
	);

	//�f�X�N���v�^�q�[�v�𐶐�
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NumDescriptors = objectCount;
	result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&objectCommon.descHeap));

	//���������X�v���C�g���ʃf�[�^��Ԃ�
	return objectCommon;
}

//�I�u�W�F�N�g�X�V
void ObjectUpdate(Object& object, const ObjectCommon& objectCommon)
{
	//���[���h�s��̍X�V
	object.matWorld = XMMatrixIdentity();

	//�g��s��
	object.matWorld *= XMMatrixScaling(object.scale.x, object.scale.y, object.scale.z);

	//Z����]�s��
	object.matWorld *= XMMatrixRotationZ(XMConvertToRadians(object.rotation.z));
	object.matWorld *= XMMatrixRotationX(XMConvertToRadians(object.rotation.x));
	object.matWorld *= XMMatrixRotationY(XMConvertToRadians(object.rotation.y));

	//���s�s��
	object.matWorld *= XMMatrixTranslation(object.position.x, object.position.y, object.position.z);

	//�r���[�̕ϊ��s��
	object.matView = XMMatrixLookAtLH(XMLoadFloat3(&object.eye), XMLoadFloat3(&object.target), XMLoadFloat3(&object.up));

	//�萔�o�b�t�@�̓]��
	ConstBufferData* constMap = nullptr;
	HRESULT result = object.constBuff->Map(0, nullptr, (void**)&constMap);
	constMap->mat = object.matWorld * object.matView * objectCommon.matProjection;
	constMap->color = object.color;
	object.constBuff->Unmap(0, nullptr);
}

//�I�u�W�F�N�g�e�N�X�`���ǂݍ���
void ObjectCommonLoadTexture(ObjectCommon& objectCommon, UINT texNumber, const wchar_t* filename, ID3D12Device* dev)
{
	//�ُ�Ȕԍ��̎w������o
	assert(texNumber <= spriteSRVCount - 1);

	HRESULT result = S_FALSE;

	//WIC�e�N�X�`���̃��[�h
	TexMetadata metadata{};
	ScratchImage scratchIng{};

	result = LoadFromWICFile(
		filename,
		WIC_FLAGS_NONE,
		&metadata, scratchIng);

	const Image* img = scratchIng.GetImage(0, 0, 0);

	//���\�[�X�ݒ�
	CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		metadata.width,
		(UINT)metadata.height,
		(UINT16)metadata.arraySize,
		(UINT16)metadata.mipLevels);

	//�e�N�X�`���o�b�t�@�̐���
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0),
		D3D12_HEAP_FLAG_NONE,
		&texresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&objectCommon.texBuff[texNumber])
	);

	//�e�N�X�`���o�b�t�@�Ƀf�[�^�]��
	result = objectCommon.texBuff[texNumber]->WriteToSubresource(
		0,
		nullptr,				//�S�̈�ɃR�s�[
		img->pixels,			//���f�[�^�A�h���X
		(UINT)img->rowPitch,	//1���C���T�C�Y
		(UINT)img->slicePitch	//�S���C���T�C�Y
	);

	//�V�F�[�_���\�[�X�r���[�ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};//�ݒ�\����
	srvDesc.Format = metadata.format;//RGBA
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
	srvDesc.Texture2D.MipLevels = 1;

	//�q�[�v��texnumber�ԖڂɃV�F�[�_���\�[�X�r���[�쐬
	dev->CreateShaderResourceView(
		objectCommon.texBuff[texNumber].Get(),//�r���[�Ɗ֘A�t����o�b�t�@
		&srvDesc,//�e�N�X�`���ݒ���
		CD3DX12_CPU_DESCRIPTOR_HANDLE(objectCommon.descHeap->GetCPUDescriptorHandleForHeapStart(), texNumber,
			dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		)
	);
}

//�I�u�W�F�N�g���_�o�b�t�@�̓]��
void ObjectTransferVertexBuffer(const Object& object, const ObjectCommon& ObjectCommon)
{
	HRESULT result = S_FALSE;

	Vertex vertices[] =
	{
		//�O
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//����0
		{{-object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 0.0f}},//���� 1
		{{object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//�E�� 2
		{{object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 0.0f}},//�E�� 3

		//��
		{{object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//�E��
		{{object.size.x, object.size.y, object.size.z}, {}, {0.0f, 1.0f}},//�E��
		{{-object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//����
		{{-object.size.x, object.size.y, object.size.z}, {}, {1.0f, 1.0f}},//����

		//��
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//����
		{{-object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//����
		{{-object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//�E��
		{{-object.size.x, object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//�E��

		//�E
		{{object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//�E��
		{{object.size.x, object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//�E��
		{{object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//����
		{{object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//����

		//��
		{{-object.size.x, object.size.y, -object.size.z}, {}, {0.0f, 1.0f}},//����
		{{-object.size.x, object.size.y, object.size.z}, {}, {0.0f, 0.0f}},//����
		{{object.size.x, object.size.y, -object.size.z}, {}, {1.0f, 1.0f}},//�E��
		{{object.size.x, object.size.y, object.size.z}, {}, {1.0f, 0.0f}},//�E��

		//��
		{{object.size.x, -object.size.y, -object.size.z}, {}, {0.0f, 0.0f}},//�E��
		{{object.size.x, -object.size.y, object.size.z}, {}, {0.0f, 1.0f}},//�E��
		{{-object.size.x, -object.size.y, -object.size.z}, {}, {1.0f, 0.0f}},//����
		{{-object.size.x, -object.size.y, object.size.z}, {}, {1.0f, 1.0f}},//����
	};

	// ���_�o�b�t�@�ւ̃f�[�^�]��
	Vertex* vertMap = nullptr;
	result = object.vertBuff->Map(0, nullptr, (void**)&vertMap);
	memcpy(vertMap, vertices, sizeof(vertices));
	object.vertBuff->Unmap(0, nullptr);
}