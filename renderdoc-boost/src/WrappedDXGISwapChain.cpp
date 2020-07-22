#include "WrappedDXGISwapChain.h"
#include "Log.h"
#include <d3d11.h>
#include "WrappedD3D11Device.h"
#include "WrappedD3D11Resource.h"

namespace rdcboost
{
	WrappedDXGISwapChain::WrappedDXGISwapChain(
		IDXGISwapChain* pReal, WrappedD3D11Device* pWrappedDevice) :
		m_pWrappedDevice(pWrappedDevice), m_pReal(pReal), m_Ref(1)
	{
		m_pReal->AddRef();
		m_pWrappedDevice->AddRef();
		m_ResizeParam.Valid = false;
	}

	WrappedDXGISwapChain::~WrappedDXGISwapChain()
	{
		m_pReal->Release();
		m_pWrappedDevice->Release();
	}

	HRESULT WrappedDXGISwapChain::GetBuffer(UINT Buffer, REFIID riid, void **ppSurface)
	{
		if (ppSurface == NULL) return E_INVALIDARG;

		// ID3D10Texture2D UUID {9B7E4C04-342C-4106-A19F-4F2704F689F0}
		static const GUID ID3D10Texture2D_uuid = { 0x9b7e4c04, 0x342c, 0x4106, { 0xa1, 0x9f, 0x4f, 0x27, 0x04, 0xf6, 0x89, 0xf0 } };

		// ID3D10Resource  UUID {9B7E4C01-342C-4106-A19F-4F2704F689F0}
		static const GUID ID3D10Resource_uuid = { 0x9b7e4c01, 0x342c, 0x4106, { 0xa1, 0x9f, 0x4f, 0x27, 0x04, 0xf6, 0x89, 0xf0 } };

		if (riid == ID3D10Texture2D_uuid || riid == ID3D10Resource_uuid)
		{
			LogError("Querying swapchain buffers via D3D10 interface UUIDs is not supported");
			return E_NOINTERFACE;
		}
		else if (riid != __uuidof(ID3D11Texture2D) && riid != __uuidof(ID3D11Resource))
		{
			LogError("Unsupported or unrecognised UUID passed to IDXGISwapChain::GetBuffer");
			return E_NOINTERFACE;
		}

		Assert(riid == __uuidof(ID3D11Texture2D) || riid == __uuidof(ID3D11Resource));

		ID3D11Texture2D *realSurface = NULL;
		HRESULT ret = m_pReal->GetBuffer(Buffer, riid, (void**) &realSurface);

		ID3D11Texture2D *wrappedTex = NULL;
		if (FAILED(ret))
		{
			LogError("Failed to get swapchain backbuffer %d: %08x", Buffer, ret);
			*ppSurface = NULL;
		}
		else 
		{
			wrappedTex = m_pWrappedDevice->GetWrappedSwapChainBuffer(Buffer, realSurface);

			if (riid == __uuidof(ID3D11Texture2D))
				*ppSurface = static_cast<ID3D11Texture2D*>(wrappedTex);
			else if (riid == __uuidof(ID3D11Resource))
				*ppSurface = static_cast<ID3D11Resource*>(wrappedTex);
		}

		if (realSurface != NULL)
		{
			realSurface->Release();
			realSurface = NULL;
		}

		return ret;
	}

	HRESULT WrappedDXGISwapChain::SetFullscreenState(BOOL Fullscreen,
																	   IDXGIOutput *pTarget)
	{
		return m_pReal->SetFullscreenState(Fullscreen, pTarget);
	}

	HRESULT WrappedDXGISwapChain::GetFullscreenState(BOOL *pFullscreen, 
																	   IDXGIOutput **ppTarget)
	{
		return m_pReal->GetFullscreenState(pFullscreen, ppTarget);
	}

	HRESULT WrappedDXGISwapChain::GetContainingOutput(IDXGIOutput **ppOutput)
	{
		return m_pReal->GetContainingOutput(ppOutput);
	}

	HRESULT WrappedDXGISwapChain::SetPrivateData(
		REFGUID Name, UINT DataSize, const void *pData)
	{
		LogError("IDXGISwapChain::SetPrivateData is not supported by now.");
		return E_FAIL;
	}

	HRESULT WrappedDXGISwapChain::SetPrivateDataInterface(
		REFGUID Name, const IUnknown *pUnknown)
	{
		LogError("IDXGISwapChain::SetPrivateDataInterface is not supported by now.");
		return E_FAIL;
	}

	HRESULT WrappedDXGISwapChain::GetPrivateData(
		REFGUID Name, UINT *pDataSize, void *pData)
	{
		LogError("IDXGISwapChain::GetPrivateData is not supported by now.");
		return E_FAIL;
	}

	HRESULT WrappedDXGISwapChain::GetDevice(REFIID riid, void **ppDevice)
	{
		*ppDevice = NULL;
		LogError("IDXGISwapChain::GetDevice is not supported by now.");
		return E_FAIL;
	}

	HRESULT WrappedDXGISwapChain::GetParent(REFIID riid, void **ppParent)
	{
// 		*ppParent = NULL;
// 		LogError("IDXGISwapChain::GetParent is not supported by now.");
// 		return E_FAIL;
		return m_pReal->GetParent(riid, ppParent);
	}

	HRESULT WrappedDXGISwapChain::QueryInterface(
		REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		*ppvObject = NULL;
		LogError("IDXGISwapChain::QueryInterface is not supported by now.");
		return E_FAIL;
	}

	HRESULT STDMETHODCALLTYPE WrappedDXGISwapChain::Present(UINT SyncInterval, UINT Flags)
	{
		m_pWrappedDevice->OnFramePresent();
		return m_pReal->Present(SyncInterval, Flags);
	}

	HRESULT WrappedDXGISwapChain::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height,
												DXGI_FORMAT NewFormat, UINT SwapChainFlags)
	{
		HRESULT res = m_pReal->ResizeBuffers(BufferCount, Width, Height,
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

	ULONG WrappedDXGISwapChain::AddRef(void)
	{
		return InterlockedIncrement(&m_Ref);
	}

	ULONG WrappedDXGISwapChain::Release(void)
	{
		unsigned int ret = InterlockedDecrement(&m_Ref);
		if (ret == 1)
		{
			m_pWrappedDevice->TryToRelease();
		}
		else if (ret == 0)
		{
			delete this;
		}

		return ret;
	}

	void WrappedDXGISwapChain::SwitchToDevice(IDXGISwapChain* pNewSwapChain)
	{
		if (m_pReal != pNewSwapChain)
		{ // TODO_wzq handle resize/fullscreen
			m_pReal->Release();
			m_pReal = pNewSwapChain;
			pNewSwapChain->AddRef();

			if (m_ResizeParam.Valid)
			{
				HRESULT res = pNewSwapChain->ResizeBuffers(
					m_ResizeParam.BufferCount, m_ResizeParam.Width, m_ResizeParam.Height,
					m_ResizeParam.NewFormat, m_ResizeParam.SwapChainFlags);

				if (FAILED(res))
					LogError("Resize on new swap chain failed.");
			}
		}
	}

}

