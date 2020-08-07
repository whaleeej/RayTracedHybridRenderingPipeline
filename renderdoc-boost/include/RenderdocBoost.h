#pragma once
#include <renderdoc/renderdoc_app.h>
#include <dxgi1_6.h>
#include <d3d11.h>
#include <d3d12.h>
#define RDCBOOST_NAMESPACE_BEGIN namespace rdcboost\
{
#define RDCBOOST_NAMESPACE_END }


RDCBOOST_NAMESPACE_BEGIN
// API for dx11
HRESULT D3D11CreateDeviceAndSwapChain(
	IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software,
	UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels,
	UINT SDKVersion, const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
	IDXGISwapChain** ppSwapChain, ID3D11Device** ppDevice,
	D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext);

void D3D11EnableRenderDoc(ID3D11Device* pDevice, bool bSwitchToRenderdoc);

// API for dx12
HRESULT  D3D12CreateDevice(
	IDXGIAdapter* pAdapter,
	D3D_FEATURE_LEVEL MinimumFeatureLevel,
	_In_ REFIID riid, // Expected: ID3D12Device
	_COM_Outptr_opt_ void** ppDevice);

HRESULT  CreateSwapChainForHwnd(
	IDXGIFactory2* pDXGIFactory, ID3D12CommandQueue *pCommandQueue, HWND hWnd,
	const DXGI_SWAP_CHAIN_DESC1 *pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
	IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain);

void D3D12EnableRenderDoc(ID3D12Device* pDevice, bool bSwitchToRenderdoc);
	
bool D3D12CallAtEndOfFrame(ID3D12Device* pDevice);

// common wrap for renderdoc
RENDERDOC_API_1_4_1* GetRenderdocAPI();
RDCBOOST_NAMESPACE_END

