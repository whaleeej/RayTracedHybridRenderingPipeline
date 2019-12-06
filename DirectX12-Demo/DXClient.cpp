//#include <d3dcompiler.h> 
//#include <DirectXMath.h>

#include "DXClient.h"
#include "DXFactory.h"
#include "WinImpl.h"
#include "Helpers.h"

uint64_t DXClient::signal(uint64_t& fenceValue)
{
	uint64_t fenceValueForSignal = ++fenceValue;
	ThrowIfFailed(g_CommandQueue->Signal(g_Fence.Get(), fenceValueForSignal));

	return fenceValueForSignal;
}

void DXClient::waitForFenceValue( uint64_t fenceValue, HANDLE fenceEvent,
	std::chrono::milliseconds duration = std::chrono::milliseconds::max())
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
