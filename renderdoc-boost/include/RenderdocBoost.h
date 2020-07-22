#pragma once
#include <d3d11.h>
#include <renderdoc/renderdoc_app.h>

namespace rdcboost
{
	HRESULT D3D11CreateDeviceAndSwapChain(
		IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software,
		UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels,
		UINT SDKVersion, const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
		IDXGISwapChain** ppSwapChain, ID3D11Device** ppDevice,
		D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext);

	void EnableRenderDoc(ID3D11Device* pDevice, bool bSwitchToRenderdoc);

	RENDERDOC_API_1_0_1* GetRenderdocAPI();
}

