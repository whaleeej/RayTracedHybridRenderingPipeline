//#include <d3dcompiler.h> 
//#include <DirectXMath.h>

#include "DXClient.h"
#include "DXFactory.h"
#include "WinImpl.h"
#include "Helpers.h"

DXClient::DXClient(bool useWarp, bool vSync, bool tearing):
	g_UseWarp(useWarp), g_VSync(vSync), g_TearingSupported(tearing)
{
	g_FenceValue = 0;
	
	g_IsCounterInitialized = false;
	frameCounter = 0;
	elapsedSeconds = 0.0;
	t0 = clock.now();

	g_IsInitialized = false;
}

DXClient::~DXClient()
{
	::CloseHandle(g_FenceEvent);
}

void DXClient::initialize(WinImpl& winImpl)
{
	// tearing supported
	g_TearingSupported = DXFactory::CheckTearingSupport();

	// device
	ComPtr<IDXGIAdapter4> dxgiAdapter4 = DXFactory::GetAdapter(g_UseWarp);
	g_Device = DXFactory::CreateDevice(dxgiAdapter4);

	// command queue and swap chain
	g_CommandQueue = DXFactory::CreateCommandQueue(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	g_SwapChain = DXFactory::CreateSwapChain(winImpl.g_hWnd, g_CommandQueue,
		winImpl.g_ClientWidth, winImpl.g_ClientHeight, DXClient::g_NumFrames);

	// RTV heap
	g_RTVDescriptorHeap = DXFactory::CreateDescriptorHeap(g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, DXClient::g_NumFrames);
	g_RTVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// backBuffer+RTV in heap and commandAllocator
	DXFactory::UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap, g_BackBuffers, DXClient::g_NumFrames);
	for (int i = 0; i < DXClient::g_NumFrames; ++i)
	{
		g_CommandAllocators[i] = DXFactory::CreateCommandAllocator(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	}

	// command List
	g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
	g_CommandList = DXFactory::CreateCommandList(g_Device,
		g_CommandAllocators[g_CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

	// fence and fenceEvent
	g_Fence = DXFactory::CreateFence(g_Device);
	g_FenceEvent = DXFactory::CreateEventHandle();

	g_IsInitialized = true;
}

uint64_t DXClient::signal(uint64_t& fenceValue)
{
	uint64_t fenceValueForSignal = ++fenceValue;
	ThrowIfFailed(g_CommandQueue->Signal(g_Fence.Get(), fenceValueForSignal));

	return fenceValueForSignal;
}

void DXClient::waitForFenceValue( uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration)
{
	if (g_Fence->GetCompletedValue() < fenceValue)
	{
		ThrowIfFailed(g_Fence->SetEventOnCompletion(fenceValue, fenceEvent));
		::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
	}
}

void DXClient::flush(uint64_t& fenceValue, HANDLE fenceEvent)
{
	uint64_t fenceValueForSignal = signal(fenceValue);
	waitForFenceValue(fenceValueForSignal, fenceEvent);
}

void DXClient::flush_out()
{
	this->flush(g_FenceValue, g_FenceEvent);
}

void DXClient::update()
{
	if (!g_IsCounterInitialized) {
		frameCounter = 0;
		elapsedSeconds = 0.0;
		t0 = clock.now();
		g_IsCounterInitialized = true;
	}
	frameCounter++;
	auto t1 = clock.now();
	auto deltaTime = t1 - t0;
	t0 = t1;
	elapsedSeconds += deltaTime.count() * 1e-9;
	if (elapsedSeconds > 1.0)
	{
		char buffer[500];
		auto fps = frameCounter / elapsedSeconds;
		sprintf_s(buffer, 500, "FPS: %f\n", fps);
		OutputDebugString(buffer);

		frameCounter = 0;
		elapsedSeconds = 0.0;
	}
}

void DXClient::render()
{
	auto commandAllocator = g_CommandAllocators[g_CurrentBackBufferIndex];
	auto backBuffer = g_BackBuffers[g_CurrentBackBufferIndex];

	commandAllocator->Reset();
	g_CommandList->Reset(commandAllocator.Get(), nullptr);
	// Clear the render target.
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		g_CommandList->ResourceBarrier(1, &barrier);
		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			g_CurrentBackBufferIndex, g_RTVDescriptorSize);

		g_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	}
	// Present
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		g_CommandList->ResourceBarrier(1, &barrier);
		ThrowIfFailed(g_CommandList->Close());

		ID3D12CommandList* const commandLists[] = {
			g_CommandList.Get()
		};
		g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
		UINT syncInterval = g_VSync ? 1 : 0;
		UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
		ThrowIfFailed(g_SwapChain->Present(syncInterval, presentFlags));

		g_FrameFenceValues[g_CurrentBackBufferIndex] = signal(g_FenceValue);
		g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

		waitForFenceValue(g_FrameFenceValues[g_CurrentBackBufferIndex], g_FenceEvent);
	}
}

void DXClient::resize(WinImpl& winImpl,  uint32_t width, uint32_t height)
{
	if (winImpl.g_ClientWidth != width || winImpl.g_ClientHeight != height)
	{
		// Don't allow 0 size swap chain back buffers.
		winImpl.resize(width, height);

		// Flush the GPU queue to make sure the swap chain's back buffers
		// are not being referenced by an in-flight command list.
		flush(g_FenceValue, g_FenceEvent);

		for (int i = 0; i < g_NumFrames; ++i)
		{
			// Any references to the back buffers must be released
			// before the swap chain can be resized.
			g_BackBuffers[i].Reset();
			g_FrameFenceValues[i] = g_FrameFenceValues[g_CurrentBackBufferIndex];
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(g_SwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(g_SwapChain->ResizeBuffers(g_NumFrames, winImpl.g_ClientWidth, winImpl.g_ClientHeight,
			swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

		DXFactory::UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap, g_BackBuffers, g_NumFrames);
	}
}
