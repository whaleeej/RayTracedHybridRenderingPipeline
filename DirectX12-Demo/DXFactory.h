#pragma once
#include <Windows.h>
#include <stdint.h>

// DX12 spec
#include <d3d12.h>
#include <dxgi1_6.h> // DXGI1.6 adds HDR feature

#include <wrl.h> // Windows runtime library
using namespace Microsoft::WRL;

class DXFactory {
public:
	static ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);
	static ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter);
	static ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device,
		D3D12_COMMAND_LIST_TYPE type);
	static ComPtr<IDXGISwapChain4>  CreateSwapChain(HWND hWnd,
		ComPtr<ID3D12CommandQueue> commandQueue,
		uint32_t width, uint32_t height, uint32_t bufferCount);
	static ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device,
		D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
	static ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device,
		D3D12_COMMAND_LIST_TYPE type);
	static ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device,
		ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);
	static ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device);
	static HANDLE CreateEventHandle();

	// excluded from factory
	static void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device,
		ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap,
		ComPtr<ID3D12Resource> backBuffers[], const uint8_t numFrames);
	static bool CheckTearingSupport();
	
};
