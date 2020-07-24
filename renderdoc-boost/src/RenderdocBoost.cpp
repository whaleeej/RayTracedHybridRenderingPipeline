#include "RenderdocBoost.h"
#include "WrappedD3D11DXGISwapChain.h"
#include "WrappedD3D11Device.h"
#include "WrappedD3D11Context.h"
#include "D3D11DeviceCreateParams.h"
#include <set>

RDCBOOST_NAMESPACE_BEGIN

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

typedef 	HRESULT (WINAPI* tD3D12CreateDevice)(
	IDXGIAdapter* pAdapter,
	D3D_FEATURE_LEVEL MinimumFeatureLevel,
	_In_ REFIID riid, // Expected: ID3D12Device
	_COM_Outptr_opt_ void** ppDevice);

tD3D12CreateDevice pfnRenderdocCreateDevice;
tD3D12CreateDevice pfnD3D12CreateDevice;

static HMODULE sRdcModule;
static RENDERDOC_API_1_0_1* sRdcAPI = NULL;

//*************************************d3d11*************************************//
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
	pfnRenderdocCreateDevice = (tD3D12CreateDevice)GetProcAddress(sRdcModule,
		"RENDERDOC_CreateWrappedD3D12Device");

	if (pfnRenderdocCreateDeviceAndSwapChain == NULL)
	{
		LogError("Can't GetProcAddress of RENDERDOC_CreateWrappedD3D11DeviceAndSwapChain");
		return false;
	}
	if (pfnRenderdocCreateDevice == NULL) {
		LogError("Can't GetProcAddress of RENDERDOC_CreateWrappedD3D12Device");
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
	WrappedD3D11DXGISwapChain* wrappedSwapChain = NULL;

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
		wrappedSwapChain = new WrappedD3D11DXGISwapChain(pRealSwapChain, wrappedDevice);
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

void D3D11EnableRenderDoc(ID3D11Device* pDevice, bool bSwitchToRdc)
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
//*************************************d3d11*************************************//

//*************************************d3d12*************************************//
HRESULT  D3D12CreateDevice(
	IDXGIAdapter* pAdapter,
	D3D_FEATURE_LEVEL MinimumFeatureLevel,
	_In_ REFIID riid, // Expected: ID3D12Device
	_COM_Outptr_opt_ void** ppDevice) {
	if (pfnD3D12CreateDevice == NULL) {
		HMODULE d3d12Module = GetModuleHandle("d3d12.dll");
		pfnD3D12CreateDevice = (tD3D12CreateDevice)GetProcAddress(d3d12Module, "D3D12CreateDevice");
	}

	ID3D12Device* pRealDevice = NULL;
	HRESULT res = pfnD3D12CreateDevice(pAdapter, MinimumFeatureLevel, IID_PPV_ARGS(&pRealDevice));
	return 0;
}

void D3D12EnableRenderDoc(ID3D12Device* pDevice, bool bSwitchToRenderdoc) {

}
//*************************************d3d12*************************************//


RENDERDOC_API_1_0_1* GetRenderdocAPI()
{
	return sRdcAPI;
}

RDCBOOST_NAMESPACE_END

