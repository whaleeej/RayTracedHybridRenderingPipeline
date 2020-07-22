#include "WrappedD3D11View.h"
#include "Log.h"

namespace rdcboost
{

	WrappedD3D11DepthStencilView::WrappedD3D11DepthStencilView(
		ID3D11Resource* pWrappedResource, ID3D11DepthStencilView* pReal, 
		WrappedD3D11Device* pDevice) :
		WrappedD3D11View(pWrappedResource, pReal, pDevice)
	{
	}

	void STDMETHODCALLTYPE WrappedD3D11DepthStencilView::GetDesc(D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc)
	{
		GetReal()->GetDesc(pDesc);
	}

	ID3D11DeviceChild* WrappedD3D11DepthStencilView::CopyToDevice(ID3D11Device* pNewDevice)
	{
		auto pWrappedResource = ConvertToWrappedBase(m_pWrappedResource);
		pWrappedResource->SwitchToDevice(pNewDevice);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsViewDesc;
		GetReal()->GetDesc(&dsViewDesc);

		ID3D11DepthStencilView* pNewView = NULL;
		HRESULT ret = pNewDevice->CreateDepthStencilView(
						static_cast<ID3D11Resource*>(pWrappedResource->GetRealDeviceChild()), 
						&dsViewDesc, &pNewView);

		if (FAILED(ret))
			LogError("CreateDepthStencilView failed when CopyToDevice");

		return pNewView;
	}

	WrappedD3D11RenderTargetView::WrappedD3D11RenderTargetView(
		ID3D11Resource* pWrappedResource, ID3D11RenderTargetView* pReal, 
		WrappedD3D11Device* pDevice) : 
		WrappedD3D11View(pWrappedResource, pReal, pDevice)
	{
	}

	void STDMETHODCALLTYPE WrappedD3D11RenderTargetView::GetDesc(D3D11_RENDER_TARGET_VIEW_DESC *pDesc)
	{
		GetReal()->GetDesc(pDesc);
	}

	ID3D11DeviceChild* WrappedD3D11RenderTargetView::CopyToDevice(ID3D11Device* pNewDevice)
	{
		auto pWrappedResource = ConvertToWrappedBase(m_pWrappedResource);
		pWrappedResource->SwitchToDevice(pNewDevice);

		D3D11_RENDER_TARGET_VIEW_DESC rtViewDesc;
		GetReal()->GetDesc(&rtViewDesc);

		ID3D11RenderTargetView* pNewView;
		HRESULT ret = pNewDevice->CreateRenderTargetView(
						static_cast<ID3D11Resource*>(pWrappedResource->GetRealDeviceChild()), 
						&rtViewDesc, 
						&pNewView);

		if (FAILED(ret))
			LogError("CreateRenderTargetView failed when CopyToDevice");

		return pNewView;
	}

	WrappedD3D11ShaderResourceView::WrappedD3D11ShaderResourceView(
		ID3D11Resource* pWrappedResource, ID3D11ShaderResourceView* pReal, 
		WrappedD3D11Device* pDevice) : 
		WrappedD3D11View(pWrappedResource, pReal, pDevice)
	{
	}

	void STDMETHODCALLTYPE WrappedD3D11ShaderResourceView::GetDesc(D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc)
	{
		GetReal()->GetDesc(pDesc);
	}

	ID3D11DeviceChild* WrappedD3D11ShaderResourceView::CopyToDevice(ID3D11Device* pNewDevice)
	{
		auto pWrappedResource = ConvertToWrappedBase(m_pWrappedResource);
		pWrappedResource->SwitchToDevice(pNewDevice);

		D3D11_SHADER_RESOURCE_VIEW_DESC srViewDesc;
		GetReal()->GetDesc(&srViewDesc);

		ID3D11ShaderResourceView* pNewView;
		HRESULT ret = pNewDevice->CreateShaderResourceView(
						static_cast<ID3D11Resource*>(pWrappedResource->GetRealDeviceChild()), 
						&srViewDesc, &pNewView);

		if (FAILED(ret))
			LogError("CreateShaderResourceView failed when CopyToDevice");

		return pNewView;
	}

	WrappedD3D11UnorderedAccessView::WrappedD3D11UnorderedAccessView(
		ID3D11Resource* pWrappedResource, ID3D11UnorderedAccessView* pReal, 
		WrappedD3D11Device* pDevice) : 
		WrappedD3D11View(pWrappedResource, pReal, pDevice)
	{
	}

	void STDMETHODCALLTYPE WrappedD3D11UnorderedAccessView::GetDesc(D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc)
	{
		GetReal()->GetDesc(pDesc);
	}

	ID3D11DeviceChild* WrappedD3D11UnorderedAccessView::CopyToDevice(ID3D11Device* pNewDevice)
	{
		auto pWrappedResource = ConvertToWrappedBase(m_pWrappedResource);
		pWrappedResource->SwitchToDevice(pNewDevice);

		D3D11_UNORDERED_ACCESS_VIEW_DESC uaViewDesc;
		GetReal()->GetDesc(&uaViewDesc);

		ID3D11UnorderedAccessView* pNewView;
		HRESULT ret = pNewDevice->CreateUnorderedAccessView(
						static_cast<ID3D11Resource*>(pWrappedResource->GetRealDeviceChild()), 
						&uaViewDesc, &pNewView);

		if (FAILED(ret))
			LogError("CreateUnorderedAccessView failed when CopyToDevice");

		return pNewView;
	}

}

