#include "WrappedD3D11Device.h"
#include "WrappedD3D11Resource.h"
#include "WrappedD3D11View.h"
#include "WrappedD3D11State.h"
#include "WrappedD3D11Context.h"
#include "WrappedD3D11Shader.h"
#include "WrappedD3D11Async.h"
#include <assert.h>
#include "Log.h"
#include "DeviceContextState.h"
#include "WrappedDXGISwapChain.h"
#include "WrappedD3D11ClassLinkage.h"
#include <d3d11_2.h>

namespace rdcboost
{

	WrappedD3D11Device::WrappedD3D11Device(
		ID3D11Device* pRealDevice, const SDeviceCreateParams& params) :
		m_pReal(pRealDevice), m_pRDCDevice(NULL), m_Ref(1), m_DeviceCreateParams(params),
		m_pWrappedSwapChain(NULL), m_bRenderDocDevice(false)
	{
		m_pReal->AddRef();
		m_DummyInfoQueue.m_pDevice = this;

		ID3D11DeviceContext* pImmediateContext = NULL;
		m_pReal->GetImmediateContext(&pImmediateContext);
		m_pWrappedContext = new WrappedD3D11Context(pImmediateContext, this);
		pImmediateContext->Release();

		for (int i = 0; i < CREATE_COUNT; ++i)
		{
			m_Counter[i].QuadPart = 0;
		}

		QueryPerformanceFrequency(&m_Frequency);
	}

	WrappedD3D11Device::~WrappedD3D11Device()
	{
		Assert(m_pWrappedContext == NULL);
		m_pReal->Release();
		if (m_pRDCDevice != NULL)
			m_pRDCDevice->Release();
	}

	class StopWatch
	{
	public:
		StopWatch(LARGE_INTEGER* result) : m_Result(result)
		{
			QueryPerformanceCounter(&m_BeginTime);
		}

		~StopWatch()
		{
			LARGE_INTEGER endTime;
			QueryPerformanceCounter(&endTime);
			m_Result->QuadPart += (endTime.QuadPart - m_BeginTime.QuadPart);
		}

	private:
		LARGE_INTEGER* m_Result;
		LARGE_INTEGER m_BeginTime;
	};

	HRESULT WrappedD3D11Device::CreateBuffer(const D3D11_BUFFER_DESC *pDesc,
											 const D3D11_SUBRESOURCE_DATA *pInitialData, 
											 ID3D11Buffer **ppBuffer)
	{
		StopWatch s(&m_Counter[CREATE_BUFFER]);
		if (ppBuffer == NULL)
			return m_pReal->CreateBuffer(pDesc, pInitialData, NULL);

		ID3D11Buffer* pRealBuffer = NULL;
		HRESULT ret = m_pReal->CreateBuffer(pDesc, pInitialData, &pRealBuffer);
		if (pRealBuffer)
		{
			WrappedD3D11Buffer* wrapped = new WrappedD3D11Buffer(pRealBuffer, this);
			*ppBuffer = wrapped;
			m_BackRefs[pRealBuffer] = wrapped;
			pRealBuffer->Release();
		}
		else
		{
			*ppBuffer = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateTexture1D(const D3D11_TEXTURE1D_DESC *pDesc, 
												const D3D11_SUBRESOURCE_DATA *pInitialData, 
												ID3D11Texture1D **ppTexture1D)
	{
		StopWatch s(&m_Counter[CREATE_TEXTURE]);
		if (ppTexture1D == NULL)
			return m_pReal->CreateTexture1D(pDesc, pInitialData, NULL);

		ID3D11Texture1D* pRealTex1D = NULL;
		HRESULT ret = m_pReal->CreateTexture1D(pDesc, pInitialData, &pRealTex1D);
		if (pRealTex1D)
		{
			WrappedD3D11Texture1D* wrapped = new WrappedD3D11Texture1D(pRealTex1D, this);
			*ppTexture1D = wrapped;
			m_BackRefs[pRealTex1D] = wrapped;
			pRealTex1D->Release();
		}
		else
		{
			*ppTexture1D = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateTexture2D(const D3D11_TEXTURE2D_DESC *pDesc, 
												const D3D11_SUBRESOURCE_DATA *pInitialData, 
												ID3D11Texture2D **ppTexture2D)
	{
		StopWatch s(&m_Counter[CREATE_TEXTURE]);
		if (ppTexture2D == NULL)
			return m_pReal->CreateTexture2D(pDesc, pInitialData, NULL);

		ID3D11Texture2D* pRealTex2D = NULL;
		HRESULT ret = m_pReal->CreateTexture2D(pDesc, pInitialData, &pRealTex2D);
		if (pRealTex2D)
		{
			WrappedD3D11Texture2D* wrapped = new WrappedD3D11Texture2D(pRealTex2D, this);
			*ppTexture2D = wrapped;
			m_BackRefs[pRealTex2D] = wrapped;
			pRealTex2D->Release();
		}
		else
		{
			*ppTexture2D = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateTexture3D(const D3D11_TEXTURE3D_DESC *pDesc, 
												const D3D11_SUBRESOURCE_DATA *pInitialData, 
												ID3D11Texture3D **ppTexture3D)
	{
		StopWatch s(&m_Counter[CREATE_TEXTURE]);
		if (ppTexture3D == NULL)
			return m_pReal->CreateTexture3D(pDesc, pInitialData, NULL);

		ID3D11Texture3D* pRealTex3D = NULL;
		HRESULT ret = m_pReal->CreateTexture3D(pDesc, pInitialData, &pRealTex3D);
		if (pRealTex3D)
		{
			WrappedD3D11Texture3D* wrapped = new WrappedD3D11Texture3D(pRealTex3D, this);
			*ppTexture3D = wrapped;
			m_BackRefs[pRealTex3D] = wrapped;
			pRealTex3D->Release();
		}
		else
		{
			*ppTexture3D = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateShaderResourceView(ID3D11Resource *pResource, 
														 const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc, 
														 ID3D11ShaderResourceView **ppSRView)
	{
		StopWatch s(&m_Counter[CREATE_VIEW]);
		ID3D11Resource* pRealResource = UnwrapSelf(pResource);
		if (ppSRView == NULL)
			return m_pReal->CreateShaderResourceView(pRealResource, pDesc, NULL);

		ID3D11ShaderResourceView* pRealSRV = NULL;
		HRESULT ret = m_pReal->CreateShaderResourceView(pRealResource,  pDesc, &pRealSRV);
		if (pRealSRV)
		{
			WrappedD3D11ShaderResourceView* wrapped = 
				new WrappedD3D11ShaderResourceView(pResource, pRealSRV, this);
			*ppSRView = wrapped;
			m_BackRefs[pRealSRV] = wrapped;
			pRealSRV->Release();
		}
		else
		{
			*ppSRView = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateUnorderedAccessView(ID3D11Resource *pResource, 
														  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc, 
														  ID3D11UnorderedAccessView **ppUAView)
	{
		StopWatch s(&m_Counter[CREATE_VIEW]);
		ID3D11Resource* pRealResource = UnwrapSelf(pResource);

		if (ppUAView == NULL)
			return m_pReal->CreateUnorderedAccessView(pRealResource, pDesc, NULL);
		
		ID3D11UnorderedAccessView* pRealUAV = NULL;
		HRESULT ret = m_pReal->CreateUnorderedAccessView(pRealResource, pDesc, &pRealUAV);
		if (pRealUAV)
		{
			WrappedD3D11UnorderedAccessView* wrapped = 
				new WrappedD3D11UnorderedAccessView(pResource, pRealUAV, this);
			*ppUAView = wrapped;
			m_BackRefs[pRealUAV] = wrapped;
			pRealUAV->Release();
		}
		else
		{
			*ppUAView = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateRenderTargetView(ID3D11Resource *pResource, 
													   const D3D11_RENDER_TARGET_VIEW_DESC *pDesc, 
													   ID3D11RenderTargetView **ppRTView)
	{
		StopWatch s(&m_Counter[CREATE_VIEW]);
		ID3D11Resource* pRealResource = UnwrapSelf(pResource);
		if (ppRTView == NULL)
			return m_pReal->CreateRenderTargetView(pRealResource, pDesc, NULL);

		ID3D11RenderTargetView* pRealRTV = NULL;
		HRESULT ret = m_pReal->CreateRenderTargetView(pRealResource, pDesc, &pRealRTV);
		if (pRealRTV)
		{
			WrappedD3D11RenderTargetView* wrapped = 
				new WrappedD3D11RenderTargetView(pResource, pRealRTV, this);
			*ppRTView = wrapped;
			m_BackRefs[pRealRTV] = wrapped;
			pRealRTV->Release();
		}
		else
		{
			*ppRTView = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateDepthStencilView(ID3D11Resource *pResource, 
													   const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc, 
													   ID3D11DepthStencilView **ppDepthStencilView)
	{
		StopWatch s(&m_Counter[CREATE_VIEW]);
		ID3D11Resource* pRealResource = UnwrapSelf(pResource);
		if (ppDepthStencilView == NULL)
			return m_pReal->CreateDepthStencilView(pRealResource, pDesc, NULL);

		ID3D11DepthStencilView* pRealDSV = NULL;
		HRESULT ret = m_pReal->CreateDepthStencilView(pRealResource, pDesc, &pRealDSV);
		if (pRealDSV)
		{
			WrappedD3D11DepthStencilView* wrapped = 
				new WrappedD3D11DepthStencilView(pResource, pRealDSV, this);
			*ppDepthStencilView = wrapped;
			m_BackRefs[pRealDSV] = wrapped;
			pRealDSV->Release();
		}
		else
		{
			*ppDepthStencilView = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateInputLayout(
		const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs, UINT NumElements, 
		const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, 
		ID3D11InputLayout **ppInputLayout)
	{
		StopWatch s(&m_Counter[CREATE_STATE]);
		if (ppInputLayout == NULL)
			return m_pReal->CreateInputLayout(pInputElementDescs, NumElements, 
											  pShaderBytecodeWithInputSignature, 
											  BytecodeLength, NULL);


		ID3D11InputLayout* pRealLayout = NULL;
		HRESULT ret = m_pReal->CreateInputLayout(pInputElementDescs, NumElements, 
												 pShaderBytecodeWithInputSignature, 
												 BytecodeLength, &pRealLayout);
		if (pRealLayout)
		{
			WrappedD3D11InputLayout* wrapped = new WrappedD3D11InputLayout(pRealLayout, this);
			wrapped->SetCreateParams(pInputElementDescs, NumElements,
									 pShaderBytecodeWithInputSignature, BytecodeLength);

			pRealLayout->Release();
			*ppInputLayout = wrapped;
			m_BackRefs[pRealLayout] = wrapped;
		}
		else
		{
			*ppInputLayout = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateVertexShader(const void *pShaderBytecode, 
												   SIZE_T BytecodeLength, 
												   ID3D11ClassLinkage *pClassLinkage, 
												   ID3D11VertexShader **ppVertexShader)
	{
		StopWatch s(&m_Counter[CREATE_SHADER]);
		HRESULT ret = m_pReal->CreateVertexShader(pShaderBytecode, BytecodeLength, 
												  UnwrapSelf(pClassLinkage), ppVertexShader);
		if (ppVertexShader && *ppVertexShader)
		{
			ID3D11VertexShader* pRealShader = *ppVertexShader;
			*ppVertexShader = new WrappedD3D11VertexShader(
									pShaderBytecode, BytecodeLength, 
									static_cast<WrappedD3D11ClassLinkage*>(pClassLinkage), 
									pRealShader, this);

			m_BackRefs[pRealShader] = static_cast<WrappedD3D11VertexShader*>(*ppVertexShader);
			pRealShader->Release();
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateGeometryShader(const void *pShaderBytecode, 
													 SIZE_T BytecodeLength, 
													 ID3D11ClassLinkage *pClassLinkage, 
													 ID3D11GeometryShader **ppGeometryShader)
	{
		StopWatch s(&m_Counter[CREATE_SHADER]);
		HRESULT ret = m_pReal->CreateGeometryShader(pShaderBytecode, BytecodeLength,
												    UnwrapSelf(pClassLinkage), ppGeometryShader);
		if (ppGeometryShader && *ppGeometryShader)
		{
			ID3D11GeometryShader* pRealShader = *ppGeometryShader;

			SGeometryShaderSOInfo soInfo;
			soInfo.withSO = false;
			*ppGeometryShader = new WrappedD3D11GeometryShader(
										pShaderBytecode, BytecodeLength, soInfo, 
										static_cast<WrappedD3D11ClassLinkage*>(pClassLinkage), 
										pRealShader, this);

			m_BackRefs[pRealShader] = static_cast<WrappedD3D11GeometryShader*>(*ppGeometryShader);
			pRealShader->Release();
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateGeometryShaderWithStreamOutput(
		const void *pShaderBytecode, SIZE_T BytecodeLength, 
		const D3D11_SO_DECLARATION_ENTRY *pSODeclaration, UINT NumEntries, 
		const UINT *pBufferStrides, UINT NumStrides, UINT RasterizedStream, 
		ID3D11ClassLinkage *pClassLinkage, ID3D11GeometryShader **ppGeometryShader)
	{ 
		StopWatch s(&m_Counter[CREATE_SHADER]);
		HRESULT ret = m_pReal->CreateGeometryShaderWithStreamOutput(
							pShaderBytecode, BytecodeLength, pSODeclaration, NumEntries,
							pBufferStrides, NumStrides, RasterizedStream,
							UnwrapSelf(pClassLinkage), ppGeometryShader);
		if (ppGeometryShader && *ppGeometryShader)
		{
			ID3D11GeometryShader* pRealShader = *ppGeometryShader;
			SGeometryShaderSOInfo info;
			info.withSO = true;
			info.SODeclaration.insert(info.SODeclaration.end(), 
									  pSODeclaration, pSODeclaration + NumEntries);
			for (auto it = info.SODeclaration.begin(); it != info.SODeclaration.end(); ++it)
			{
				size_t len = strlen(it->SemanticName);
				char* newSemanticName = new char[len + 1];
				memcpy(newSemanticName, it->SemanticName, len + 1);
				it->SemanticName = newSemanticName;
			}

			info.BufferStrides.insert(info.BufferStrides.end(), 
									  pBufferStrides, pBufferStrides + NumStrides);
			info.rasterizedStream = RasterizedStream;
			*ppGeometryShader = new WrappedD3D11GeometryShader(
				pShaderBytecode, BytecodeLength, info,
				static_cast<WrappedD3D11ClassLinkage*>(pClassLinkage),
				pRealShader, this);

			m_BackRefs[pRealShader] = static_cast<WrappedD3D11GeometryShader*>(*ppGeometryShader);
			pRealShader->Release();
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreatePixelShader(const void *pShaderBytecode, 
												  SIZE_T BytecodeLength, 
												  ID3D11ClassLinkage *pClassLinkage, 
												  ID3D11PixelShader **ppPixelShader)
	{
		StopWatch s(&m_Counter[CREATE_SHADER]);
		HRESULT ret = m_pReal->CreatePixelShader(pShaderBytecode, BytecodeLength,
												 UnwrapSelf(pClassLinkage), ppPixelShader);
		if (ppPixelShader && *ppPixelShader)
		{
			ID3D11PixelShader* pRealShader = *ppPixelShader;
			*ppPixelShader = new WrappedD3D11PixelShader(
									pShaderBytecode, BytecodeLength, 
									static_cast<WrappedD3D11ClassLinkage*>(pClassLinkage),
									pRealShader, this);

			m_BackRefs[pRealShader] = static_cast<WrappedD3D11PixelShader*>(*ppPixelShader);
			pRealShader->Release();
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateHullShader(const void *pShaderBytecode, 
												 SIZE_T BytecodeLength, 
												 ID3D11ClassLinkage *pClassLinkage, 
												 ID3D11HullShader **ppHullShader)
	{
		StopWatch s(&m_Counter[CREATE_SHADER]);
		HRESULT ret = m_pReal->CreateHullShader(pShaderBytecode, BytecodeLength,
												UnwrapSelf(pClassLinkage), ppHullShader);
		if (ppHullShader && *ppHullShader)
		{
			ID3D11HullShader* pRealShader = *ppHullShader;
			*ppHullShader = new WrappedD3D11HullShader(
								pShaderBytecode, BytecodeLength,
								static_cast<WrappedD3D11ClassLinkage*>(pClassLinkage),
								pRealShader, this);

			m_BackRefs[pRealShader] = static_cast<WrappedD3D11HullShader*>(*ppHullShader);
			pRealShader->Release();
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateDomainShader(const void *pShaderBytecode, 
												   SIZE_T BytecodeLength, 
												   ID3D11ClassLinkage *pClassLinkage, 
												   ID3D11DomainShader **ppDomainShader)
	{
		StopWatch s(&m_Counter[CREATE_SHADER]);
		HRESULT ret = m_pReal->CreateDomainShader(pShaderBytecode, BytecodeLength,
												  UnwrapSelf(pClassLinkage), ppDomainShader);
		if (ppDomainShader && *ppDomainShader)
		{
			ID3D11DomainShader* pRealShader = *ppDomainShader;
			*ppDomainShader = new WrappedD3D11DomainShader(
									pShaderBytecode, BytecodeLength,
									static_cast<WrappedD3D11ClassLinkage*>(pClassLinkage),
									pRealShader, this);

			m_BackRefs[pRealShader] = static_cast<WrappedD3D11DomainShader*>(*ppDomainShader);
			pRealShader->Release();
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateComputeShader(const void *pShaderBytecode, 
													SIZE_T BytecodeLength, 
													ID3D11ClassLinkage *pClassLinkage, 
													ID3D11ComputeShader **ppComputeShader)
	{
		StopWatch s(&m_Counter[CREATE_SHADER]);
		HRESULT ret = m_pReal->CreateComputeShader(pShaderBytecode, BytecodeLength,
												   UnwrapSelf(pClassLinkage), ppComputeShader);
		if (ppComputeShader && *ppComputeShader)
		{
			ID3D11ComputeShader* pRealShader = *ppComputeShader;
			*ppComputeShader = new WrappedD3D11ComputeShader(
									pShaderBytecode, BytecodeLength,
									static_cast<WrappedD3D11ClassLinkage*>(pClassLinkage),
									pRealShader, this);

			m_BackRefs[pRealShader] = static_cast<WrappedD3D11ComputeShader*>(*ppComputeShader);
			pRealShader->Release();
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateClassLinkage(ID3D11ClassLinkage **ppLinkage)
	{
		HRESULT res = m_pReal->CreateClassLinkage(ppLinkage);
		if (ppLinkage && *ppLinkage)
		{
			ID3D11ClassLinkage* pReal = *ppLinkage;
			*ppLinkage = new WrappedD3D11ClassLinkage(pReal, this);
			m_BackRefs[pReal] = static_cast<WrappedD3D11ClassLinkage*>(*ppLinkage);
			pReal->Release();
		}

		return res;
	}

	HRESULT WrappedD3D11Device::CreateBlendState(const D3D11_BLEND_DESC *pBlendStateDesc, 
												 ID3D11BlendState **ppBlendState)
	{
		StopWatch s(&m_Counter[CREATE_STATE]);
		if (ppBlendState == NULL)
			return m_pReal->CreateBlendState(pBlendStateDesc, NULL);

		ID3D11BlendState* pRealState = NULL;
		HRESULT ret = m_pReal->CreateBlendState(pBlendStateDesc, &pRealState);
		if (pRealState)
		{
			ID3D11BlendState* pWrapper = GetWrapper(pRealState);
			if (pWrapper != NULL)
			{
				*ppBlendState = pWrapper;
			}
			else
			{
				WrappedD3D11BlendState* wrapped = new WrappedD3D11BlendState(pRealState, this);
				*ppBlendState = wrapped;
				m_BackRefs[pRealState] = wrapped;
			}

			pRealState->Release();
		}
		else
		{
			*ppBlendState = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateDepthStencilState(
		const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc, 
		ID3D11DepthStencilState **ppDepthStencilState)
	{
		StopWatch s(&m_Counter[CREATE_STATE]);
		if (ppDepthStencilState == NULL)
			return m_pReal->CreateDepthStencilState(pDepthStencilDesc, NULL);

		ID3D11DepthStencilState* pRealState = NULL;
		HRESULT ret = m_pReal->CreateDepthStencilState(pDepthStencilDesc, &pRealState);
		if (pRealState)
		{
			ID3D11DepthStencilState* pWrapper = GetWrapper(pRealState);
			if (pWrapper != NULL)
			{
				*ppDepthStencilState = pWrapper;
			}
			else
			{
				WrappedD3D11DepthStencilState* wrapped = new WrappedD3D11DepthStencilState(pRealState, this);
				*ppDepthStencilState = wrapped;
				m_BackRefs[pRealState] = wrapped;
			}

			pRealState->Release();
		}
		else
		{
			*ppDepthStencilState = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateRasterizerState(
		const D3D11_RASTERIZER_DESC *pRasterizerDesc, 
		ID3D11RasterizerState **ppRasterizerState)
	{
		StopWatch s(&m_Counter[CREATE_STATE]);
		if (ppRasterizerState == NULL)
			return m_pReal->CreateRasterizerState(pRasterizerDesc, NULL);

		ID3D11RasterizerState* pRealState = NULL;
		HRESULT ret = m_pReal->CreateRasterizerState(pRasterizerDesc, &pRealState);
		if (pRealState)
		{
			ID3D11RasterizerState* pWrapper = GetWrapper(pRealState);
			if (pWrapper != NULL)
			{
				*ppRasterizerState = pWrapper;
			}
			else
			{
				WrappedD3D11RasterizerState* wrapped = new WrappedD3D11RasterizerState(pRealState, this);
				*ppRasterizerState = wrapped;
				m_BackRefs[pRealState] = wrapped;
			}

			pRealState->Release();
		}
		else
		{
			*ppRasterizerState = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateSamplerState(const D3D11_SAMPLER_DESC *pSamplerDesc, 
												   ID3D11SamplerState **ppSamplerState)
	{
		StopWatch s(&m_Counter[CREATE_STATE]);
		if (ppSamplerState == NULL)
			return m_pReal->CreateSamplerState(pSamplerDesc, NULL);

		ID3D11SamplerState* pRealState = NULL;
		HRESULT ret = m_pReal->CreateSamplerState(pSamplerDesc, &pRealState);
		if (pRealState)
		{
			ID3D11SamplerState* pWrapper = GetWrapper(pRealState);
			if (pWrapper != NULL)
			{
				*ppSamplerState = pWrapper;
			}
			else
			{
				WrappedD3D11SamplerState* wrapped = new WrappedD3D11SamplerState(pRealState, this);
				*ppSamplerState = wrapped;
				m_BackRefs[pRealState] = wrapped;
			}

			pRealState->Release();
		}
		else
		{
			*ppSamplerState = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateQuery(const D3D11_QUERY_DESC *pQueryDesc, 
											ID3D11Query **ppQuery)
	{
		StopWatch s(&m_Counter[CREATE_STATE]);
		if (ppQuery == NULL)
			return m_pReal->CreateQuery(pQueryDesc, NULL);

		ID3D11Query* pRealQuery = NULL;
		HRESULT ret = m_pReal->CreateQuery(pQueryDesc, &pRealQuery);
		if (pRealQuery)
		{
			WrappedD3D11Query* wrapped = new WrappedD3D11Query(pRealQuery, this);
			pRealQuery->Release();
			*ppQuery = wrapped;
			m_BackRefs[pRealQuery] = wrapped;
		}
		else
		{
			*ppQuery = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreatePredicate(const D3D11_QUERY_DESC *pPredicateDesc, 
												ID3D11Predicate **ppPredicate)
	{
		StopWatch s(&m_Counter[CREATE_STATE]);
		if (ppPredicate == NULL)
			return m_pReal->CreatePredicate(pPredicateDesc, NULL);

		ID3D11Predicate* pRealPredicate = NULL;
		HRESULT ret = m_pReal->CreatePredicate(pPredicateDesc, &pRealPredicate);
		if (pRealPredicate)
		{
			WrappedD3D11Predicate* wrapped = new WrappedD3D11Predicate(pRealPredicate, this);
			pRealPredicate->Release();
			*ppPredicate = wrapped;
			m_BackRefs[pRealPredicate] = wrapped;
		}
		else
		{
			*ppPredicate = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateCounter(const D3D11_COUNTER_DESC *pCounterDesc, 
											  ID3D11Counter **ppCounter)
	{
		StopWatch s(&m_Counter[CREATE_STATE]);
		if (ppCounter == NULL)
			return m_pReal->CreateCounter(pCounterDesc, NULL);

		ID3D11Counter* pRealCounter = NULL;
		HRESULT ret = m_pReal->CreateCounter(pCounterDesc, &pRealCounter);
		if (pRealCounter)
		{
			WrappedD3D11Counter* wrapped = new WrappedD3D11Counter(pRealCounter, this);
			pRealCounter->Release();
			*ppCounter = wrapped;
			m_BackRefs[pRealCounter] = wrapped;
		}
		else
		{
			*ppCounter = NULL;
		}

		return ret;
	}

	HRESULT WrappedD3D11Device::CreateDeferredContext(UINT ContextFlags, 
													  ID3D11DeviceContext **ppDeferredContext)
	{
		LogError("CreateDeferredContext is not supported by now");

		if (ppDeferredContext)
			*ppDeferredContext = NULL;

		return E_FAIL;
	}

	HRESULT WrappedD3D11Device::OpenSharedResource(HANDLE hResource, 
												   REFIID ReturnedInterface, void **ppResource)
	{
		return m_pReal->OpenSharedResource(hResource, ReturnedInterface, ppResource);
	}

	HRESULT WrappedD3D11Device::CheckFormatSupport(DXGI_FORMAT Format, UINT *pFormatSupport)
	{
		return m_pReal->CheckFormatSupport(Format, pFormatSupport);
	}

	HRESULT WrappedD3D11Device::CheckMultisampleQualityLevels(DXGI_FORMAT Format, 
															  UINT SampleCount, 
															  UINT *pNumQualityLevels)
	{
		return m_pReal->CheckMultisampleQualityLevels(Format, SampleCount, pNumQualityLevels);
	}

	void WrappedD3D11Device::CheckCounterInfo(D3D11_COUNTER_INFO *pCounterInfo)
	{
		m_pReal->CheckCounterInfo(pCounterInfo);
	}

	HRESULT WrappedD3D11Device::CheckCounter(const D3D11_COUNTER_DESC *pDesc, 
											 D3D11_COUNTER_TYPE *pType, UINT *pActiveCounters, 
											 LPSTR szName, UINT *pNameLength, LPSTR szUnits, 
											 UINT *pUnitsLength, LPSTR szDescription, 
											 UINT *pDescriptionLength)
	{
		return m_pReal->CheckCounter(pDesc, pType, pActiveCounters,
									 szName, pNameLength, szUnits, 
									 pUnitsLength, szDescription, pDescriptionLength);
	}

	HRESULT WrappedD3D11Device::CheckFeatureSupport(D3D11_FEATURE Feature, 
													void *pFeatureSupportData, 
													UINT FeatureSupportDataSize)
	{
		return m_pReal->CheckFeatureSupport(Feature, pFeatureSupportData, 
											FeatureSupportDataSize);
	}

	HRESULT WrappedD3D11Device::GetPrivateData(REFGUID guid, UINT *pDataSize, void *pData)
	{
		return m_pReal->GetPrivateData(guid, pDataSize, pData);
	}

	HRESULT WrappedD3D11Device::SetPrivateData(REFGUID guid, UINT DataSize, const void *pData)
	{
		m_PrivateDatas.SetPrivateData(guid, DataSize, pData);
		return m_pReal->SetPrivateData(guid, DataSize, pData);
	}

	HRESULT WrappedD3D11Device::SetPrivateDataInterface(REFGUID guid, const IUnknown *pData)
	{
		m_PrivateDatas.SetPrivateDataInterface(guid, pData);
		return m_pReal->SetPrivateDataInterface(guid, pData);
	}

	D3D_FEATURE_LEVEL WrappedD3D11Device::GetFeatureLevel(void)
	{
		return m_pReal->GetFeatureLevel();
	}

	UINT WrappedD3D11Device::GetCreationFlags(void)
	{
		return m_pReal->GetCreationFlags();
	}

	HRESULT WrappedD3D11Device::GetDeviceRemovedReason(void)
	{
		return m_pReal->GetDeviceRemovedReason();
	}

	void WrappedD3D11Device::GetImmediateContext(ID3D11DeviceContext **ppImmediateContext)
	{
		if (ppImmediateContext)
		{
			*ppImmediateContext = m_pWrappedContext;
			m_pWrappedContext->AddRef();
		}
	}

	HRESULT WrappedD3D11Device::SetExceptionMode(UINT RaiseFlags)
	{
		return m_pReal->SetExceptionMode(RaiseFlags);
	}

	UINT WrappedD3D11Device::GetExceptionMode(void)
	{
		return m_pReal->GetExceptionMode();
	}

	HRESULT WrappedD3D11Device::QueryInterface(REFIID riid, void** ppvObject)
	{
		HRESULT hr = S_OK;
		if (riid == __uuidof(ID3D11Device))
		{
			AddRef();
			*ppvObject = static_cast<ID3D11Device*>(this);
			return S_OK;
		}
		else if (riid == __uuidof(IUnknown))
		{
			AddRef();
			*ppvObject = static_cast<IUnknown*>(this);
			return S_OK;
		}
		else
		{
			if (riid == __uuidof(IDXGIDevice))
			{
				LogError("Querying ID3D11Device for IDXGIDevice, not support");
			}
			else if (riid == __uuidof(IDXGIDevice1))
			{
				LogError("Querying ID3D11Device for IDXGIDevice1, not support");
			}
			else if (riid == __uuidof(IDXGIDevice2))
			{
				LogError("Querying ID3D11Device for IDXGIDevice2, not support");
			}
			else if (riid == __uuidof(IDXGIDevice3))
			{
				LogError("Querying ID3D11Device for IDXGIDevice3, not support");
			}
			else if (riid == __uuidof(ID3D11Device1))
			{
				LogError("Querying ID3D11Device for ID3D11Device1, not support");
			}
			else if (riid == __uuidof(ID3D11Device2))
			{
				LogError("Querying ID3D11Device for ID3D11Device2, not support");
			}
			else if (riid == __uuidof(ID3D10Multithread))
			{
				LogError("Querying ID3D11Device for ID3D10Multithread, not support");
			}
			else if (riid == __uuidof(ID3D11InfoQueue))
			{
// 				LogError("Querying ID3D11Device for ID3D11InfoQueue, not support");
				*ppvObject = static_cast<ID3D11InfoQueue*>(&m_DummyInfoQueue);
				m_DummyInfoQueue.AddRef();
				return S_OK;
			}
			else if (riid == __uuidof(ID3D11Debug))
			{
				LogError("Querying ID3D11Device for ID3D11Debug, not support");
			}
			else
			{
				LogError("Querying ID3D11Device for unknown interface");
			}
			return E_NOINTERFACE;
		}
	}

	ULONG WrappedD3D11Device::AddRef(void)
	{
		return InterlockedIncrement(&m_Ref);
	}

	ULONG WrappedD3D11Device::Release(void)
	{
		unsigned int ret = InterlockedDecrement(&m_Ref);
		if (ret == 2) // only soft refs.
		{
			TryToRelease();
		}

		return ret;
	}

	void WrappedD3D11Device::SwitchToDevice(ID3D11Device* pNewDevice, IDXGISwapChain* pNewSwapChain)
	{
		// 1. copy states of the old device to the new one.
		pNewDevice->SetExceptionMode(m_pReal->GetExceptionMode());
		m_PrivateDatas.CopyPrivateData(pNewDevice);

		// 2. save states of the old immediate device context to the new one.
		SDeviceContextState deviceContextState;
		m_pWrappedContext->SaveState(&deviceContextState);

		// 3. copy resources to the new device.
		// SwapChainBuffers need to do switch before other resources.
		if (m_pWrappedSwapChain)
			m_pWrappedSwapChain->SwitchToDevice(pNewSwapChain);

		for (UINT Buffer = 0; Buffer < m_SwapChainBuffers.size(); ++Buffer)
		{
			if (m_SwapChainBuffers[Buffer] != NULL)
			{
				ID3D11Texture2D* pSurface = NULL;
				pNewSwapChain->GetBuffer(Buffer, __uuidof(ID3D11Texture2D), (void**)&pSurface);
				Assert(pSurface != NULL);

				m_SwapChainBuffers[Buffer]->SwitchToDeviceForSwapChainBuffer(pNewDevice, pSurface);
			}
		}

		if (!m_BackRefs.empty())
		{
			printf("Transferring resource to new device:\n");
			printf("--------------------------------------------------\n");
			std::map<ID3D11DeviceChild*, WrappedD3D11DeviceChildBase*> newBackRefs;
			int progress = 0, idx = 0;
			for (auto it = m_BackRefs.begin(); it != m_BackRefs.end(); ++it)
			{
				it->second->SwitchToDevice(pNewDevice);
				newBackRefs[it->second->GetRealDeviceChild()] = it->second;

				++idx;
				while (progress < (int) (idx * 50 / m_BackRefs.size()))
				{
					printf(">");
					++progress;
				}
			}
			printf("\n");
			m_BackRefs.swap(newBackRefs);
		}

		// 4. restore states of the old immediate device context to the new one.
		m_pWrappedContext->SwitchToDevice(pNewDevice);
		m_pWrappedContext->RestoreState(&deviceContextState);

		ULONG refs = m_pReal->Release();
		if (refs != 0)
			LogError("Previous real device ref count: %d", (int) refs);

		pNewDevice->AddRef();
		m_pReal = pNewDevice;
	}

	ID3D11Texture2D* WrappedD3D11Device::GetWrappedSwapChainBuffer(UINT Buffer, 
																   ID3D11Texture2D *realSurface)
	{
		Assert(realSurface != NULL);
		ID3D11Texture2D* tex = NULL;
		if (Buffer < m_SwapChainBuffers.size() && m_SwapChainBuffers[Buffer] != NULL)
		{
			if (m_SwapChainBuffers[Buffer]->GetReal() == realSurface)
			{
				tex = m_SwapChainBuffers[Buffer];
				tex->AddRef();
			}
			else
			{
				LogWarn("Previous swap chain buffer isn't released by user.");
				m_SwapChainBuffers[Buffer] = NULL;
			}
		}

		if (tex == NULL)
		{
			m_SwapChainBuffers.resize(Buffer + 1);
			tex = m_SwapChainBuffers[Buffer] = new WrappedD3D11Texture2D(realSurface, this);
		}

		return tex;
	}

	void WrappedD3D11Device::InitSwapChain(WrappedDXGISwapChain* pWrappedSwapchain)
	{
		m_pWrappedSwapChain = pWrappedSwapchain;
		m_pWrappedSwapChain->AddRef();
	}

	void WrappedD3D11Device::TryToRelease()
	{
		unsigned int softRef = 0;
		if (m_pWrappedContext) ++softRef;
		if (m_pWrappedSwapChain) ++softRef;

		if (m_Ref == softRef && 
			(!m_pWrappedContext || m_pWrappedContext->GetRef() == 1) &&
			(!m_pWrappedSwapChain || m_pWrappedSwapChain->GetRef() == 1))
		{
			SAFE_RELEASE(m_pWrappedContext);
			SAFE_RELEASE(m_pWrappedSwapChain);
			delete this;
		}
	}

	void WrappedD3D11Device::OnDeviceChildReleased(ID3D11DeviceChild* pChild)
	{
		if (m_BackRefs.erase(pChild) != 0)
			return;

		for (auto it = m_SwapChainBuffers.begin(); it != m_SwapChainBuffers.end(); ++it)
		{
			if (*it && (*it)->GetReal() == pChild)
			{
				*it = NULL;
				return;
			}
		}

		LogError("Unknown device child released.");
	}

	void WrappedD3D11Device::OnFramePresent()
	{
// 		double timeInMs[CREATE_COUNT];
// 		for (int i = 0; i < CREATE_COUNT; ++i)
// 		{
// 			timeInMs[i] = (double)m_Counter[i].QuadPart * 1000.0 / m_Frequency.QuadPart;
// 		}
// 
// 		char buf[1024];
// 		sprintf_s(buf, "CreateBuffer: %.3fms, CreateTexture: %.3fms, CreateView: %.3fms, "
// 					 "CreateState: %.3fms, CreateShader: %.3fms", 
// 					 timeInMs[0], timeInMs[1], timeInMs[2], 
// 					 timeInMs[3], timeInMs[4]);
// 
// 		LogInfo("%s", buf);

		for (int i = 0; i < CREATE_COUNT; ++i)
		{
			m_Counter[i].QuadPart = 0;
		}
	}

	ULONG STDMETHODCALLTYPE DummyID3D11InfoQueue::AddRef()
	{
		return m_pDevice->AddRef();
	}

	ULONG STDMETHODCALLTYPE DummyID3D11InfoQueue::Release()
	{
		return m_pDevice->Release();
	}

}

