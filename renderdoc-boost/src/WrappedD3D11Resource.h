#pragma once
#include <d3d11.h>
#include "WrappedD3D11DeviceChild.h"

namespace rdcboost
{
	template <typename NestedType>
	class WrappedD3D11Resource : public WrappedD3D11DeviceChild<NestedType>
	{
	public:
		WrappedD3D11Resource(NestedType* pReal, WrappedD3D11Device* pDevice) :
			WrappedD3D11DeviceChild(pReal, pDevice)
		{
		}

		virtual void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION *pResourceDimension)
		{
			GetReal()->GetType(pResourceDimension);
		}

		virtual void STDMETHODCALLTYPE SetEvictionPriority(UINT EvictionPriority)
		{
			GetReal()->SetEvictionPriority(EvictionPriority);
		}

		virtual UINT STDMETHODCALLTYPE GetEvictionPriority(void)
		{
			return GetReal()->GetEvictionPriority();
		}

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
		{
			if (riid == __uuidof(ID3D11Resource))
			{
				*ppvObject = static_cast<ID3D11Resource*>(this);
				AddRef();
				return S_OK;
			}

			return WrappedD3D11DeviceChild::QueryInterface(riid, ppvObject);
		}
	};

	class WrappedD3D11Buffer : public WrappedD3D11Resource<ID3D11Buffer>
	{
	public:
		WrappedD3D11Buffer(ID3D11Buffer* pReal, WrappedD3D11Device* pDevice);
		virtual void STDMETHODCALLTYPE GetDesc(D3D11_BUFFER_DESC *pDesc);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);
	};

	class WrappedD3D11Texture1D : public WrappedD3D11Resource<ID3D11Texture1D>
	{
	public:
		WrappedD3D11Texture1D(ID3D11Texture1D* pReal, WrappedD3D11Device* pDevice);
		virtual void STDMETHODCALLTYPE GetDesc(D3D11_TEXTURE1D_DESC *pDesc);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);
	};

	class WrappedD3D11Texture2D : public WrappedD3D11Resource<ID3D11Texture2D>
	{
	public:
		WrappedD3D11Texture2D(ID3D11Texture2D* pReal, WrappedD3D11Device* pDevice);
		virtual void STDMETHODCALLTYPE GetDesc(D3D11_TEXTURE2D_DESC *pDesc);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);
		virtual void CopyToDeviceForSwapChainBuffer(ID3D11Device* pNewDevice, ID3D11Texture2D* pNewBuffer);
	};

	class WrappedD3D11Texture3D : public WrappedD3D11Resource<ID3D11Texture3D>
	{
	public:
		WrappedD3D11Texture3D(ID3D11Texture3D* pReal, WrappedD3D11Device* pDevice);
		virtual void STDMETHODCALLTYPE GetDesc(D3D11_TEXTURE3D_DESC *pDesc);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);
	};
}

