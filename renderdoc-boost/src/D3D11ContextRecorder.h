#pragma once
#include "WrappedD3D11Context.h"
#include <stdint.h>

class Serialiser;
namespace rdcboost
{
	class D3D11ContextRecorder : public WrappedD3D11Context
	{
	public:
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

		virtual void STDMETHODCALLTYPE ClearState(void);

		virtual HRESULT STDMETHODCALLTYPE FinishCommandList(
			BOOL RestoreDeferredContextState,
			/* [annotation] */
			_Out_opt_  ID3D11CommandList **ppCommandList);

	private:
		void Serialise_SetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers);


	private:
		ID3D11DeviceContext* m_pActive;
		Serialiser *m_pSerialiser;
		uint64_t m_ResourceID;
	};
}

