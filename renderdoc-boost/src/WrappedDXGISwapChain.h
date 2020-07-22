#pragma once
#include <dxgi.h>

namespace rdcboost
{
	class WrappedD3D11Device;
	class WrappedDXGISwapChain : public IDXGISwapChain
	{
		struct SResizeBufferParameter
		{
			bool Valid;
			UINT BufferCount;
			UINT Width;
			UINT Height;
			DXGI_FORMAT NewFormat;
			UINT SwapChainFlags;
		};

	public:
		WrappedDXGISwapChain(IDXGISwapChain* pReal, WrappedD3D11Device* pWrappedDevice);

		virtual ~WrappedDXGISwapChain();

		virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void **ppSurface);

		// TODO_wzq wrap IDXGIOutput
		virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen,
															 IDXGIOutput *pTarget);

		virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL *pFullscreen,
															 IDXGIOutput **ppTarget);
		
		virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput **ppOutput);


		// TODO_wzq wrap following methods.
		virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize,
														 const void *pData);

		virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name,
																  const IUnknown *pUnknown);

		virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT *pDataSize,
														 void *pData);

		virtual HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void **ppDevice);

		virtual HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void **ppParent);

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(
			REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);


	public:
		virtual HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags);

		virtual HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC *pDesc)
		{
			return m_pReal->GetDesc(pDesc);
		}

		virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(
			UINT BufferCount, UINT Width, UINT Height,
			DXGI_FORMAT NewFormat, UINT SwapChainFlags);

		virtual HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC *pNewTargetParameters)
		{
			return m_pReal->ResizeTarget(pNewTargetParameters);
		}

		virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS *pStats)
		{
			return m_pReal->GetFrameStatistics(pStats);
		}

		virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT *pLastPresentCount)
		{
			return m_pReal->GetLastPresentCount(pLastPresentCount);
		}

		virtual ULONG STDMETHODCALLTYPE AddRef(void);

		virtual ULONG STDMETHODCALLTYPE Release(void);

		ULONG GetRef() { return m_Ref; }

	public:
		void SwitchToDevice(IDXGISwapChain* pNewSwapChain);

	private:
		WrappedD3D11Device* m_pWrappedDevice;
		IDXGISwapChain* m_pReal;
		unsigned int m_Ref;

		SResizeBufferParameter m_ResizeParam;
	};
}

