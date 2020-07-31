#include "WrappedD3D12DXGISwapChain.h"
#include "WrappedD3D12Resource.h"
#include "WrappedD3D12CommandQueue.h"

RDCBOOST_NAMESPACE_BEGIN

WrappedD3D12DXGISwapChain::WrappedD3D12DXGISwapChain(
	IDXGISwapChain1* pReal, WrappedD3D12CommandQueue* pCommandQueue)
	: m_pRealSwapChain(pReal)
	, m_pWrappedCommandQueue(pCommandQueue)
	, m_Ref(1)
{

	m_ResizeParam.Valid = false;

	// update for swapchain backbuffers
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc;
	m_pRealSwapChain->GetDesc1(&swapchainDesc);
	m_SwapChainBuffers.resize(swapchainDesc.BufferCount);
	for (UINT i = 0; i < swapchainDesc.BufferCount; i++) {
		COMPtr<ID3D12Resource> pvResource ;
		m_pRealSwapChain->GetBuffer(i, IID_PPV_ARGS(&pvResource));

		COMPtr<WrappedD3D12Device> pvDevice = m_pWrappedCommandQueue->GetWrappedDevice();

		WrappedD3D12Resource* pvWrappedRes = new WrappedD3D12Resource(
			pvResource.Get(), pvDevice.Get(), NULL,
			WrappedD3D12Resource::BackBufferWrappedD3D12Resource);
		m_SwapChainBuffers[i] = (pvWrappedRes);
		m_SwapChainBuffers[i]->InitSwapChain(m_pRealSwapChain.Get());
		pvWrappedRes->Release();
	}
}

WrappedD3D12DXGISwapChain::~WrappedD3D12DXGISwapChain()
{
}

void WrappedD3D12DXGISwapChain::SwitchToCommandQueue(ID3D12CommandQueue* pRealCommandQueue)
{
	//这里我们假设创建用的commandQueue并没有改变
	if (pRealCommandQueue == m_pWrappedCommandQueue->GetReal().Get()) {
		return;
	}

	COMPtr<IDXGISwapChain1> pNewSwapChain = CopyToCommandQueue(pRealCommandQueue);

	{ //TODO handle resize/fullscreen
		Assert(pNewSwapChain.Get());

		m_pRealSwapChain = pNewSwapChain;

		if (m_ResizeParam.Valid) //TODO: support resize
		{
			LogError("Unsupportted Resized swapchain buffer");
			HRESULT res = pNewSwapChain->ResizeBuffers(
				m_ResizeParam.BufferCount, m_ResizeParam.Width, m_ResizeParam.Height,
				m_ResizeParam.NewFormat, m_ResizeParam.SwapChainFlags);
			if (FAILED(res))
				LogError("Resize on new swap chain failed.");
		}

		DXGI_SWAP_CHAIN_DESC1 swapchainDesc;
		m_pRealSwapChain->GetDesc1(&swapchainDesc);
		if (m_SwapChainBuffers.size() != swapchainDesc.BufferCount) {
			m_SwapChainBuffers.resize(swapchainDesc.BufferCount);
		}
		for (UINT i = 0; i < swapchainDesc.BufferCount; i++) {
			COMPtr<ID3D12Resource> pvResource = 0;
			m_pRealSwapChain->GetBuffer(i, IID_PPV_ARGS(&pvResource));
			Assert(pvResource.Get());
			if (m_SwapChainBuffers[i].Get()) {
				m_SwapChainBuffers[i]->SwitchToSwapChain(m_pRealSwapChain.Get(), pvResource.Get());
			}
			else {
				COMPtr<WrappedD3D12Device> pvDevice=m_pWrappedCommandQueue->GetWrappedDevice();
				Assert(pvDevice.Get());
				auto pvWrappedRes = new WrappedD3D12Resource(pvResource.Get(), pvDevice.Get(), NULL, WrappedD3D12Resource::BackBufferWrappedD3D12Resource);
				m_SwapChainBuffers[i] = (pvWrappedRes);
				m_SwapChainBuffers[i]->InitSwapChain(m_pRealSwapChain.Get());
				pvWrappedRes->Release();
			}
		}
	}
}

COMPtr<IDXGISwapChain1> WrappedD3D12DXGISwapChain::CopyToCommandQueue(ID3D12CommandQueue* pRealCommandQueue) {
	/************************************************************************/
	/*          only support DxgiFacotry4 and DxgiSwapchain 4 here            */
	/************************************************************************/
	//TODO:dxgi factory compatibility
	IDXGIFactory4* dxgiFactory4;
	UINT createFactoryFlags = 0;
	HRESULT ret = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4));
	if (FAILED(ret)) {
		LogError("the dxgi factory is not suppot by now");
		return NULL;
	}

	IDXGISwapChain1* swapChain1;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	HWND hWnd;
	m_pRealSwapChain->GetDesc1(&swapChainDesc);
	m_pRealSwapChain->GetHwnd(&hWnd);
	Assert(hWnd);
	dxgiFactory4->CreateSwapChainForHwnd(
		pRealCommandQueue,
		hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1);

	//TODO: Disable the Alt+Enter fullscreen toggle feature eg. DXGI_MWA_NO_ALT_ENTER
	ret = dxgiFactory4->MakeWindowAssociation(hWnd, /*DXGI_MWA_NO_ALT_ENTER*/0);
	if (FAILED(ret)) {
		LogError("Failed to make window association with newly created swap chain");
		return NULL;
	}
	return swapChain1;
}


/************************************************************************/
/*                         override                                                                     */
/************************************************************************/
HRESULT  STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::GetBuffer(UINT Buffer, REFIID riid, void **ppSurface)
{
	if (ppSurface == NULL) return E_INVALIDARG;

	if (riid != __uuidof(ID3D12Resource))
	{
		LogError("Unsupported or unrecognised UUID passed to IDXGISwapChain::GetBuffer");
		return E_NOINTERFACE;
	}

	COMPtr<ID3D12Resource>realSurface = NULL;
	HRESULT ret = m_pRealSwapChain->GetBuffer(Buffer, riid, (void**)&realSurface);

	WrappedD3D12Resource *wrappedTex = NULL;
	if (FAILED(ret))
	{
		LogError("Failed to get swapchain backbuffer %d: %08x", Buffer, ret);
		*ppSurface = NULL;
	}
	else
	{
		Assert(realSurface != NULL);
		if (Buffer < m_SwapChainBuffers.size() && m_SwapChainBuffers[Buffer] != NULL)
		{
			if (m_SwapChainBuffers[Buffer]->GetReal().Get() == realSurface.Get())
			{
				wrappedTex = m_SwapChainBuffers[Buffer].Get();
				wrappedTex->AddRef();
			}
			else
			{
				LogWarn("Previous swap chain buffer isn't released by user.");
				m_SwapChainBuffers[Buffer].Reset();
			}
		}

		if (wrappedTex == NULL)
		{
			m_SwapChainBuffers.resize((size_t)Buffer + 1);
			COMPtr<WrappedD3D12Device> pvDevice = m_pWrappedCommandQueue->GetWrappedDevice();
			Assert(pvDevice.Get());
			auto pvWrappedRes = new WrappedD3D12Resource(realSurface.Get(), pvDevice.Get(), NULL, WrappedD3D12Resource::BackBufferWrappedD3D12Resource);
			m_SwapChainBuffers[Buffer] = (pvWrappedRes);
			m_SwapChainBuffers[Buffer]->InitSwapChain(m_pRealSwapChain.Get());
			wrappedTex = m_SwapChainBuffers[Buffer].Get();
			wrappedTex->AddRef();
		}


		if (riid == __uuidof(ID3D12Resource))
			*ppSurface = static_cast<ID3D12Resource*>(wrappedTex);
		else {
			*ppSurface = NULL;
			wrappedTex->Release();
			LogError("Unsupported or unrecognised UUID passed to IDXGISwapChain::GetBuffer");
			return E_NOINTERFACE;
		}
	}
	return ret;
}

HRESULT  STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::SetFullscreenState(BOOL Fullscreen,
	IDXGIOutput *pTarget)
{
	return m_pRealSwapChain->SetFullscreenState(Fullscreen, pTarget);
}

HRESULT  STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::GetFullscreenState(BOOL *pFullscreen,
	IDXGIOutput **ppTarget)
{
	return m_pRealSwapChain->GetFullscreenState(pFullscreen, ppTarget);
}

HRESULT  STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::GetContainingOutput(IDXGIOutput **ppOutput)
{
	return m_pRealSwapChain->GetContainingOutput(ppOutput);
}

HRESULT  STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::SetPrivateData(
	REFGUID Name, UINT DataSize, const void *pData)
{
	LogError("IDXGISwapChain::SetPrivateData is not supported by now.");
	return E_FAIL;
}

HRESULT  STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::SetPrivateDataInterface(
	REFGUID Name, const IUnknown *pUnknown)
{
	LogError("IDXGISwapChain::SetPrivateDataInterface is not supported by now.");
	return E_FAIL;
}

HRESULT  STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::GetPrivateData(
	REFGUID Name, UINT *pDataSize, void *pData)
{
	LogError("IDXGISwapChain::GetPrivateData is not supported by now.");
	return E_FAIL;
}

HRESULT  STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::GetDevice(REFIID riid, void **ppDevice)
{
	*ppDevice = NULL;
	LogError("IDXGISwapChain::GetDevice is not supported by now.");
	return E_FAIL;
}

HRESULT  STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::GetParent(REFIID riid, void **ppParent)
{
	*ppParent = NULL;
	LogError("IDXGISwapChain::GetParent is not supported by now.");
	return E_FAIL;
	return m_pRealSwapChain->GetParent(riid, ppParent);
}

HRESULT  STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::QueryInterface(
	REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	*ppvObject = NULL;
	LogError("IDXGISwapChain::QueryInterface is not supported by now.");
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::Present(UINT SyncInterval, UINT Flags)
{
	return m_pRealSwapChain->Present(SyncInterval, Flags);
}

HRESULT  STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height,
	DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	LogError("SwapChain ResizeBuffers not supportted by now");
	HRESULT res = m_pRealSwapChain->ResizeBuffers(BufferCount, Width, Height,
		NewFormat, SwapChainFlags);

	if (SUCCEEDED(res))
	{
		m_ResizeParam.Valid = true;
		m_ResizeParam.BufferCount = BufferCount;
		m_ResizeParam.Width = Width;
		m_ResizeParam.Height = Height;
		m_ResizeParam.NewFormat = NewFormat;
		m_ResizeParam.SwapChainFlags = SwapChainFlags;
	}

	return res;
}

ULONG  STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::AddRef(void)
{
	return InterlockedIncrement(&m_Ref);
}

ULONG  STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::Release(void)
{
	unsigned int ret = InterlockedDecrement(&m_Ref);
	//TODO: need try to release?
	//if (ret == 1)
	//{
	//	m_pWrappedDevice->TryToRelease();
	//}
	//else 
	if (ret == 0)
	{
		delete this;
	}

	return ret;
}

HRESULT   STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::GetDesc1(
	/* [annotation][out] */
	_Out_  DXGI_SWAP_CHAIN_DESC1 *pDesc) {
	return m_pRealSwapChain->GetDesc1(pDesc);
}

HRESULT  STDMETHODCALLTYPE  WrappedD3D12DXGISwapChain::GetFullscreenDesc(
	/* [annotation][out] */
	_Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc) {
	return m_pRealSwapChain->GetFullscreenDesc(pDesc);
}

HRESULT  STDMETHODCALLTYPE  WrappedD3D12DXGISwapChain::GetHwnd(
	/* [annotation][out] */
	_Out_  HWND *pHwnd) {
	return m_pRealSwapChain->GetHwnd(pHwnd);
}

HRESULT  STDMETHODCALLTYPE  WrappedD3D12DXGISwapChain::GetCoreWindow(
	/* [annotation][in] */
	_In_  REFIID refiid,
	/* [annotation][out] */
	_COM_Outptr_  void **ppUnk) {
	return m_pRealSwapChain->GetCoreWindow(refiid, ppUnk);
}

HRESULT  STDMETHODCALLTYPE  WrappedD3D12DXGISwapChain::Present1(
	/* [in] */ UINT SyncInterval,
	/* [in] */ UINT PresentFlags,
	/* [annotation][in] */
	_In_  const DXGI_PRESENT_PARAMETERS *pPresentParameters) {
	return m_pRealSwapChain->Present1(SyncInterval,PresentFlags,pPresentParameters);
}

BOOL STDMETHODCALLTYPE   WrappedD3D12DXGISwapChain::IsTemporaryMonoSupported(void) {
	return m_pRealSwapChain->IsTemporaryMonoSupported();
}

HRESULT  STDMETHODCALLTYPE  WrappedD3D12DXGISwapChain::GetRestrictToOutput(
	/* [annotation][out] */
	_Out_  IDXGIOutput **ppRestrictToOutput) {
	return m_pRealSwapChain->GetRestrictToOutput(ppRestrictToOutput);
}

HRESULT  STDMETHODCALLTYPE  WrappedD3D12DXGISwapChain::SetBackgroundColor(
	/* [annotation][in] */
	_In_  const DXGI_RGBA *pColor) {
	return m_pRealSwapChain->SetBackgroundColor(pColor);
}

HRESULT  STDMETHODCALLTYPE  WrappedD3D12DXGISwapChain::GetBackgroundColor(
	/* [annotation][out] */
	_Out_  DXGI_RGBA *pColor) {
	return m_pRealSwapChain->GetBackgroundColor(pColor);
}

HRESULT  STDMETHODCALLTYPE  WrappedD3D12DXGISwapChain::SetRotation(
	/* [annotation][in] */
	_In_  DXGI_MODE_ROTATION Rotation) {
	return m_pRealSwapChain->SetRotation(Rotation);
}

HRESULT  STDMETHODCALLTYPE  WrappedD3D12DXGISwapChain::GetRotation(
	/* [annotation][out] */
	_Out_  DXGI_MODE_ROTATION *pRotation) {
	return m_pRealSwapChain->GetRotation(pRotation);
}


RDCBOOST_NAMESPACE_END