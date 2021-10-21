#pragma once

#include "WindowsInitialize.h"
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <d3dx12.h>

using namespace Microsoft::WRL;
using namespace std;

//DirectX初期化処理
class DirectXInitialize
{
public://関数
	ID3D12Device* GetDev();//デバイスの取得
	ID3D12GraphicsCommandList* GetCmdList();//コマンドリストの取得
	void Initialize(WindowsInitialize* WinIni);//初期化
	void BeforeDraw();//描画コマンド前
	void AfterDraw();//描画コマンド後

private://変数
	//初期化用
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
	
private://関数
	void InitializeAdapter();//アダプタの列挙
	void InitializeDevice();//デバイスの生成
	void InitializeCmdList();//コマンドリスト
	void CreateSwapChain();//スワップチェーン
	void CreateRTV();//レンダーターゲットビュー
	void CreateFence();//フェンス
	void CreateDepthBuffer();//深度バッファ生成

};