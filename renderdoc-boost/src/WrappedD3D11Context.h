#pragma once
#include <d3d11_1.h>
#include "WrappedD3D11DeviceChild.h"

namespace rdcboost
{
	class WrappedD3D11Context;
	class WrappedD3DUserAnno : public ID3DUserDefinedAnnotation
	{
	public:
		WrappedD3DUserAnno(WrappedD3D11Context* pImpl) : m_pImpl(pImpl) {}

		virtual INT STDMETHODCALLTYPE BeginEvent(
			/* [annotation] */
			_In_  LPCWSTR Name);

		virtual INT STDMETHODCALLTYPE EndEvent(void);

		virtual void STDMETHODCALLTYPE SetMarker(
			/* [annotation] */
			_In_  LPCWSTR Name);

		virtual BOOL STDMETHODCALLTYPE GetStatus(void);

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);

		virtual ULONG STDMETHODCALLTYPE AddRef(void);

		virtual ULONG STDMETHODCALLTYPE Release(void);

	private:
		WrappedD3D11Context* m_pImpl;
	};

	struct SDeviceContextState;
	class WrappedD3D11Device;
	class WrappedD3D11Context : public WrappedD3D11DeviceChild<ID3D11DeviceContext>
	{
	public:
		WrappedD3D11Context(ID3D11DeviceContext* pRealContext, WrappedD3D11Device* pWrappedDevice);
		virtual ~WrappedD3D11Context();

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);

		virtual ULONG STDMETHODCALLTYPE Release(void);

		ULONG GetRef() { return m_Ref; }

		virtual void STDMETHODCALLTYPE VSSetConstantBuffers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
			/* [annotation] */
			_In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

		virtual void STDMETHODCALLTYPE PSSetShaderResources(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
			/* [annotation] */
			_In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

		virtual void STDMETHODCALLTYPE PSSetShader(
			/* [annotation] */
			_In_opt_  ID3D11PixelShader *pPixelShader,
			/* [annotation] */
			_In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
			UINT NumClassInstances);

		virtual void STDMETHODCALLTYPE PSSetSamplers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
			/* [annotation] */
			_In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

		virtual void STDMETHODCALLTYPE VSSetShader(
			/* [annotation] */
			_In_opt_  ID3D11VertexShader *pVertexShader,
			/* [annotation] */
			_In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
			UINT NumClassInstances);

		virtual void STDMETHODCALLTYPE DrawIndexed(
			/* [annotation] */
			_In_  UINT IndexCount,
			/* [annotation] */
			_In_  UINT StartIndexLocation,
			/* [annotation] */
			_In_  INT BaseVertexLocation);

		virtual void STDMETHODCALLTYPE Draw(
			/* [annotation] */
			_In_  UINT VertexCount,
			/* [annotation] */
			_In_  UINT StartVertexLocation);

		virtual HRESULT STDMETHODCALLTYPE Map(
			/* [annotation] */
			_In_  ID3D11Resource *pResource,
			/* [annotation] */
			_In_  UINT Subresource,
			/* [annotation] */
			_In_  D3D11_MAP MapType,
			/* [annotation] */
			_In_  UINT MapFlags,
			/* [annotation] */
			_Out_  D3D11_MAPPED_SUBRESOURCE *pMappedResource);

		virtual void STDMETHODCALLTYPE Unmap(
			/* [annotation] */
			_In_  ID3D11Resource *pResource,
			/* [annotation] */
			_In_  UINT Subresource);

		virtual void STDMETHODCALLTYPE PSSetConstantBuffers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
			/* [annotation] */
			_In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

		virtual void STDMETHODCALLTYPE IASetInputLayout(
			/* [annotation] */
			_In_opt_  ID3D11InputLayout *pInputLayout);

		virtual void STDMETHODCALLTYPE IASetVertexBuffers(
			/* [annotation] */
			_In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumBuffers,
			/* [annotation] */
			_In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppVertexBuffers,
			/* [annotation] */
			_In_reads_opt_(NumBuffers)  const UINT *pStrides,
			/* [annotation] */
			_In_reads_opt_(NumBuffers)  const UINT *pOffsets);

		virtual void STDMETHODCALLTYPE IASetIndexBuffer(
			/* [annotation] */
			_In_opt_  ID3D11Buffer *pIndexBuffer,
			/* [annotation] */
			_In_  DXGI_FORMAT Format,
			/* [annotation] */
			_In_  UINT Offset);

		virtual void STDMETHODCALLTYPE DrawIndexedInstanced(
			/* [annotation] */
			_In_  UINT IndexCountPerInstance,
			/* [annotation] */
			_In_  UINT InstanceCount,
			/* [annotation] */
			_In_  UINT StartIndexLocation,
			/* [annotation] */
			_In_  INT BaseVertexLocation,
			/* [annotation] */
			_In_  UINT StartInstanceLocation);

		virtual void STDMETHODCALLTYPE DrawInstanced(
			/* [annotation] */
			_In_  UINT VertexCountPerInstance,
			/* [annotation] */
			_In_  UINT InstanceCount,
			/* [annotation] */
			_In_  UINT StartVertexLocation,
			/* [annotation] */
			_In_  UINT StartInstanceLocation);

		virtual void STDMETHODCALLTYPE GSSetConstantBuffers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
			/* [annotation] */
			_In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

		virtual void STDMETHODCALLTYPE GSSetShader(
			/* [annotation] */
			_In_opt_  ID3D11GeometryShader *pShader,
			/* [annotation] */
			_In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
			UINT NumClassInstances);

		virtual void STDMETHODCALLTYPE IASetPrimitiveTopology(
			/* [annotation] */
			_In_  D3D11_PRIMITIVE_TOPOLOGY Topology);

		virtual void STDMETHODCALLTYPE VSSetShaderResources(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
			/* [annotation] */
			_In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

		virtual void STDMETHODCALLTYPE VSSetSamplers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
			/* [annotation] */
			_In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

		virtual void STDMETHODCALLTYPE Begin(
			/* [annotation] */
			_In_  ID3D11Asynchronous *pAsync);

		virtual void STDMETHODCALLTYPE End(
			/* [annotation] */
			_In_  ID3D11Asynchronous *pAsync);

		virtual HRESULT STDMETHODCALLTYPE GetData(
			/* [annotation] */
			_In_  ID3D11Asynchronous *pAsync,
			/* [annotation] */
			_Out_writes_bytes_opt_(DataSize)  void *pData,
			/* [annotation] */
			_In_  UINT DataSize,
			/* [annotation] */
			_In_  UINT GetDataFlags);

		virtual void STDMETHODCALLTYPE SetPredication(
			/* [annotation] */
			_In_opt_  ID3D11Predicate *pPredicate,
			/* [annotation] */
			_In_  BOOL PredicateValue);

		virtual void STDMETHODCALLTYPE GSSetShaderResources(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
			/* [annotation] */
			_In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

		virtual void STDMETHODCALLTYPE GSSetSamplers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
			/* [annotation] */
			_In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

		virtual void STDMETHODCALLTYPE OMSetRenderTargets(
			/* [annotation] */
			_In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumViews,
			/* [annotation] */
			_In_reads_opt_(NumViews)  ID3D11RenderTargetView *const *ppRenderTargetViews,
			/* [annotation] */
			_In_opt_  ID3D11DepthStencilView *pDepthStencilView);

		virtual void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews(
			/* [annotation] */
			_In_  UINT NumRTVs,
			/* [annotation] */
			_In_reads_opt_(NumRTVs)  ID3D11RenderTargetView *const *ppRenderTargetViews,
			/* [annotation] */
			_In_opt_  ID3D11DepthStencilView *pDepthStencilView,
			/* [annotation] */
			_In_range_(0, D3D11_1_UAV_SLOT_COUNT - 1)  UINT UAVStartSlot,
			/* [annotation] */
			_In_  UINT NumUAVs,
			/* [annotation] */
			_In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
			/* [annotation] */
			_In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts);

		virtual void STDMETHODCALLTYPE OMSetBlendState(
			/* [annotation] */
			_In_opt_  ID3D11BlendState *pBlendState,
			/* [annotation] */
			_In_opt_  const FLOAT BlendFactor[4],
			/* [annotation] */
			_In_  UINT SampleMask);

		virtual void STDMETHODCALLTYPE OMSetDepthStencilState(
			/* [annotation] */
			_In_opt_  ID3D11DepthStencilState *pDepthStencilState,
			/* [annotation] */
			_In_  UINT StencilRef);

		virtual void STDMETHODCALLTYPE SOSetTargets(
			/* [annotation] */
			_In_range_(0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,
			/* [annotation] */
			_In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppSOTargets,
			/* [annotation] */
			_In_reads_opt_(NumBuffers)  const UINT *pOffsets);

		virtual void STDMETHODCALLTYPE DrawAuto(void);

		virtual void STDMETHODCALLTYPE DrawIndexedInstancedIndirect(
			/* [annotation] */
			_In_  ID3D11Buffer *pBufferForArgs,
			/* [annotation] */
			_In_  UINT AlignedByteOffsetForArgs);

		virtual void STDMETHODCALLTYPE DrawInstancedIndirect(
			/* [annotation] */
			_In_  ID3D11Buffer *pBufferForArgs,
			/* [annotation] */
			_In_  UINT AlignedByteOffsetForArgs);

		virtual void STDMETHODCALLTYPE Dispatch(
			/* [annotation] */
			_In_  UINT ThreadGroupCountX,
			/* [annotation] */
			_In_  UINT ThreadGroupCountY,
			/* [annotation] */
			_In_  UINT ThreadGroupCountZ);

		virtual void STDMETHODCALLTYPE DispatchIndirect(
			/* [annotation] */
			_In_  ID3D11Buffer *pBufferForArgs,
			/* [annotation] */
			_In_  UINT AlignedByteOffsetForArgs);

		virtual void STDMETHODCALLTYPE RSSetState(
			/* [annotation] */
			_In_opt_  ID3D11RasterizerState *pRasterizerState);

		virtual void STDMETHODCALLTYPE RSSetViewports(
			/* [annotation] */
			_In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
			/* [annotation] */
			_In_reads_opt_(NumViewports)  const D3D11_VIEWPORT *pViewports);

		virtual void STDMETHODCALLTYPE RSSetScissorRects(
			/* [annotation] */
			_In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
			/* [annotation] */
			_In_reads_opt_(NumRects)  const D3D11_RECT *pRects);

		virtual void STDMETHODCALLTYPE CopySubresourceRegion(
			/* [annotation] */
			_In_  ID3D11Resource *pDstResource,
			/* [annotation] */
			_In_  UINT DstSubresource,
			/* [annotation] */
			_In_  UINT DstX,
			/* [annotation] */
			_In_  UINT DstY,
			/* [annotation] */
			_In_  UINT DstZ,
			/* [annotation] */
			_In_  ID3D11Resource *pSrcResource,
			/* [annotation] */
			_In_  UINT SrcSubresource,
			/* [annotation] */
			_In_opt_  const D3D11_BOX *pSrcBox);

		virtual void STDMETHODCALLTYPE CopyResource(
			/* [annotation] */
			_In_  ID3D11Resource *pDstResource,
			/* [annotation] */
			_In_  ID3D11Resource *pSrcResource);

		virtual void STDMETHODCALLTYPE UpdateSubresource(
			/* [annotation] */
			_In_  ID3D11Resource *pDstResource,
			/* [annotation] */
			_In_  UINT DstSubresource,
			/* [annotation] */
			_In_opt_  const D3D11_BOX *pDstBox,
			/* [annotation] */
			_In_  const void *pSrcData,
			/* [annotation] */
			_In_  UINT SrcRowPitch,
			/* [annotation] */
			_In_  UINT SrcDepthPitch);

		virtual void STDMETHODCALLTYPE CopyStructureCount(
			/* [annotation] */
			_In_  ID3D11Buffer *pDstBuffer,
			/* [annotation] */
			_In_  UINT DstAlignedByteOffset,
			/* [annotation] */
			_In_  ID3D11UnorderedAccessView *pSrcView);

		virtual void STDMETHODCALLTYPE ClearRenderTargetView(
			/* [annotation] */
			_In_  ID3D11RenderTargetView *pRenderTargetView,
			/* [annotation] */
			_In_  const FLOAT ColorRGBA[4]);

		virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint(
			/* [annotation] */
			_In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
			/* [annotation] */
			_In_  const UINT Values[4]);

		virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat(
			/* [annotation] */
			_In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
			/* [annotation] */
			_In_  const FLOAT Values[4]);

		virtual void STDMETHODCALLTYPE ClearDepthStencilView(
			/* [annotation] */
			_In_  ID3D11DepthStencilView *pDepthStencilView,
			/* [annotation] */
			_In_  UINT ClearFlags,
			/* [annotation] */
			_In_  FLOAT Depth,
			/* [annotation] */
			_In_  UINT8 Stencil);

		virtual void STDMETHODCALLTYPE GenerateMips(
			/* [annotation] */
			_In_  ID3D11ShaderResourceView *pShaderResourceView);

		virtual void STDMETHODCALLTYPE SetResourceMinLOD(
			/* [annotation] */
			_In_  ID3D11Resource *pResource,
			FLOAT MinLOD);

		virtual FLOAT STDMETHODCALLTYPE GetResourceMinLOD(
			/* [annotation] */
			_In_  ID3D11Resource *pResource);

		virtual void STDMETHODCALLTYPE ResolveSubresource(
			/* [annotation] */
			_In_  ID3D11Resource *pDstResource,
			/* [annotation] */
			_In_  UINT DstSubresource,
			/* [annotation] */
			_In_  ID3D11Resource *pSrcResource,
			/* [annotation] */
			_In_  UINT SrcSubresource,
			/* [annotation] */
			_In_  DXGI_FORMAT Format);

		virtual void STDMETHODCALLTYPE ExecuteCommandList(
			/* [annotation] */
			_In_  ID3D11CommandList *pCommandList,
			BOOL RestoreContextState);

		virtual void STDMETHODCALLTYPE HSSetShaderResources(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
			/* [annotation] */
			_In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

		virtual void STDMETHODCALLTYPE HSSetShader(
			/* [annotation] */
			_In_opt_  ID3D11HullShader *pHullShader,
			/* [annotation] */
			_In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
			UINT NumClassInstances);

		virtual void STDMETHODCALLTYPE HSSetSamplers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
			/* [annotation] */
			_In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

		virtual void STDMETHODCALLTYPE HSSetConstantBuffers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
			/* [annotation] */
			_In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

		virtual void STDMETHODCALLTYPE DSSetShaderResources(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
			/* [annotation] */
			_In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

		virtual void STDMETHODCALLTYPE DSSetShader(
			/* [annotation] */
			_In_opt_  ID3D11DomainShader *pDomainShader,
			/* [annotation] */
			_In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
			UINT NumClassInstances);

		virtual void STDMETHODCALLTYPE DSSetSamplers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
			/* [annotation] */
			_In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

		virtual void STDMETHODCALLTYPE DSSetConstantBuffers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
			/* [annotation] */
			_In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

		virtual void STDMETHODCALLTYPE CSSetShaderResources(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
			/* [annotation] */
			_In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews);

		virtual void STDMETHODCALLTYPE CSSetUnorderedAccessViews(
			/* [annotation] */
			_In_range_(0, D3D11_1_UAV_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_1_UAV_SLOT_COUNT - StartSlot)  UINT NumUAVs,
			/* [annotation] */
			_In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
			/* [annotation] */
			_In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts);

		virtual void STDMETHODCALLTYPE CSSetShader(
			/* [annotation] */
			_In_opt_  ID3D11ComputeShader *pComputeShader,
			/* [annotation] */
			_In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
			UINT NumClassInstances);

		virtual void STDMETHODCALLTYPE CSSetSamplers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
			/* [annotation] */
			_In_reads_opt_(NumSamplers)  ID3D11SamplerState *const *ppSamplers);

		virtual void STDMETHODCALLTYPE CSSetConstantBuffers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
			/* [annotation] */
			_In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers);

		virtual void STDMETHODCALLTYPE VSGetConstantBuffers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
			/* [annotation] */
			_Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

		virtual void STDMETHODCALLTYPE PSGetShaderResources(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
			/* [annotation] */
			_Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

		virtual void STDMETHODCALLTYPE PSGetShader(
			/* [annotation] */
			_Out_  ID3D11PixelShader **ppPixelShader,
			/* [annotation] */
			_Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
			/* [annotation] */
			_Inout_opt_  UINT *pNumClassInstances);

		virtual void STDMETHODCALLTYPE PSGetSamplers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
			/* [annotation] */
			_Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

		virtual void STDMETHODCALLTYPE VSGetShader(
			/* [annotation] */
			_Out_  ID3D11VertexShader **ppVertexShader,
			/* [annotation] */
			_Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
			/* [annotation] */
			_Inout_opt_  UINT *pNumClassInstances);

		virtual void STDMETHODCALLTYPE PSGetConstantBuffers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
			/* [annotation] */
			_Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

		virtual void STDMETHODCALLTYPE IAGetInputLayout(
			/* [annotation] */
			_Out_  ID3D11InputLayout **ppInputLayout);

		virtual void STDMETHODCALLTYPE IAGetVertexBuffers(
			/* [annotation] */
			_In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumBuffers,
			/* [annotation] */
			_Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppVertexBuffers,
			/* [annotation] */
			_Out_writes_opt_(NumBuffers)  UINT *pStrides,
			/* [annotation] */
			_Out_writes_opt_(NumBuffers)  UINT *pOffsets);

		virtual void STDMETHODCALLTYPE IAGetIndexBuffer(
			/* [annotation] */
			_Out_opt_  ID3D11Buffer **pIndexBuffer,
			/* [annotation] */
			_Out_opt_  DXGI_FORMAT *Format,
			/* [annotation] */
			_Out_opt_  UINT *Offset);

		virtual void STDMETHODCALLTYPE GSGetConstantBuffers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
			/* [annotation] */
			_Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

		virtual void STDMETHODCALLTYPE GSGetShader(
			/* [annotation] */
			_Out_  ID3D11GeometryShader **ppGeometryShader,
			/* [annotation] */
			_Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
			/* [annotation] */
			_Inout_opt_  UINT *pNumClassInstances);

		virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology(
			/* [annotation] */
			_Out_  D3D11_PRIMITIVE_TOPOLOGY *pTopology);

		virtual void STDMETHODCALLTYPE VSGetShaderResources(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
			/* [annotation] */
			_Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

		virtual void STDMETHODCALLTYPE VSGetSamplers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
			/* [annotation] */
			_Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

		virtual void STDMETHODCALLTYPE GetPredication(
			/* [annotation] */
			_Out_opt_  ID3D11Predicate **ppPredicate,
			/* [annotation] */
			_Out_opt_  BOOL *pPredicateValue);

		virtual void STDMETHODCALLTYPE GSGetShaderResources(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
			/* [annotation] */
			_Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

		virtual void STDMETHODCALLTYPE GSGetSamplers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
			/* [annotation] */
			_Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

		virtual void STDMETHODCALLTYPE OMGetRenderTargets(
			/* [annotation] */
			_In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumViews,
			/* [annotation] */
			_Out_writes_opt_(NumViews)  ID3D11RenderTargetView **ppRenderTargetViews,
			/* [annotation] */
			_Out_opt_  ID3D11DepthStencilView **ppDepthStencilView);

		virtual void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews(
			/* [annotation] */
			_In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumRTVs,
			/* [annotation] */
			_Out_writes_opt_(NumRTVs)  ID3D11RenderTargetView **ppRenderTargetViews,
			/* [annotation] */
			_Out_opt_  ID3D11DepthStencilView **ppDepthStencilView,
			/* [annotation] */
			_In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)  UINT UAVStartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - UAVStartSlot)  UINT NumUAVs,
			/* [annotation] */
			_Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews);

		virtual void STDMETHODCALLTYPE OMGetBlendState(
			/* [annotation] */
			_Out_opt_  ID3D11BlendState **ppBlendState,
			/* [annotation] */
			_Out_opt_  FLOAT BlendFactor[4],
			/* [annotation] */
			_Out_opt_  UINT *pSampleMask);

		virtual void STDMETHODCALLTYPE OMGetDepthStencilState(
			/* [annotation] */
			_Out_opt_  ID3D11DepthStencilState **ppDepthStencilState,
			/* [annotation] */
			_Out_opt_  UINT *pStencilRef);

		virtual void STDMETHODCALLTYPE SOGetTargets(
			/* [annotation] */
			_In_range_(0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,
			/* [annotation] */
			_Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppSOTargets);

		virtual void STDMETHODCALLTYPE RSGetState(
			/* [annotation] */
			_Out_  ID3D11RasterizerState **ppRasterizerState);

		virtual void STDMETHODCALLTYPE RSGetViewports(
			/* [annotation] */
			_Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumViewports,
			/* [annotation] */
			_Out_writes_opt_(*pNumViewports)  D3D11_VIEWPORT *pViewports);

		virtual void STDMETHODCALLTYPE RSGetScissorRects(
			/* [annotation] */
			_Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumRects,
			/* [annotation] */
			_Out_writes_opt_(*pNumRects)  D3D11_RECT *pRects);

		virtual void STDMETHODCALLTYPE HSGetShaderResources(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
			/* [annotation] */
			_Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

		virtual void STDMETHODCALLTYPE HSGetShader(
			/* [annotation] */
			_Out_  ID3D11HullShader **ppHullShader,
			/* [annotation] */
			_Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
			/* [annotation] */
			_Inout_opt_  UINT *pNumClassInstances);

		virtual void STDMETHODCALLTYPE HSGetSamplers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
			/* [annotation] */
			_Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

		virtual void STDMETHODCALLTYPE HSGetConstantBuffers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
			/* [annotation] */
			_Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

		virtual void STDMETHODCALLTYPE DSGetShaderResources(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
			/* [annotation] */
			_Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

		virtual void STDMETHODCALLTYPE DSGetShader(
			/* [annotation] */
			_Out_  ID3D11DomainShader **ppDomainShader,
			/* [annotation] */
			_Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
			/* [annotation] */
			_Inout_opt_  UINT *pNumClassInstances);

		virtual void STDMETHODCALLTYPE DSGetSamplers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
			/* [annotation] */
			_Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

		virtual void STDMETHODCALLTYPE DSGetConstantBuffers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
			/* [annotation] */
			_Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

		virtual void STDMETHODCALLTYPE CSGetShaderResources(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
			/* [annotation] */
			_Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews);

		virtual void STDMETHODCALLTYPE CSGetUnorderedAccessViews(
			/* [annotation] */
			_In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - StartSlot)  UINT NumUAVs,
			/* [annotation] */
			_Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews);

		virtual void STDMETHODCALLTYPE CSGetShader(
			/* [annotation] */
			_Out_  ID3D11ComputeShader **ppComputeShader,
			/* [annotation] */
			_Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
			/* [annotation] */
			_Inout_opt_  UINT *pNumClassInstances);

		virtual void STDMETHODCALLTYPE CSGetSamplers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
			/* [annotation] */
			_Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers);

		virtual void STDMETHODCALLTYPE CSGetConstantBuffers(
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
			/* [annotation] */
			_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
			/* [annotation] */
			_Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers);

		virtual void STDMETHODCALLTYPE ClearState(void);

		virtual void STDMETHODCALLTYPE Flush(void);

		virtual D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType(void);

		virtual UINT STDMETHODCALLTYPE GetContextFlags(void);

		virtual HRESULT STDMETHODCALLTYPE FinishCommandList(
			BOOL RestoreDeferredContextState,
			/* [annotation] */
			_Out_opt_  ID3D11CommandList **ppCommandList);

		ID3D11DeviceContext* GetActivePtr() { return GetReal(); }


	public:
		void SaveState(SDeviceContextState* pState);
		void RestoreState(const SDeviceContextState* pState);


	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);

	private:
		bool CheckDeviceStatus();
		void CheckBeforeCommand();

		enum ECommandType { ECT_DRAW_COMMAND, ECT_COMPUTE_COMMAND, ECT_COPY_COMMAND, ECT_UPDATE_COMMAND };
		void CheckAfterCommand(ECommandType commandType);

		void SetConstantBuffers_imp(UINT StartSlot, UINT NumBuffers, 
									ID3D11Buffer *const *ppConstantBuffers,
									void (STDMETHODCALLTYPE ID3D11DeviceContext::* pfn)(UINT, UINT, ID3D11Buffer*const*));

		void SetShaderResources_imp(UINT StartSlot, UINT NumViews,
									ID3D11ShaderResourceView *const *ppShaderResourceViews,
									void (STDMETHODCALLTYPE ID3D11DeviceContext::* pfn)(UINT, UINT, ID3D11ShaderResourceView*const*));

		void SetSamplers_imp(UINT StartSlot, UINT NumSamplers, 
							 ID3D11SamplerState *const *ppSamplers,
							 void (STDMETHODCALLTYPE ID3D11DeviceContext::* pfn)(UINT, UINT, ID3D11SamplerState*const*));

		template <typename UnwrappedType>
		UnwrappedType* Unwrap(UnwrappedType* pWrapped)
		{
			return UnwrapSelf(pWrapped);
		}

		template <typename UnwrappedType>
		UnwrappedType** Unwrap(UINT Num, UnwrappedType* const * pWrappeds, UnwrappedType** pUnwrappeds)
		{
			if (pWrappeds == NULL)
				return NULL;

			for (UINT i = 0; i < Num; ++i)
			{
				pUnwrappeds[i] = Unwrap(pWrappeds[i]);
			}
			return pUnwrappeds;
		}

		template <typename UnwrappedType>
		void Wrap(UnwrappedType** pUnwrappeds, UINT Num = 1)
		{
			if (pUnwrappeds == NULL)
				return;

			for (UINT i = 0; i < Num; ++i)
			{
				UnwrappedType* unwrapped = pUnwrappeds[i];
				if (unwrapped != NULL)
				{
					pUnwrappeds[i] = m_pWrappedDevice->GetWrapper(unwrapped);
					unwrapped->Release();
				}
			}
		}

	private:
		UINT m_SOOffsets[D3D11_SO_BUFFER_SLOT_COUNT];
		WrappedD3DUserAnno m_UserAnno;
		ID3D11Query* m_pFlushQuery;
	};
}
