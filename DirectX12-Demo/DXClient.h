#pragma once
#include <stdint.h>
#include <chrono>
#include <wrl.h> // Windows runtime library
#ifndef MS_ComPtr
#define MS_ComPtr(TYPE) Microsoft::WRL::ComPtr<TYPE>
#endif

// DX12 spec
#include <d3d12.h>
#include <dxgi1_6.h> // DXGI1.6 adds HDR feature
#include <d3dx12.h>

#if defined(max)
#undef max
#endif
#if defined(min)
#undef min
#endif

class WinImpl;

class DXClient {
public:
	DXClient(bool useWarp=false, bool vSync=false, bool tearing=false);
	~DXClient();
	void initialize(WinImpl& winImpl);

	uint64_t signal(uint64_t& fenceValue);
	void waitForFenceValue(uint64_t fenceValue, HANDLE fenceEvent,
		std::chrono::milliseconds duration = std::chrono::milliseconds::max());
	void flush(uint64_t& fenceValue, HANDLE fenceEvent);

	void flush_out();
	void update();
	void render();
	void resize(WinImpl& winImpl, uint32_t width, uint32_t height);

public:
	const static uint8_t g_NumFrames = 3; // back buffers

	// Support configuration
	bool g_UseWarp; // Windows Advance Rasterization Platform
	bool g_VSync;
	bool g_TearingSupported ;

	// DirectX 12 Objects
	MS_ComPtr(ID3D12Device2) g_Device1;
	MS_ComPtr(ID3D12Device2) g_Device;
	MS_ComPtr(ID3D12CommandQueue) g_CommandQueue;
	MS_ComPtr(IDXGISwapChain4) g_SwapChain; // used to present
	MS_ComPtr(ID3D12Resource) g_BackBuffers[g_NumFrames]; // pointer to back buffer resources
	MS_ComPtr(ID3D12GraphicsCommandList) g_CommandList; // one per thread
	MS_ComPtr(ID3D12CommandAllocator) g_CommandAllocators[g_NumFrames]; // backing memory for commands, sync with GPU execution->one per backbuffer
	MS_ComPtr(ID3D12DescriptorHeap) g_RTVDescriptorHeap; // RTV(view=desc) location,type/dim of the RT(tex), 
	UINT g_RTVDescriptorSize;
	UINT g_CurrentBackBufferIndex;

	// Sync Objects
	MS_ComPtr(ID3D12Fence) g_Fence; // fence per command queue
	uint64_t g_FenceValue ;
	uint64_t g_FrameFenceValues[g_NumFrames] ;
	HANDLE g_FenceEvent; // handle to osEvent

	// Frametime
	bool g_IsCounterInitialized;
	uint64_t frameCounter;
	double elapsedSeconds;
	std::chrono::high_resolution_clock clock;
	std::chrono::time_point<std::chrono::steady_clock> t0 ;

	bool g_IsInitialized;
};