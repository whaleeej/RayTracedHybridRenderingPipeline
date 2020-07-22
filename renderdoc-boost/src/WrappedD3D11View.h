#pragma once
#include "WrappedD3D11DeviceChild.h"

namespace rdcboost
{
	template <typename NestedType>
	class WrappedD3D11View : public WrappedD3D11DeviceChild<NestedType>
	{
	public:
		WrappedD3D11View(ID3D11Resource* pWrappedResource,
						 NestedType* pReal, WrappedD3D11Device* pDevice) :
						 m_pWrappedResource(pWrappedResource), 
						 WrappedD3D11DeviceChild(pReal, pDevice)
		{
			m_pWrappedResource->AddRef();
		}

		virtual ~WrappedD3D11View()
		{
			m_pWrappedResource->Release();
		}

		virtual void STDMETHODCALLTYPE GetResource(ID3D11Resource **ppResource) 
		{
			if (ppResource)
			{
				*ppResource = m_pWrappedResource;
				m_pWrappedResource->AddRef();
			}
		}

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
		{
			if (riid == __uuidof(ID3D11View))
			{
				*ppvObject = static_cast<ID3D11View*>(this);
				AddRef();
				return S_OK;
			}

			return WrappedD3D11DeviceChild::QueryInterface(riid, ppvObject);
		}

	protected:
		ID3D11Resource* m_pWrappedResource;
	};

	class WrappedD3D11DepthStencilView : public WrappedD3D11View<ID3D11DepthStencilView>
	{
	public:
		WrappedD3D11DepthStencilView(ID3D11Resource* pWrappedResource,
									 ID3D11DepthStencilView* pReal, WrappedD3D11Device* pDevice);

		virtual void STDMETHODCALLTYPE GetDesc(D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);
	};

	class WrappedD3D11RenderTargetView : public WrappedD3D11View<ID3D11RenderTargetView>
	{
	public:
		WrappedD3D11RenderTargetView(ID3D11Resource* pWrappedResource,
									 ID3D11RenderTargetView* pReal, WrappedD3D11Device* pDevice);

		virtual void STDMETHODCALLTYPE GetDesc(D3D11_RENDER_TARGET_VIEW_DESC *pDesc);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);
	};

	class WrappedD3D11ShaderResourceView : public WrappedD3D11View<ID3D11ShaderResourceView>
	{
	public:
		WrappedD3D11ShaderResourceView(ID3D11Resource* pWrappedResource, 
									   ID3D11ShaderResourceView* pReal, WrappedD3D11Device* pDevice);

		virtual void STDMETHODCALLTYPE GetDesc(D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);
	};

	class WrappedD3D11UnorderedAccessView : public WrappedD3D11View<ID3D11UnorderedAccessView>
	{
	public:
		WrappedD3D11UnorderedAccessView(ID3D11Resource* pWrappedResource,
										ID3D11UnorderedAccessView* pReal, WrappedD3D11Device* pDevice);

		virtual void STDMETHODCALLTYPE GetDesc(D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);
	};

}

