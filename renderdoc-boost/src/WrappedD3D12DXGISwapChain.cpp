#include "WrappedD3D12Heap.h"
#include "WrappedD3D12DXGISwapChain.h"
#include "WrappedD3D12Resource.h"
#include "WrappedD3D12CommandQueue.h"

RDCBOOST_NAMESPACE_BEGIN

WrappedD3D12DXGISwapChain::WrappedD3D12DXGISwapChain(
	IDXGISwapChain1* pReal, WrappedD3D12CommandQueue* pCommandQueue)
	: m_pRealSwapChain(pReal)
	, m_pWrappedCommandQueue(pCommandQueue)
	, m_Ref(1)
	, m_HighestVersion(1)
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
			pvResource.Get(), pvDevice.Get());
		m_SwapChainBuffers[i] = (pvWrappedRes);
		m_SwapChainBuffers[i]->InitSwapChain(m_pRealSwapChain.Get());
		pvWrappedRes->Release();
	}

	//check idxgi swapchain support
	COMPtr<IDXGISwapChain2> pSwapchain2;
	HRESULT ret = m_pRealSwapChain.As(&pSwapchain2);
	if (!FAILED(ret))
		m_HighestVersion++;
	else return;
	COMPtr<IDXGISwapChain3> pSwapchain3;
	ret = m_pRealSwapChain.As(&pSwapchain3);
	if (!FAILED(ret))
		m_HighestVersion++;
	else return;
	COMPtr<IDXGISwapChain4> pSwapchain4;
	ret = m_pRealSwapChain.As(&pSwapchain4);
	if (!FAILED(ret))
		m_HighestVersion++;
	else return;
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
	auto oldIndex = GetCurrentBackBufferIndex();
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
				auto pvWrappedRes = new WrappedD3D12Resource(pvResource.Get(), pvDevice.Get());
				m_SwapChainBuffers[i] = (pvWrappedRes);
				m_SwapChainBuffers[i]->InitSwapChain(m_pRealSwapChain.Get());
				pvWrappedRes->Release();
			}
		}
	}
	auto currentIndex = GetCurrentBackBufferIndex();
	while (currentIndex != oldIndex) {
		Present(0, 0);
		currentIndex = GetCurrentBackBufferIndex();
	}
}

COMPtr<IDXGISwapChain1> WrappedD3D12DXGISwapChain::CopyToCommandQueue(ID3D12CommandQueue* pRealCommandQueue) {
	IDXGIFactory4* dxgiFactory4; //TODO: higher version of dxgi? or low compatibility
	UINT createFactoryFlags = 0;
	HRESULT ret = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4));
	if (FAILED(ret)) {
		LogError("the dxgi factory 4 is not suppot by now");
		return NULL;
	}

	COMPtr<IDXGISwapChain1> swapChain1;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	HWND hWnd;
	m_pRealSwapChain->GetDesc1(&swapChainDesc);
	m_pRealSwapChain->GetHwnd(&hWnd);
	Assert(hWnd);
	
	// clear swapchain refs
	for (int i = 0; i < m_SwapChainBuffers.size(); i++) {
		m_SwapChainBuffers[i]->ResetBySwapChain();
	}
	m_pRealSwapChain.Reset();

	dxgiFactory4->CreateSwapChainForHwnd(
		pRealCommandQueue,
		hWnd,
		&swapChainDesc,
		nullptr,//TDOO fullscreen desc
		nullptr,//TODO dxgi output
		&swapChain1);

	ret = dxgiFactory4->MakeWindowAssociation(hWnd, 0);//TODO: association flag
	if (FAILED(ret)) {
		LogError("Failed to make window association with newly created swap chain");
		return NULL;
	}

	m_HighestVersion = 1;
	COMPtr<IDXGISwapChain2> pSwapchain2;
	ret = swapChain1.As(&pSwapchain2);
	if (!FAILED(ret))
		m_HighestVersion++;
	else return swapChain1;
	COMPtr<IDXGISwapChain3> pSwapchain3;
	ret = swapChain1.As(&pSwapchain3);
	if (!FAILED(ret))
		m_HighestVersion++;
	else return swapChain1;
	COMPtr<IDXGISwapChain4> pSwapchain4;
	ret = swapChain1.As(&pSwapchain4);
	if (!FAILED(ret))
		m_HighestVersion++;
	return swapChain1;
}

bool WrappedD3D12DXGISwapChain::isResourceExist(WrappedD3D12Resource* pWrappedResource, ID3D12Resource* pRealResource) {
	auto find1 = m_BackRefs_Resource_Reflection.find(pWrappedResource);
	if (find1 != m_BackRefs_Resource_Reflection.end() && find1->second == pRealResource)
		return true;
	return false;
}

void WrappedD3D12DXGISwapChain::cacheResourceReflectionToOldReal() {
	for (auto it = m_SwapChainBuffers.begin(); it != m_SwapChainBuffers.end(); it++) {
		m_BackRefs_Resource_Reflection.emplace(it->Get(),
			static_cast<ID3D12Resource*>((*it)->GetReal().Get()));
	}
};

void WrappedD3D12DXGISwapChain::clearResourceReflectionToOldReal() {
	m_BackRefs_Resource_Reflection.clear();
}

/************************************************************************/
/*                         override                                                                     */
/************************************************************************/
HRESULT  STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::GetBuffer(UINT Buffer, REFIID riid, void **ppSurface)
{
	Assert(riid == __uuidof(ID3D12Resource));
	if (ppSurface == NULL) return E_INVALIDARG;

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
			auto pvWrappedRes = new WrappedD3D12Resource(realSurface.Get(), pvDevice.Get());
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
	if (riid == __uuidof(IDXGISwapChain)) {
		if (checkVersionSupport(0)) {
			*ppvObject = static_cast<IDXGISwapChain*>(this);
			AddRef();
			return S_OK;
		}
		else
			return E_FAIL;
	}
	else if (riid == __uuidof(IDXGISwapChain1)) {
		if (checkVersionSupport(1)) {
			*ppvObject = static_cast<IDXGISwapChain1*>(this);
			AddRef();
			return S_OK;
		}
		else
			return E_FAIL;
	}
	else if (riid == __uuidof(IDXGISwapChain2)) {
		if (checkVersionSupport(2)) {
			*ppvObject = static_cast<IDXGISwapChain2*>(this);
			AddRef();
			return S_OK;
		}
		else
			return E_FAIL;
	}
	else if (riid == __uuidof(IDXGISwapChain3)) {
		if (checkVersionSupport(3)) {
			*ppvObject = static_cast<IDXGISwapChain3*>(this);
			AddRef();
			return S_OK;
		}
		else
			return E_FAIL;
	}
	else if (riid == __uuidof(IDXGISwapChain4)) {
		if (checkVersionSupport(4)) {
			*ppvObject = static_cast<IDXGISwapChain4*>(this);
			AddRef();
			return S_OK;
		}
		else
			return E_FAIL;
	}
	else if (riid == __uuidof(IDXGIDeviceSubObject)) {
		*ppvObject = static_cast<IDXGIDeviceSubObject*>(this);
		AddRef();
		return S_OK;
	}
	else if (riid == __uuidof(IDXGIObject)) {
		*ppvObject = static_cast<IDXGIObject*>(this);
		AddRef();
		return S_OK;
	}
	else if (riid == __uuidof(IUnknown)) {
		*ppvObject = static_cast<IUnknown*>(this);
		AddRef();
		return S_OK;
	}
	LogError("Invalid query for this interface");
	return GetReal1()->QueryInterface(riid, ppvObject);
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


//override for swapchain2
HRESULT STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::SetSourceSize(
	UINT Width,
	UINT Height) {
	if (!checkVersionSupport(2))
		return E_FAIL;
	return GetReal2()->SetSourceSize(Width, Height);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::GetSourceSize(
	/* [annotation][out] */
	_Out_  UINT* pWidth,
	/* [annotation][out] */
	_Out_  UINT* pHeight) {
	if (!checkVersionSupport(2))
		return E_FAIL;
	return GetReal2()->GetSourceSize(
		 pWidth,
		 pHeight);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::SetMaximumFrameLatency(
	UINT MaxLatency) {
	if (!checkVersionSupport(2))
		return E_FAIL;
	return GetReal2()->SetMaximumFrameLatency(
		 MaxLatency);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::GetMaximumFrameLatency(
	/* [annotation][out] */
	_Out_  UINT* pMaxLatency) {
	if (!checkVersionSupport(2))
		return E_FAIL;
	return GetReal2()->GetMaximumFrameLatency(
		pMaxLatency);
}

HANDLE STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::GetFrameLatencyWaitableObject(void) {
	if (!checkVersionSupport(2)) {
		LogError("IDXGI SwapChain2 not supportted");
		return 0;
	}
	
	return GetReal2()->GetFrameLatencyWaitableObject();
}

HRESULT STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::SetMatrixTransform(
	const DXGI_MATRIX_3X2_F* pMatrix) {
	if (!checkVersionSupport(2))
		return E_FAIL;
	return GetReal2()->SetMatrixTransform(
		pMatrix);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::GetMatrixTransform(
	/* [annotation][out] */
	_Out_  DXGI_MATRIX_3X2_F* pMatrix) {
	if (!checkVersionSupport(2))
		return E_FAIL;
	return GetReal2()->GetMatrixTransform(
		pMatrix);
}

//override swapchain3
UINT STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::GetCurrentBackBufferIndex(void) {
	if (!checkVersionSupport(3))
	{
		LogError("idxgi swapchain3 not supportted");
		return 0;
	}
	return GetReal3()->GetCurrentBackBufferIndex();
}

HRESULT STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::CheckColorSpaceSupport(
	/* [annotation][in] */
	_In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
	/* [annotation][out] */
	_Out_  UINT* pColorSpaceSupport) {
	if (!checkVersionSupport(3))
		return E_FAIL;
	return GetReal3()->CheckColorSpaceSupport(
		ColorSpace,
		pColorSpaceSupport);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::SetColorSpace1(
	/* [annotation][in] */
	_In_  DXGI_COLOR_SPACE_TYPE ColorSpace) {
	if (!checkVersionSupport(3))
		return E_FAIL;
	return GetReal3()->SetColorSpace1(ColorSpace);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::ResizeBuffers1(
	/* [annotation][in] */
	_In_  UINT BufferCount,
	/* [annotation][in] */
	_In_  UINT Width,
	/* [annotation][in] */
	_In_  UINT Height,
	/* [annotation][in] */
	_In_  DXGI_FORMAT Format,
	/* [annotation][in] */
	_In_  UINT SwapChainFlags,
	/* [annotation][in] */
	_In_reads_(BufferCount)  const UINT* pCreationNodeMask,
	/* [annotation][in] */
	_In_reads_(BufferCount)  IUnknown* const* ppPresentQueue) {
	if (!checkVersionSupport(3))
		return E_FAIL;
	return GetReal3()->ResizeBuffers1(
		BufferCount,
		Width,
		Height,
		Format,
		SwapChainFlags,
		pCreationNodeMask,
		ppPresentQueue);
}

//override swapchain4
HRESULT STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::SetHDRMetaData(
	/* [annotation][in] */
	_In_  DXGI_HDR_METADATA_TYPE Type,
	/* [annotation][in] */
	_In_  UINT Size,
	/* [annotation][size_is][in] */
	_In_reads_opt_(Size)  void* pMetaData) {
	if (!checkVersionSupport(4))
		return E_FAIL;
	return GetReal4()->SetHDRMetaData(
		Type,
		Size,
		pMetaData);
}

RDCBOOST_NAMESPACE_END