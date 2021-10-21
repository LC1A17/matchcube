#pragma once

#include "WindowsInitialize.h"
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <d3dx12.h>

using namespace Microsoft::WRL;
using namespace std;

//DirectX����������
class DirectXInitialize
{
public://�֐�
	ID3D12Device* GetDev();//�f�o�C�X�̎擾
	ID3D12GraphicsCommandList* GetCmdList();//�R�}���h���X�g�̎擾
	void Initialize(WindowsInitialize* WinIni);//������
	void BeforeDraw();//�`��R�}���h�O
	void AfterDraw();//�`��R�}���h��

private://�ϐ�
	//�������p
	WindowsInitialize* winini;
	ComPtr<ID3D12Device> dev;
	ComPtr<IDXGIFactory6> dxgiFactory;
	ComPtr<IDXGIAdapter1> tmpAdapter;
	ComPtr<IDXGISwapChain4> swapchain;
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> cmdList;
	ComPtr<ID3D12CommandQueue> cmdQueue;
	ComPtr<ID3D12DescriptorHeap> rtvHeaps;
	ComPtr<ID3D12Fence> fence;
	ComPtr<ID3D12Resource> depthBuffer;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	vector<ComPtr<ID3D12Resource>> backBuffers;
	UINT64 fenceVal = 0;
	D3D12_RESOURCE_BARRIER barrierDesc{};
	
private://�֐�
	void InitializeAdapter();//�A�_�v�^�̗�
	void InitializeDevice();//�f�o�C�X�̐���
	void InitializeCmdList();//�R�}���h���X�g
	void CreateSwapChain();//�X���b�v�`�F�[��
	void CreateRTV();//�����_�[�^�[�Q�b�g�r���[
	void CreateFence();//�t�F���X
	void CreateDepthBuffer();//�[�x�o�b�t�@����

};