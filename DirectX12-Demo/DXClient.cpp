//#include <d3dcompiler.h>
//#include <DirectXMath.h>
#include <d3dx12.h>

#include "DXClient.h"

#include "Helpers.h"
#include "Window.h"
#include "DXFactory.h"
#include "DXCommandRequester.h"

DXClient::DXClient(bool useWarp, bool vSync, bool tearing):
	g_UseWarp(useWarp), g_VSync(vSync), g_TearingSupported(tearing)
{
	g_IsCounterInitialized = false;
	frameCounter = 0;
	elapsedSeconds = 0.0;
	t0 = clock.now();

	g_IsInitialized = false;
}

DXClient::~DXClient()
{
	g_CommandRequester->flush();
}

void DXClient::initialize(Window& window)
{
	// tearing supported
	g_TearingSupported = DXFactory::CheckTearingSupport();

	// device
	MS_ComPtr(IDXGIAdapter4) dxgiAdapter4 = DXFactory::GetAdapter(g_UseWarp);
	g_Device = DXFactory::CreateDevice(dxgiAdapter4);

	g_CommandRequester = std::make_unique<DXCommandRequester>(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

	g_SwapChain = DXFactory::CreateSwapChain(window.g_hWnd, g_CommandRequester->getCommandQueue(),
		window.g_ClientWidth, window.g_ClientHeight, DXClient::g_NumFrames);

	g_RTVDescriptorHeap = DXFactory::CreateDescriptorHeap(g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, DXClient::g_NumFrames);
	g_RTVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// backBuffer+RTV in heap and commandAllocator
	DXFactory::UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap, g_BackBuffers, DXClient::g_NumFrames);

	g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

	g_IsInitialized = true;
}

void DXClient::resize(Window& window,  uint32_t width, uint32_t height)
{
	if (window.g_ClientWidth != width || window.g_ClientHeight != height)
	{
		// Don't allow 0 size swap chain back buffers.
		window.resize(width, height);

		// Flush the GPU queue to make sure the swap chain's back buffers
		// are not being referenced by an in-flight command list.
		g_CommandRequester->flush();

		for (int i = 0; i < g_NumFrames; ++i)
		{
			// Any references to the back buffers must be released
			// before the swap chain can be resized.
			g_BackBuffers[i].Reset();
			g_FenceValues[i] = g_FenceValues[g_CurrentBackBufferIndex];
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(g_SwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(g_SwapChain->ResizeBuffers(g_NumFrames, window.g_ClientWidth, window.g_ClientHeight,
			swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

		DXFactory::UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap, g_BackBuffers, g_NumFrames);
	}
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
	auto backBuffer = g_BackBuffers[g_CurrentBackBufferIndex];
	MS_ComPtr(ID3D12GraphicsCommandList) commandList = g_CommandRequester->getCommandList();
	// Clear the render target.
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		commandList->ResourceBarrier(1, &barrier);

		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			g_CurrentBackBufferIndex, g_RTVDescriptorSize);

		commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	}
	// Present
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &barrier);
		g_FenceValues[g_CurrentBackBufferIndex]=g_CommandRequester->executeCommandList(commandList);

		UINT syncInterval = g_VSync ? 1 : 0;
		UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
		ThrowIfFailed(g_SwapChain->Present(syncInterval, presentFlags));

		g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

		g_CommandRequester->waitForFenceValue(g_FenceValues[g_CurrentBackBufferIndex]);
	}
}

