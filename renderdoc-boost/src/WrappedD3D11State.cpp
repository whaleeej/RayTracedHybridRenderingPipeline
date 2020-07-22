#include "WrappedD3D11State.h"
#include "Log.h"

namespace rdcboost
{

	WrappedD3D11BlendState::WrappedD3D11BlendState(ID3D11BlendState* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11DeviceChild(pReal, pDevice)
	{
	}

	void STDMETHODCALLTYPE WrappedD3D11BlendState::GetDesc(D3D11_BLEND_DESC *pDesc)
	{
		GetReal()->GetDesc(pDesc);
	}

	ID3D11DeviceChild* WrappedD3D11BlendState::CopyToDevice(ID3D11Device* pNewDevice)
	{
		D3D11_BLEND_DESC blendDesc;
		GetReal()->GetDesc(&blendDesc);

		ID3D11BlendState* pNewState;
		if (FAILED(pNewDevice->CreateBlendState(&blendDesc, &pNewState)))
			LogError("CreateBlendState failed when CopyToDevice");

		return pNewState;
	}

	WrappedD3D11DepthStencilState::WrappedD3D11DepthStencilState(ID3D11DepthStencilState* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11DeviceChild(pReal, pDevice)
	{
	}

	void STDMETHODCALLTYPE WrappedD3D11DepthStencilState::GetDesc(D3D11_DEPTH_STENCIL_DESC *pDesc)
	{
		GetReal()->GetDesc(pDesc);
	}

	ID3D11DeviceChild* WrappedD3D11DepthStencilState::CopyToDevice(ID3D11Device* pNewDevice)
	{
		D3D11_DEPTH_STENCIL_DESC dsDesc;
		GetReal()->GetDesc(&dsDesc);

		ID3D11DepthStencilState* pNewState;
		if (FAILED(pNewDevice->CreateDepthStencilState(&dsDesc, &pNewState)))
			LogError("CreateDepthStencilState failed when CopyToDevice");

		return pNewState;
	}

	WrappedD3D11RasterizerState::WrappedD3D11RasterizerState(ID3D11RasterizerState* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11DeviceChild(pReal, pDevice)
	{
	}

	void STDMETHODCALLTYPE WrappedD3D11RasterizerState::GetDesc(D3D11_RASTERIZER_DESC *pDesc)
	{
		GetReal()->GetDesc(pDesc);
	}

	ID3D11DeviceChild* WrappedD3D11RasterizerState::CopyToDevice(ID3D11Device* pNewDevice)
	{
		D3D11_RASTERIZER_DESC rasterizerDesc;
		GetReal()->GetDesc(&rasterizerDesc);

		ID3D11RasterizerState* pNewState;
		if (FAILED(pNewDevice->CreateRasterizerState(&rasterizerDesc, &pNewState)))
			LogError("CreateRasterizerState failed when CopyToDevice");

		return pNewState;
	}

	WrappedD3D11SamplerState::WrappedD3D11SamplerState(ID3D11SamplerState* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11DeviceChild(pReal, pDevice)
	{
	}

	void STDMETHODCALLTYPE WrappedD3D11SamplerState::GetDesc(D3D11_SAMPLER_DESC *pDesc)
	{
		GetReal()->GetDesc(pDesc);
	}

	ID3D11DeviceChild* WrappedD3D11SamplerState::CopyToDevice(ID3D11Device* pNewDevice)
	{
		D3D11_SAMPLER_DESC samplerDesc;
		GetReal()->GetDesc(&samplerDesc);

		ID3D11SamplerState* pNewState;
		if (FAILED(pNewDevice->CreateSamplerState(&samplerDesc, &pNewState)))
			LogError("CreateSamplerState failed when CopyToDevice");

		return pNewState;
	}

	WrappedD3D11InputLayout::WrappedD3D11InputLayout(
		ID3D11InputLayout* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11DeviceChild(pReal, pDevice), m_pInputElementDescs(NULL),
		m_NumElements(0), m_pShaderBytecodeWithInputSignature(NULL), m_BytecodeLength(0)
	{
	}

	WrappedD3D11InputLayout::~WrappedD3D11InputLayout()
	{
		delete[] m_pInputElementDescs;
		delete[] m_pShaderBytecodeWithInputSignature;
	}

	void WrappedD3D11InputLayout::SetCreateParams(
		const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs, UINT NumElements, 
		const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength)
	{
		m_NumElements = NumElements;
		m_BytecodeLength = BytecodeLength;

		if (pInputElementDescs)
		{
			m_pInputElementDescs = new D3D11_INPUT_ELEMENT_DESC[NumElements];
			memcpy(m_pInputElementDescs, pInputElementDescs, 
				   sizeof(D3D11_INPUT_ELEMENT_DESC) * NumElements);
		}
		
		if (pShaderBytecodeWithInputSignature)
		{
			m_pShaderBytecodeWithInputSignature = new byte[BytecodeLength];
			memcpy(m_pShaderBytecodeWithInputSignature, pShaderBytecodeWithInputSignature, 
				   BytecodeLength);
		}
	}

	ID3D11DeviceChild* WrappedD3D11InputLayout::CopyToDevice(ID3D11Device* pNewDevice)
	{
		ID3D11InputLayout* pNewLayout = NULL;
		HRESULT ret = pNewDevice->CreateInputLayout(
						m_pInputElementDescs, m_NumElements, 
						m_pShaderBytecodeWithInputSignature, m_BytecodeLength, &pNewLayout);

		if (FAILED(ret))
			LogError("CreateInputLayout failed when CopyToDevice");

		return pNewLayout;
	}

}

