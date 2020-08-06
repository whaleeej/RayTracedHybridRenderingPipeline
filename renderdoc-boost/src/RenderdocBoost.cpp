#include <set>
#include "RenderdocBoost.h"
#include "DeviceCreateParams.h"
//d3d11 wrapper
#include "WrappedD3D11DXGISwapChain.h"
#include "WrappedD3D11Device.h"
#include "WrappedD3D11Context.h"
//d3d12 wrapper
#include "WrappedD3D12Device.h"
#include "WrappedD3D12CommandQueue.h"

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

tD3D11CreateDeviceAndSwapChain pfnRenderdocD3D11CreateDeviceAndSwapChain;
tD3D11CreateDeviceAndSwapChain pfnD3D11CreateDeviceAndSwapChain;

typedef 	HRESULT (WINAPI* tD3D12CreateDevice)(
	_In_opt_ IUnknown* pAdapter,
	D3D_FEATURE_LEVEL MinimumFeatureLevel,
	_In_ REFIID riid, // Expected: ID3D12Device
	_COM_Outptr_opt_ void** ppDevice);


tD3D12CreateDevice pfnRenderdocD3D12CreateDevice;
tD3D12CreateDevice pfnD3D12CreateDevice;

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

	pfnRenderdocD3D11CreateDeviceAndSwapChain =
		(tD3D11CreateDeviceAndSwapChain)GetProcAddress(sRdcModule, "RENDERDOC_CreateWrappedD3D11DeviceAndSwapChain");
	pfnRenderdocD3D12CreateDevice = (tD3D12CreateDevice)GetProcAddress(sRdcModule,
		"RENDERDOC_CreateWrappedD3D12Device");

	if (pfnRenderdocD3D11CreateDeviceAndSwapChain == NULL)
	{
		LogError("Can't GetProcAddress of RENDERDOC_CreateWrappedD3D11DeviceAndSwapChain");
		return false;
	}
	if (pfnRenderdocD3D12CreateDevice == NULL) {
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

//*************************************d3d11*************************************//
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
		bSwitchToRdc ? pfnRenderdocD3D11CreateDeviceAndSwapChain : pfnD3D11CreateDeviceAndSwapChain;

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
	Assert(riid == __uuidof(ID3D12Device));

	if (pfnD3D12CreateDevice == NULL) {
		HMODULE d3d12Module = GetModuleHandle("d3d12.dll");
		Assert(d3d12Module);
		pfnD3D12CreateDevice = (tD3D12CreateDevice)GetProcAddress(d3d12Module, "D3D12CreateDevice");
	}

	COMPtr<ID3D12Device> pRealDevice = NULL;
	HRESULT ret = pfnD3D12CreateDevice(pAdapter, MinimumFeatureLevel, IID_PPV_ARGS(&pRealDevice));

	WrappedD3D12Device* pWrappedDevice = NULL;
	if (!FAILED(ret)) {
		pWrappedDevice = new WrappedD3D12Device(pRealDevice.Get(), 
			SDeviceCreateParams(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, 0, 0, &MinimumFeatureLevel, 1, 0, 0));
	}

	if (pWrappedDevice&&ppDevice) {
		*ppDevice = static_cast<ID3D12Device*>(pWrappedDevice);
	}
	else if (pWrappedDevice) {
		pWrappedDevice->Release();
	}
	else
		LogError("Unable to create wrapped ID3D12Device");

	return ret;
}

HRESULT  CreateSwapChainForHwnd(
	IDXGIFactory2* pDXGIFactory, ID3D12CommandQueue *pCommandQueue, HWND hWnd,
	const DXGI_SWAP_CHAIN_DESC1 *pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
	IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain)
{
	return static_cast<WrappedD3D12CommandQueue*>(pCommandQueue)->createSwapChain(
		pDXGIFactory, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
}

static bool d3d12InRenderdocFlg = false;

void D3D12EnableRenderDoc(ID3D12Device* pDevice, bool bSwitchToRenderdoc) {
	if (bSwitchToRenderdoc && !InitRenderDoc())
	{
		d3d12InRenderdocFlg = false;
		LogError("Can't enable renderdoc because renderdoc is not present.");
		return;
	}
	else if (!bSwitchToRenderdoc) {
		d3d12InRenderdocFlg = false;
	}
	else {
		d3d12InRenderdocFlg = true;
	}
}

bool D3D12CallAtEndOfFrame(ID3D12Device* pDevice){

	WrappedD3D12Device* pWrappedDevice = static_cast<WrappedD3D12Device*>(pDevice);
	if (!pWrappedDevice || (pWrappedDevice->isRenderDocDevice() == d3d12InRenderdocFlg))
		return false;

	tD3D12CreateDevice pfnCreateDevice = 
		d3d12InRenderdocFlg ? pfnRenderdocD3D12CreateDevice : pfnD3D12CreateDevice;

	const SDeviceCreateParams params = pWrappedDevice->GetDeviceCreateParams();
	if (params.FeatureLevels < 1)
	{
		LogError("Can't switch to device without any feature level");
		return  false;
	}

	COMPtr<ID3D12Device> pRealDevice = NULL;
	
	HRESULT ret = pfnCreateDevice(params.pAdapter, (params.pFeatureLevels)[0], IID_PPV_ARGS(&pRealDevice));

	if (FAILED(ret)) {
		LogError("Create new device failed.");
		return false;
	}

	pWrappedDevice->SetAsRenderDocDevice(d3d12InRenderdocFlg);
	pWrappedDevice->SwitchToDeviceRdc(pRealDevice.Get());
	return true;
}
//*************************************d3d12*************************************//

RENDERDOC_API_1_0_1* GetRenderdocAPI()
{
	return sRdcAPI;
}

RDCBOOST_NAMESPACE_END

