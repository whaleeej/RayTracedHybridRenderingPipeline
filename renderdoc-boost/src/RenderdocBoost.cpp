#include "RenderdocBoost.h"
#include "WrappedDXGISwapChain.h"
#include "WrappedD3D11Device.h"
#include "WrappedD3D11Context.h"
#include "DeviceCreateParams.h"
#include <set>

namespace rdcboost
{

	typedef HRESULT(WINAPI *tD3D11CreateDeviceAndSwapChain)(
		IDXGIAdapter *pAdapter,
		D3D_DRIVER_TYPE DriverType,
		HMODULE Software,
		UINT Flags,
		const D3D_FEATURE_LEVEL *pFeatureLevels,
		UINT FeatureLevels,
		UINT SDKVersion,
		const DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
		IDXGISwapChain **ppSwapChain,
		ID3D11Device **ppDevice,
		D3D_FEATURE_LEVEL *pFeatureLevel,
		ID3D11DeviceContext **ppImmediateContext);

	tD3D11CreateDeviceAndSwapChain pfnRenderdocCreateDeviceAndSwapChain;
	tD3D11CreateDeviceAndSwapChain pfnD3D11CreateDeviceAndSwapChain;

	static HMODULE sRdcModule;
	static RENDERDOC_API_1_0_1* sRdcAPI = NULL;

	static bool InitRenderDoc()
	{
		if (sRdcModule != NULL)
			return true;

		printf("Loading renderdoc.dll ...\n");
		sRdcModule = LoadLibrary("renderdoc.dll");
		if (sRdcModule == 0)
		{
			DWORD errorCode = GetLastError();
			LogError("Load renderdoc.dll failed(ERROR CODE: %d).", (int) errorCode);
			return false;
		}

		pfnRenderdocCreateDeviceAndSwapChain =
			(tD3D11CreateDeviceAndSwapChain)GetProcAddress(sRdcModule, "RENDERDOC_CreateWrappedD3D11DeviceAndSwapChain");

		if (pfnRenderdocCreateDeviceAndSwapChain == NULL)
		{
			LogError("Can't GetProcAddress of RENDERDOC_CreateWrappedD3D11DeviceAndSwapChain");
			return false;
		}

		pRENDERDOC_GetAPI pRenderDocGetAPIFn = 
			(pRENDERDOC_GetAPI) GetProcAddress(sRdcModule, "RENDERDOC_GetAPI");

		if (pRenderDocGetAPIFn == NULL)
		{
			LogError("Can't find RENDERDOC_GetAPI in renderdoc.dll.");
			return true;
		}

		if (pRenderDocGetAPIFn(eRENDERDOC_API_Version_1_0_1, (void**)&sRdcAPI) == 0 ||
			sRdcAPI == NULL)
		{
			LogError("Get API from renderdoc failed.");
			return true;
		}

		// TODO_wzq when to free?
// 		FreeLibrary(rdcModule);
		return true;
	}

	HRESULT D3D11CreateDeviceAndSwapChain(
		IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, 
		UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, 
		UINT SDKVersion, const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc, 
		IDXGISwapChain** ppSwapChain, ID3D11Device** ppDevice, 
		D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext)
	{
		if (pfnD3D11CreateDeviceAndSwapChain == NULL)
		{
			HMODULE d3d11Module = GetModuleHandle("d3d11.dll");
			pfnD3D11CreateDeviceAndSwapChain = 
				(tD3D11CreateDeviceAndSwapChain)GetProcAddress(d3d11Module, "D3D11CreateDeviceAndSwapChain");
		}

		SDeviceCreateParams params(pAdapter, DriverType, Software, Flags, 
								   pFeatureLevels, FeatureLevels, SDKVersion, pSwapChainDesc);
		
		IDXGISwapChain* pRealSwapChain = NULL;
		ID3D11Device* pRealDevice = NULL;
		HRESULT res = pfnD3D11CreateDeviceAndSwapChain(
							pAdapter, DriverType, Software, Flags, pFeatureLevels,
							FeatureLevels, SDKVersion, pSwapChainDesc, &pRealSwapChain,
							&pRealDevice, pFeatureLevel, NULL);

		WrappedD3D11Device* wrappedDevice = NULL;
		WrappedDXGISwapChain* wrappedSwapChain = NULL;

		if (pRealDevice)
		{
			wrappedDevice = new WrappedD3D11Device(pRealDevice, params);
			pRealDevice->Release();
			wrappedDevice->SetAsRenderDocDevice(false);

			if (ppImmediateContext)
				wrappedDevice->GetImmediateContext(ppImmediateContext);
		}

		if (pRealSwapChain)
		{
			wrappedSwapChain = new WrappedDXGISwapChain(pRealSwapChain, wrappedDevice);
			pRealSwapChain->Release();
		}

		if (wrappedDevice && wrappedSwapChain)
			wrappedDevice->InitSwapChain(wrappedSwapChain);

		if (ppDevice)
			*ppDevice = wrappedDevice;
		else if (wrappedDevice)
			wrappedDevice->Release();

		if (ppSwapChain)
			*ppSwapChain = wrappedSwapChain;
		else if (wrappedSwapChain)
			wrappedSwapChain->Release();

		return res;
	}

	void EnableRenderDoc(ID3D11Device* pDevice, bool bSwitchToRdc)
	{
		if (bSwitchToRdc && !InitRenderDoc())
		{
			LogError("Can't enable renderdoc because renderdoc is not present.");
			return;
		}

		WrappedD3D11Device* pWrappedDevice = static_cast<WrappedD3D11Device*>(pDevice);
		if (pDevice == NULL || (pWrappedDevice->IsRenderDocDevice() == bSwitchToRdc))
			return;

		const SDeviceCreateParams& params = pWrappedDevice->GetDeviceCreateParams();

		tD3D11CreateDeviceAndSwapChain pfnCreateDeviceAndSwapChain =
			bSwitchToRdc ? pfnRenderdocCreateDeviceAndSwapChain : pfnD3D11CreateDeviceAndSwapChain;

		IDXGISwapChain* pRealSwapChain = NULL;
		ID3D11Device* pRealDevice = NULL;
		HRESULT res = pfnCreateDeviceAndSwapChain(
							params.pAdapter, params.DriverType, params.Software,
							params.Flags, params.pFeatureLevels, params.FeatureLevels,
							params.SDKVersion, params.SwapChainDesc,
							&pRealSwapChain, &pRealDevice, NULL, NULL);

		if (FAILED(res))
		{
			LogError("Create new device failed.");
			return;
		}

		pWrappedDevice->SwitchToDevice(pRealDevice, pRealSwapChain);
		pRealSwapChain->Release();
		pRealDevice->Release();
		pWrappedDevice->SetAsRenderDocDevice(bSwitchToRdc);
	}

	RENDERDOC_API_1_0_1* GetRenderdocAPI()
	{
		return sRdcAPI;
	}

}

