#pragma once
#include <stdint.h>
#include <chrono>
#include <memory>
#include <wrl.h> // Windows runtime library
#ifndef MS_ComPtr
#define MS_ComPtr(TYPE) Microsoft::WRL::ComPtr<TYPE>
#endif

// DX12 spec
#include <d3d12.h>
#include <dxgi1_6.h> // DXGI1.6 adds HDR feature

class Window;
class DXCommandRequester;

class DXClient {
public:
	DXClient(bool useWarp=false, bool vSync=false, bool tearing=false);
	~DXClient();
	void initialize(Window& window);
	void resize(Window& window, uint32_t width, uint32_t height);

	void update();
	void render();

public:
	const static uint8_t g_NumFrames = 3; // back buffers

	// Support configuration
	bool g_UseWarp; // Windows Advance Rasterization Platform
	bool g_VSync;
	bool g_TearingSupported ;

	// DirectX 12 Objects
	MS_ComPtr(ID3D12Device2) g_Device;
	MS_ComPtr(IDXGISwapChain4) g_SwapChain; // used to present
	MS_ComPtr(ID3D12Resource) g_BackBuffers[g_NumFrames]; // pointer to back buffer resources
	MS_ComPtr(ID3D12DescriptorHeap) g_RTVDescriptorHeap; // RTV(view=desc) location,type/dim of the RT(tex), 
	UINT g_RTVDescriptorSize;
	UINT g_CurrentBackBufferIndex;

	// CommandQueue
	std::unique_ptr<DXCommandRequester> g_CommandRequester;
	uint64_t g_FenceValues[g_NumFrames];

	// Frametime
	bool g_IsCounterInitialized;
	uint64_t frameCounter;
	double elapsedSeconds;
	std::chrono::high_resolution_clock clock;
	std::chrono::time_point<std::chrono::steady_clock> t0 ;

	bool g_IsInitialized;
};