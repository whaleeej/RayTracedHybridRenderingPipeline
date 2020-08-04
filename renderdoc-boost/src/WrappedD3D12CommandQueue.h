#pragma once
#include <dxgi1_6.h>
#include "WrappedD3D12DeviceChild.h"

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12DXGISwapChain;
class WrappedD3D12Resource;

class WrappedD3D12CommandQueue : public WrappedD3D12DeviceChild<ID3D12CommandQueue>
{
public:
	WrappedD3D12CommandQueue(ID3D12CommandQueue* pRealD3D12CommandQueue, WrappedD3D12Device* pDevice);
	~WrappedD3D12CommandQueue();

public://override
	virtual void STDMETHODCALLTYPE UpdateTileMappings(
		_In_  ID3D12Resource *pResource,
		UINT NumResourceRegions,
		_In_reads_opt_(NumResourceRegions)  const D3D12_TILED_RESOURCE_COORDINATE *pResourceRegionStartCoordinates,
		_In_reads_opt_(NumResourceRegions)  const D3D12_TILE_REGION_SIZE *pResourceRegionSizes,
		_In_opt_  ID3D12Heap *pHeap,
		UINT NumRanges,
		_In_reads_opt_(NumRanges)  const D3D12_TILE_RANGE_FLAGS *pRangeFlags,
		_In_reads_opt_(NumRanges)  const UINT *pHeapRangeStartOffsets,
		_In_reads_opt_(NumRanges)  const UINT *pRangeTileCounts,
		D3D12_TILE_MAPPING_FLAGS Flags) ;

	virtual void STDMETHODCALLTYPE CopyTileMappings(
		_In_  ID3D12Resource *pDstResource,
		_In_  const D3D12_TILED_RESOURCE_COORDINATE *pDstRegionStartCoordinate,
		_In_  ID3D12Resource *pSrcResource,
		_In_  const D3D12_TILED_RESOURCE_COORDINATE *pSrcRegionStartCoordinate,
		_In_  const D3D12_TILE_REGION_SIZE *pRegionSize,
		D3D12_TILE_MAPPING_FLAGS Flags) ;

	virtual void STDMETHODCALLTYPE ExecuteCommandLists(
		_In_  UINT NumCommandLists,
		_In_reads_(NumCommandLists)  ID3D12CommandList *const *ppCommandLists) ;

	virtual void STDMETHODCALLTYPE SetMarker(
		UINT Metadata,
		_In_reads_bytes_opt_(Size)  const void *pData,
		UINT Size) ;

	virtual void STDMETHODCALLTYPE BeginEvent(
		UINT Metadata,
		_In_reads_bytes_opt_(Size)  const void *pData,
		UINT Size) ;

	virtual void STDMETHODCALLTYPE EndEvent(void) ;

	virtual HRESULT STDMETHODCALLTYPE Signal(
		ID3D12Fence *pFence,
		UINT64 Value) ;

	virtual HRESULT STDMETHODCALLTYPE Wait(
		ID3D12Fence *pFence,
		UINT64 Value) ;

	virtual HRESULT STDMETHODCALLTYPE GetTimestampFrequency(
		_Out_  UINT64 *pFrequency) ;

	virtual HRESULT STDMETHODCALLTYPE GetClockCalibration(
		_Out_  UINT64 *pGpuTimestamp,
		_Out_  UINT64 *pCpuTimestamp) ;

	virtual D3D12_COMMAND_QUEUE_DESC STDMETHODCALLTYPE GetDesc(void) ;

public: //function
	bool isResourceExist(WrappedD3D12Resource* pWrappedResource);

public: //framework
	HRESULT createSwapChain(
		IDXGIFactory2* pDXGIFactory, HWND hWnd, 
		const DXGI_SWAP_CHAIN_DESC1 *pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc, 
		IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain);
	virtual COMPtr<ID3D12DeviceChild> CopyToDevice(ID3D12Device* pNewDevice);

protected:
	COMPtr<WrappedD3D12DXGISwapChain> m_pWrappedSwapChain;
};

RDCBOOST_NAMESPACE_END