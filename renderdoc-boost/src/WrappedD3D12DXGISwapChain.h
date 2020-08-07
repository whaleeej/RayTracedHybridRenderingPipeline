#pragma once 
#include "RdcBoostPCH.h"
#include "WrappedD3D12Object.h"

RDCBOOST_NAMESPACE_BEGIN
class WrappedD3D12Resource;
class WrappedD3D12CommandQueue;

class WrappedD3D12DXGISwapChain :public IDXGISwapChain4 { //support from swapchain1-4
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

public: //override swapchain
	virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void **ppSurface);

	virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen,
		IDXGIOutput *pTarget);

	virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL *pFullscreen,
		IDXGIOutput **ppTarget);

	virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput **ppOutput);

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

public: //override for swapchain2
	virtual HRESULT STDMETHODCALLTYPE SetSourceSize(
		UINT Width,
		UINT Height);

	virtual HRESULT STDMETHODCALLTYPE GetSourceSize(
		/* [annotation][out] */
		_Out_  UINT* pWidth,
		/* [annotation][out] */
		_Out_  UINT* pHeight);

	virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(
		UINT MaxLatency);

	virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(
		/* [annotation][out] */
		_Out_  UINT* pMaxLatency);

	virtual HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject(void) ;

	virtual HRESULT STDMETHODCALLTYPE SetMatrixTransform(
		const DXGI_MATRIX_3X2_F* pMatrix);

	virtual HRESULT STDMETHODCALLTYPE GetMatrixTransform(
		/* [annotation][out] */
		_Out_  DXGI_MATRIX_3X2_F* pMatrix);

public: //override swapchain3
	virtual UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex(void);

	virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(
		/* [annotation][in] */
		_In_  DXGI_COLOR_SPACE_TYPE ColorSpace,
		/* [annotation][out] */
		_Out_  UINT* pColorSpaceSupport) ;

	virtual HRESULT STDMETHODCALLTYPE SetColorSpace1(
		/* [annotation][in] */
		_In_  DXGI_COLOR_SPACE_TYPE ColorSpace) ;

	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1(
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
		_In_reads_(BufferCount)  IUnknown* const* ppPresentQueue);

public: //override swapchain4
	virtual HRESULT STDMETHODCALLTYPE SetHDRMetaData(
		/* [annotation][in] */
		_In_  DXGI_HDR_METADATA_TYPE Type,
		/* [annotation][in] */
		_In_  UINT Size,
		/* [annotation][size_is][in] */
		_In_reads_opt_(Size)  void* pMetaData);

public: //func
	bool checkVersionSupport(int _ver) { return _ver <= m_HighestVersion; };

	bool isResourceExist(WrappedD3D12Resource* pWrappedResource, ID3D12Resource* pRealResource);

	void cacheResourceReflectionToOldReal();

	void clearResourceReflectionToOldReal();

public: // framework
	void SwitchToCommandQueue(ID3D12CommandQueue* pRealCommandQueue);

	COMPtr<IDXGISwapChain1> CopyToCommandQueue(ID3D12CommandQueue* pRealCommandQueue);

	COMPtr<IDXGISwapChain> GetReal() { 
		COMPtr<IDXGISwapChain> _ret; _ret = static_cast<IDXGISwapChain*>(m_pRealSwapChain.Get()); 
	};

	COMPtr<IDXGISwapChain1> GetReal1() {
		return m_pRealSwapChain;
	};

	COMPtr<IDXGISwapChain2> GetReal2() {
		COMPtr<IDXGISwapChain2> _ret;
		m_pRealSwapChain.As(&_ret);
		return _ret;
	};

	COMPtr<IDXGISwapChain3> GetReal3() {
		COMPtr<IDXGISwapChain3> _ret;
		m_pRealSwapChain.As(&_ret);
		return _ret;
	};

	COMPtr<IDXGISwapChain4> GetReal4() {
		COMPtr<IDXGISwapChain4> _ret;
		m_pRealSwapChain.As(&_ret);
		return _ret;
	};

protected:
	COMPtr<IDXGISwapChain1> m_pRealSwapChain;// the wrapped real swapchain

	COMPtr<WrappedD3D12CommandQueue> const m_pWrappedCommandQueue;
	std::vector< COMPtr<WrappedD3D12Resource>> m_SwapChainBuffers;
	std::map<WrappedD3D12ObjectBase*, ID3D12Resource*>		m_BackRefs_Resource_Reflection; //cache old real device when switching

	SResizeBufferParameter m_ResizeParam;
	unsigned int m_Ref;
	int m_HighestVersion;
};

RDCBOOST_NAMESPACE_END

