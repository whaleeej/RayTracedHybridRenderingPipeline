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
	m_pRealSwapChain->AddRef();
	m_pWrappedCommandQueue->AddRef();

	m_ResizeParam.Valid = false;

	// update for swapchain backbuffers
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc;
	m_pRealSwapChain->GetDesc1(&swapchainDesc);
	m_SwapChainBuffers.resize(swapchainDesc.BufferCount);
	for (int i = 0; i < swapchainDesc.BufferCount; i++) {
		ID3D12Resource* pvResource ;
		m_pRealSwapChain->GetBuffer(i, IID_PPV_ARGS(&pvResource));
		WrappedD3D12Device* pvDevice;
		m_pWrappedCommandQueue->GetDevice(__uuidof(ID3D12Device), (void**)&pvDevice);
		m_SwapChainBuffers[i] = new WrappedD3D12Resource(pvResource, pvDevice);
		m_SwapChainBuffers[i]->InitSwapChain(m_pRealSwapChain);
		pvResource->Release(); pvDevice->Release();
	}
}

WrappedD3D12DXGISwapChain::~WrappedD3D12DXGISwapChain()
{
	m_pRealSwapChain->Release();
	m_pWrappedCommandQueue->Release();
}

void WrappedD3D12DXGISwapChain::SwitchToCommandQueue(ID3D12CommandQueue* pRealCommandQueue)
{
	if (pRealCommandQueue == m_pWrappedCommandQueue->GetReal()) {
		return;
	}

	IDXGISwapChain1* pNewSwapChain = CopyToCommandQueue(pRealCommandQueue);

	{ //TODO handle resize/fullscreen
		Assert(pNewSwapChain);

		m_pRealSwapChain->Release();
		m_pRealSwapChain = pNewSwapChain;
		m_pRealSwapChain->AddRef();

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
		for (int i = 0; i < swapchainDesc.BufferCount; i++) {
			ID3D12Resource* pvResource = 0;
			m_pRealSwapChain->GetBuffer(i, IID_PPV_ARGS(&pvResource));
			Assert(pvResource);
			if (m_SwapChainBuffers[i]) {
				m_SwapChainBuffers[i]->SwitchToSwapChain(m_pRealSwapChain, pvResource);
			}
			else {
				WrappedD3D12Device* pvDevice;
				m_pWrappedCommandQueue->GetDevice(__uuidof(ID3D12Device), (void**)& pvDevice);
				Assert(pvDevice);
				m_SwapChainBuffers[i] = new WrappedD3D12Resource(pvResource, pvDevice);
				m_SwapChainBuffers[i]->InitSwapChain(m_pRealSwapChain);
				pvDevice->Release();
			}
			pvResource->Release();
		}
	}
}

IDXGISwapChain1* WrappedD3D12DXGISwapChain::CopyToCommandQueue(ID3D12CommandQueue* pRealCommandQueue) {
	/************************************************************************/
	/*          only support DxgiFacotry4 and DxgiSwapchain 4 here            */
	/************************************************************************/
	//TODO:dxgi factory suppot more?
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
	//TODO: dxgi swapchain1?2?3?4?
	static_cast<IDXGISwapChain1*>(m_pRealSwapChain)->GetDesc1(&swapChainDesc);
	static_cast<IDXGISwapChain1*>(m_pRealSwapChain)->GetHwnd(&hWnd);
	Assert(hWnd);
	dxgiFactory4->CreateSwapChainForHwnd(
		pRealCommandQueue,
		hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1);

	//TODO: Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	// need to depend on situation!!!
	ret = dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(ret)) {
		LogError("Failed to make window association with newly created swap chain");
		return NULL;
	}


	//IDXGISwapChain4* swapChain4;
	//ret = swapChain1->QueryInterface(__uuidof(IDXGISwapChain4), (void**)&swapChain4);
	//if (FAILED(ret)) {
	//	LogError("Fail to As create swapchain1 to swapchain4");
	//	return NULL;
	//}

	//return static_cast<IDXGISwapChain1*>(swapChain4);

	return swapChain1;
}


/************************************************************************/
/*                         override                                                                     */
/************************************************************************/
HRESULT WrappedD3D12DXGISwapChain::GetBuffer(UINT Buffer, REFIID riid, void **ppSurface)
{
	if (ppSurface == NULL) return E_INVALIDARG;

	if (riid != __uuidof(ID3D12Resource))
	{
		LogError("Unsupported or unrecognised UUID passed to IDXGISwapChain::GetBuffer");
		return E_NOINTERFACE;
	}

	ID3D12Resource *realSurface = NULL;
	HRESULT ret = m_pRealSwapChain->GetBuffer(Buffer, riid, (void**)&realSurface);

	ID3D12Resource *wrappedTex = NULL;
	if (FAILED(ret))
	{
		LogError("Failed to get swapchain backbuffer %d: %08x", Buffer, ret);
		*ppSurface = NULL;
	}
	else
	{
		//wrappedTex = m_pWrappedDevice->GetWrappedSwapChainBuffer(Buffer, realSurface);

		Assert(realSurface != NULL);
		if (Buffer < m_SwapChainBuffers.size() && m_SwapChainBuffers[Buffer] != NULL)
		{
			if (m_SwapChainBuffers[Buffer]->GetReal() == realSurface)
			{
				wrappedTex = m_SwapChainBuffers[Buffer];
				wrappedTex->AddRef();
			}
			else
			{
				LogWarn("Previous swap chain buffer isn't released by user.");
				m_SwapChainBuffers[Buffer] = NULL;
			}
		}

		if (wrappedTex == NULL)
		{
			m_SwapChainBuffers.resize((size_t)Buffer + 1);
			WrappedD3D12Device* pvDevice;
			m_pWrappedCommandQueue->GetDevice(__uuidof(ID3D12Device), (void**)&pvDevice);
			Assert(pvDevice);
			m_SwapChainBuffers[Buffer] = new WrappedD3D12Resource(realSurface, pvDevice);
			m_SwapChainBuffers[Buffer]->InitSwapChain(m_pRealSwapChain);
			wrappedTex = m_SwapChainBuffers[Buffer];
		}


		if (riid == __uuidof(ID3D12Resource))
			*ppSurface = static_cast<ID3D12Resource*>(wrappedTex);
	}

	if (realSurface != NULL)
	{
		realSurface->Release();
		realSurface = NULL;
	}

	return ret;
}

HRESULT WrappedD3D12DXGISwapChain::SetFullscreenState(BOOL Fullscreen,
	IDXGIOutput *pTarget)
{
	return m_pRealSwapChain->SetFullscreenState(Fullscreen, pTarget);
}

HRESULT WrappedD3D12DXGISwapChain::GetFullscreenState(BOOL *pFullscreen,
	IDXGIOutput **ppTarget)
{
	return m_pRealSwapChain->GetFullscreenState(pFullscreen, ppTarget);
}

HRESULT WrappedD3D12DXGISwapChain::GetContainingOutput(IDXGIOutput **ppOutput)
{
	return m_pRealSwapChain->GetContainingOutput(ppOutput);
}

HRESULT WrappedD3D12DXGISwapChain::SetPrivateData(
	REFGUID Name, UINT DataSize, const void *pData)
{
	LogError("IDXGISwapChain::SetPrivateData is not supported by now.");
	return E_FAIL;
}

HRESULT WrappedD3D12DXGISwapChain::SetPrivateDataInterface(
	REFGUID Name, const IUnknown *pUnknown)
{
	LogError("IDXGISwapChain::SetPrivateDataInterface is not supported by now.");
	return E_FAIL;
}

HRESULT WrappedD3D12DXGISwapChain::GetPrivateData(
	REFGUID Name, UINT *pDataSize, void *pData)
{
	LogError("IDXGISwapChain::GetPrivateData is not supported by now.");
	return E_FAIL;
}

HRESULT WrappedD3D12DXGISwapChain::GetDevice(REFIID riid, void **ppDevice)
{
	*ppDevice = NULL;
	LogError("IDXGISwapChain::GetDevice is not supported by now.");
	return E_FAIL;
}

HRESULT WrappedD3D12DXGISwapChain::GetParent(REFIID riid, void **ppParent)
{
	// 		*ppParent = NULL;
	// 		LogError("IDXGISwapChain::GetParent is not supported by now.");
	// 		return E_FAIL;
	return m_pRealSwapChain->GetParent(riid, ppParent);
}

HRESULT WrappedD3D12DXGISwapChain::QueryInterface(
	REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	*ppvObject = NULL;
	LogError("IDXGISwapChain::QueryInterface is not supported by now.");
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12DXGISwapChain::Present(UINT SyncInterval, UINT Flags)
{
	//m_pWrappedDevice->OnFramePresent();//TODO: need update counter here?
	return m_pRealSwapChain->Present(SyncInterval, Flags);
}

HRESULT WrappedD3D12DXGISwapChain::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height,
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

ULONG WrappedD3D12DXGISwapChain::AddRef(void)
{
	return InterlockedIncrement(&m_Ref);
}

ULONG WrappedD3D12DXGISwapChain::Release(void)
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

RDCBOOST_NAMESPACE_END