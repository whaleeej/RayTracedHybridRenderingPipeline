#pragma once
#define WIN32_LEAN_AND_MEAN // for Windows.h minimum included
#include <Windows.h>
#include <wrl.h> // Windows runtime library
#ifndef MS_ComPtr
#define MS_ComPtr(TYPE) Microsoft::WRL::ComPtr<TYPE>
#endif

// DX12 spec
#include <d3d12.h>
#include <dxgi1_6.h> // DXGI1.6 adds HDR feature
#include <d3dx12.h>

class DXFactory {
public:
	static MS_ComPtr(IDXGIAdapter4) GetAdapter(bool useWarp);
	static MS_ComPtr(ID3D12Device2) CreateDevice(MS_ComPtr(IDXGIAdapter4) adapter);
	static MS_ComPtr(ID3D12CommandQueue) CreateCommandQueue(MS_ComPtr(ID3D12Device2) device,
		D3D12_COMMAND_LIST_TYPE type);
	static MS_ComPtr(IDXGISwapChain4)  CreateSwapChain(HWND hWnd,
		MS_ComPtr(ID3D12CommandQueue) commandQueue,
		uint32_t width, uint32_t height, uint32_t bufferCount);
	static MS_ComPtr(ID3D12DescriptorHeap) CreateDescriptorHeap(MS_ComPtr(ID3D12Device2) device,
		D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
	static MS_ComPtr(ID3D12CommandAllocator) CreateCommandAllocator(MS_ComPtr(ID3D12Device2) device,
		D3D12_COMMAND_LIST_TYPE type);
	static MS_ComPtr(ID3D12GraphicsCommandList) CreateCommandList(MS_ComPtr(ID3D12Device2) device,
		MS_ComPtr(ID3D12CommandAllocator) commandAllocator, D3D12_COMMAND_LIST_TYPE type);
	static MS_ComPtr(ID3D12Fence) CreateFence(MS_ComPtr(ID3D12Device2) device);
	static HANDLE CreateEventHandle();

	// excluded from factory
	static void UpdateRenderTargetViews(MS_ComPtr(ID3D12Device2) device,
		MS_ComPtr(IDXGISwapChain4) swapChain, MS_ComPtr(ID3D12DescriptorHeap) descriptorHeap,
		MS_ComPtr(ID3D12Resource) backBuffers[], const uint8_t numFrames);
	static bool CheckTearingSupport();
	
};
