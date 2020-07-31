#pragma once 
#include <vector>
#include <dxgi1_6.h>
#include "RdcBoostPCH.h"

RDCBOOST_NAMESPACE_BEGIN
class WrappedD3D12Resource;
class WrappedD3D12CommandQueue;

class WrappedD3D12DXGISwapChain :public IDXGISwapChain1 {
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
	WrappedD3D12DXGISwapChain(IDXGISwapChain1* pReal, WrappedD3D12CommandQueue* pCommandQueue);
	virtual ~WrappedD3D12DXGISwapChain();

public: //override
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

	virtual HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags);

	virtual HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC *pDesc)
	{
		return m_pRealSwapChain->GetDesc(pDesc);
	}

	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(
		UINT BufferCount, UINT Width, UINT Height,
		DXGI_FORMAT NewFormat, UINT SwapChainFlags);

	virtual HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC *pNewTargetParameters)
	{
		return m_pRealSwapChain->ResizeTarget(pNewTargetParameters);
	}

	virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS *pStats)
	{
		return m_pRealSwapChain->GetFrameStatistics(pStats);
	}

	virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT *pLastPresentCount)
	{
		return m_pRealSwapChain->GetLastPresentCount(pLastPresentCount);
	}

	virtual ULONG STDMETHODCALLTYPE AddRef(void);

	virtual ULONG STDMETHODCALLTYPE Release(void);

	ULONG GetRef() { return m_Ref; }

public: //override for swapchain1
	virtual HRESULT STDMETHODCALLTYPE GetDesc1(
		/* [annotation][out] */
		_Out_  DXGI_SWAP_CHAIN_DESC1 *pDesc) ;

	virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc(
		/* [annotation][out] */
		_Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc) ;

	virtual HRESULT STDMETHODCALLTYPE GetHwnd(
		/* [annotation][out] */
		_Out_  HWND *pHwnd) ;

	virtual HRESULT STDMETHODCALLTYPE GetCoreWindow(
		/* [annotation][in] */
		_In_  REFIID refiid,
		/* [annotation][out] */
		_COM_Outptr_  void **ppUnk) ;

	virtual HRESULT STDMETHODCALLTYPE Present1(
		/* [in] */ UINT SyncInterval,
		/* [in] */ UINT PresentFlags,
		/* [annotation][in] */
		_In_  const DXGI_PRESENT_PARAMETERS *pPresentParameters) ;

	virtual BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported(void) ;

	virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput(
		/* [annotation][out] */
		_Out_  IDXGIOutput **ppRestrictToOutput) ;

	virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor(
		/* [annotation][in] */
		_In_  const DXGI_RGBA *pColor) ;

	virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor(
		/* [annotation][out] */
		_Out_  DXGI_RGBA *pColor) ;

	virtual HRESULT STDMETHODCALLTYPE SetRotation(
		/* [annotation][in] */
		_In_  DXGI_MODE_ROTATION Rotation) ;

	virtual HRESULT STDMETHODCALLTYPE GetRotation(
		/* [annotation][out] */
		_Out_  DXGI_MODE_ROTATION *pRotation) ;

public: // framework
	void SwitchToCommandQueue(ID3D12CommandQueue* pRealCommandQueue);
	COMPtr<IDXGISwapChain1> CopyToCommandQueue(ID3D12CommandQueue* pRealCommandQueue);

protected:
	COMPtr<IDXGISwapChain1> m_pRealSwapChain;// the wrapped real swapchain

	COMPtr<WrappedD3D12CommandQueue> const m_pWrappedCommandQueue;
	std::vector< COMPtr<WrappedD3D12Resource>> m_SwapChainBuffers;


	SResizeBufferParameter m_ResizeParam;
	unsigned int m_Ref;
};

RDCBOOST_NAMESPACE_END

