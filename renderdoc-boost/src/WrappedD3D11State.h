#pragma once
#include "WrappedD3D11DeviceChild.h"

namespace rdcboost
{
	class WrappedD3D11BlendState : public WrappedD3D11DeviceChild<ID3D11BlendState>
	{
	public:
		WrappedD3D11BlendState(ID3D11BlendState* pReal, WrappedD3D11Device* pDevice);
		virtual void STDMETHODCALLTYPE GetDesc(D3D11_BLEND_DESC *pDesc);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);
	};

	class WrappedD3D11DepthStencilState : public WrappedD3D11DeviceChild<ID3D11DepthStencilState>
	{
	public:
		WrappedD3D11DepthStencilState(ID3D11DepthStencilState* pReal, WrappedD3D11Device* pDevice);
		virtual void STDMETHODCALLTYPE GetDesc(D3D11_DEPTH_STENCIL_DESC *pDesc);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);
	};

	class WrappedD3D11RasterizerState : public WrappedD3D11DeviceChild<ID3D11RasterizerState>
	{
	public:
		WrappedD3D11RasterizerState(ID3D11RasterizerState* pReal, WrappedD3D11Device* pDevice);
		virtual void STDMETHODCALLTYPE GetDesc(D3D11_RASTERIZER_DESC *pDesc);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);
	};

	class WrappedD3D11SamplerState : public WrappedD3D11DeviceChild<ID3D11SamplerState>
	{
	public:
		WrappedD3D11SamplerState(ID3D11SamplerState* pReal, WrappedD3D11Device* pDevice);
		virtual void STDMETHODCALLTYPE GetDesc(D3D11_SAMPLER_DESC *pDesc);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);
	};

	class WrappedD3D11InputLayout : public WrappedD3D11DeviceChild<ID3D11InputLayout>
	{
	public:
		WrappedD3D11InputLayout(ID3D11InputLayout* pReal, WrappedD3D11Device* pDevice);

		virtual ~WrappedD3D11InputLayout();

		void SetCreateParams(const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs, UINT NumElements,
							 const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);

	private:
		D3D11_INPUT_ELEMENT_DESC *m_pInputElementDescs;
		UINT m_NumElements;
		byte* m_pShaderBytecodeWithInputSignature;
		SIZE_T m_BytecodeLength;
	};
}

