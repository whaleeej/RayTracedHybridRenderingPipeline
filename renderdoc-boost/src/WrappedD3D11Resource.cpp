#include "WrappedD3D11Resource.h"
#include "Log.h"

namespace rdcboost
{

	template <typename TDesc, typename TRes, typename Traits>
	inline static TRes* CreateResourceWithInitialData(
		TRes* pRealRes, const TDesc& myDesc, 
		ID3D11Device* pOldDevice, ID3D11DeviceContext* pOldContext,
		ID3D11Device* pNewDevice, const Traits& pCopyTraits)
	{
		TDesc stageDesc = myDesc;
		stageDesc.BindFlags = 0;
		stageDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		stageDesc.MiscFlags = 0;
		stageDesc.Usage = D3D11_USAGE_STAGING;

		D3D11_SUBRESOURCE_DATA* pInitialData = NULL;
		UINT subresCount = pCopyTraits.GetSubresCount(&myDesc);
		TRes* pStageRes = NULL;
		if (SUCCEEDED(pCopyTraits.CreateResource(pOldDevice, &stageDesc, NULL, &pStageRes)))
		{
			pInitialData = new D3D11_SUBRESOURCE_DATA[subresCount];
			pOldContext->CopyResource(pStageRes, pRealRes);

			for (UINT subresIdx = 0; subresIdx < subresCount; ++subresIdx)
			{
				D3D11_MAPPED_SUBRESOURCE mappedRes;
				if (SUCCEEDED(pOldContext->Map(pStageRes, subresIdx, D3D11_MAP_READ, 0, &mappedRes)))
				{
					pInitialData[subresIdx].pSysMem = mappedRes.pData;
					pInitialData[subresIdx].SysMemPitch = mappedRes.RowPitch;
					pInitialData[subresIdx].SysMemSlicePitch = mappedRes.DepthPitch;
				}
				else
				{
					pInitialData[subresIdx].pSysMem = NULL;
					LogError("Map subresource of stage resource failed when CopyToDevice");
				}
			}
		}
		else
		{
			LogError("Create stage resource failed when CopyToDevice");
		}


		TRes* pNewRes = NULL;
		if (FAILED(pCopyTraits.CreateResource(pNewDevice, &myDesc, pInitialData, &pNewRes)))
		{
			LogError("Create resource failed on new device when CopyToDevice");
		}

		if (pInitialData != NULL)
		{
			delete[] pInitialData;
			for (UINT subresIdx = 0; subresIdx < subresCount; ++subresIdx)
				pOldContext->Unmap(pStageRes, subresIdx);

			pStageRes->Release();
		}

		return pNewRes;
	}

	template <typename TDesc, typename TRes, typename Traits>
	inline static TRes* CopyResource(
		TRes* pRealRes, ID3D11Device* pOldDevice, ID3D11Device* pNewDevice,
		const Traits& pCopyTraits)
	{
		TDesc myDesc;
		pRealRes->GetDesc(&myDesc);

		ID3D11DeviceContext* pOldContext = NULL, *pNewContext = NULL;
		pOldDevice->GetImmediateContext(&pOldContext);
		pNewDevice->GetImmediateContext(&pNewContext);

		TRes* pNewRes = NULL;
		if (pCopyTraits.GetSampleCount(&myDesc) == 1)
		{
			pNewRes = CreateResourceWithInitialData(pRealRes, myDesc, 
													pOldDevice, pOldContext, 
													pNewDevice, pCopyTraits);
		}
		else
		{
			LogError("Multisampled texture data transfer is not supported by now."
					 "The content of the new texture will be black.");


			if (FAILED(pCopyTraits.CreateResource(pNewDevice, &myDesc, NULL, &pNewRes)))
			{
				LogError("Create multisampled resource failed on new device when CopyToDevice");
			}
		}
		
		if (pNewRes != NULL)
		{
			FLOAT MinLOD = pOldContext->GetResourceMinLOD(pRealRes);
			pNewContext->SetResourceMinLOD(pNewRes, MinLOD);
		}

		pOldContext->Release();
		pNewContext->Release();
		return pNewRes;
	}

	WrappedD3D11Buffer::WrappedD3D11Buffer(ID3D11Buffer* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11Resource(pReal, pDevice)
	{

	}

	void STDMETHODCALLTYPE WrappedD3D11Buffer::GetDesc(D3D11_BUFFER_DESC *pDesc)
	{
		GetReal()->GetDesc(pDesc);
	}

	ID3D11DeviceChild* WrappedD3D11Buffer::CopyToDevice(ID3D11Device* pNewDevice)
	{
		struct CopyTraitsBuffer
		{
			UINT GetSubresCount(const D3D11_BUFFER_DESC* desc) const
			{
				return 1;
			}

			UINT GetSampleCount(const D3D11_BUFFER_DESC* desc) const
			{
				return 1;
			}

			HRESULT CreateResource(ID3D11Device* pDevice, const D3D11_BUFFER_DESC *pDesc,
								  const D3D11_SUBRESOURCE_DATA *pInitialData,
								  ID3D11Buffer **ppBuffer) const
			{
				return pDevice->CreateBuffer(pDesc, pInitialData, ppBuffer);
			}
		};

		return CopyResource<D3D11_BUFFER_DESC>(
			GetReal(), GetRealDevice(), pNewDevice, CopyTraitsBuffer());
	}

	WrappedD3D11Texture1D::WrappedD3D11Texture1D(ID3D11Texture1D* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11Resource(pReal, pDevice)
	{
	}

	void STDMETHODCALLTYPE WrappedD3D11Texture1D::GetDesc(D3D11_TEXTURE1D_DESC *pDesc)
	{
		GetReal()->GetDesc(pDesc);
	}

	ID3D11DeviceChild* WrappedD3D11Texture1D::CopyToDevice(ID3D11Device* pNewDevice)
	{
		struct CopyTraits1D
		{
			UINT GetSubresCount(const D3D11_TEXTURE1D_DESC* desc) const
			{
				return desc->ArraySize * desc->MipLevels;
			}

			UINT GetSampleCount(const D3D11_TEXTURE1D_DESC* desc) const
			{
				return 1;
			}

			HRESULT CreateResource(ID3D11Device* pDevice, const D3D11_TEXTURE1D_DESC *pDesc, 
								  const D3D11_SUBRESOURCE_DATA *pInitialData, 
								  ID3D11Texture1D **ppTexture1D) const
			{
				return pDevice->CreateTexture1D(pDesc, pInitialData, ppTexture1D);
			}
		};

		return CopyResource<D3D11_TEXTURE1D_DESC>(
				GetReal(), GetRealDevice(), pNewDevice, CopyTraits1D());
	}

	WrappedD3D11Texture2D::WrappedD3D11Texture2D(ID3D11Texture2D* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11Resource(pReal, pDevice)
	{

	}

	void STDMETHODCALLTYPE WrappedD3D11Texture2D::GetDesc(D3D11_TEXTURE2D_DESC *pDesc)
	{
		GetReal()->GetDesc(pDesc);
	}

	ID3D11DeviceChild* WrappedD3D11Texture2D::CopyToDevice(ID3D11Device* pNewDevice)
	{
		struct CopyTraits2D
		{
			UINT GetSubresCount(const D3D11_TEXTURE2D_DESC* desc) const
			{
				return desc->ArraySize * desc->MipLevels;
			}

			UINT GetSampleCount(const D3D11_TEXTURE2D_DESC* desc) const
			{
				return desc->SampleDesc.Count;
			}

			HRESULT CreateResource(ID3D11Device* pDevice, const D3D11_TEXTURE2D_DESC *pDesc, 
								  const D3D11_SUBRESOURCE_DATA *pInitialData, 
								  ID3D11Texture2D **ppTexture2D) const
			{
				return pDevice->CreateTexture2D(pDesc, pInitialData, ppTexture2D);
			}
		};

		return CopyResource<D3D11_TEXTURE2D_DESC>(
				GetReal(), GetRealDevice(), pNewDevice, CopyTraits2D());
	}

	void WrappedD3D11Texture2D::CopyToDeviceForSwapChainBuffer(ID3D11Device* pNewDevice, 
															   ID3D11Texture2D* pNewBuffer)
	{
		struct CopyTraits2D
		{
			UINT GetSubresCount(const D3D11_TEXTURE2D_DESC* desc) const
			{
				return desc->ArraySize * desc->MipLevels;
			}

			UINT GetSampleCount(const D3D11_TEXTURE2D_DESC* desc) const
			{
				return desc->SampleDesc.Count;
			}

			HRESULT CreateResource(ID3D11Device* pDevice, const D3D11_TEXTURE2D_DESC *pDesc,
								   const D3D11_SUBRESOURCE_DATA *pInitialData,
								   ID3D11Texture2D **ppTexture2D) const
			{
				if (pDevice != pNewDevice) // hard-fix, means create stage resource.
				{
					Assert(pDesc->Usage == D3D11_USAGE_STAGING);
					return pDevice->CreateTexture2D(pDesc, pInitialData, ppTexture2D);
				}

				// hard-fix, means create new swapchain buffer on new device.
				Assert(pNewBuffer != NULL);
				if (pInitialData != NULL)
				{
					Assert(pDesc->SampleDesc.Count == 1);
					D3D11_TEXTURE2D_DESC tmpTexDesc = *pDesc;
					tmpTexDesc.Usage = D3D11_USAGE_IMMUTABLE;
					tmpTexDesc.BindFlags = 0;
					tmpTexDesc.CPUAccessFlags = 0;
					tmpTexDesc.MiscFlags = 0;
					ID3D11Texture2D* pTmpTexture2D = NULL;
					if (SUCCEEDED(pDevice->CreateTexture2D(pDesc, pInitialData, &pTmpTexture2D)))
					{
						ID3D11DeviceContext* pContext = NULL;
						pDevice->GetImmediateContext(&pContext);
						pContext->CopyResource(pNewBuffer, pTmpTexture2D);
						pTmpTexture2D->Release();
						pContext->Release();
					}
					else
					{
						LogError("Create temp texture failed, new swapchain buffer will be black.");
					}
				}

				*ppTexture2D = pNewBuffer;
				pNewBuffer = NULL; // reference obtained by ppTexture2D.
				return S_OK;
			}

			ID3D11Device* pNewDevice;
			mutable ID3D11Texture2D* pNewBuffer;
		};

		CopyResource<D3D11_TEXTURE2D_DESC>(
			GetReal(), GetRealDevice(), pNewDevice, CopyTraits2D{ pNewDevice, pNewBuffer });
	}

	WrappedD3D11Texture3D::WrappedD3D11Texture3D(ID3D11Texture3D* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11Resource(pReal, pDevice)
	{

	}

	void STDMETHODCALLTYPE WrappedD3D11Texture3D::GetDesc(D3D11_TEXTURE3D_DESC *pDesc)
	{
		GetReal()->GetDesc(pDesc);
	}

	ID3D11DeviceChild* WrappedD3D11Texture3D::CopyToDevice(ID3D11Device* pNewDevice)
	{
		struct CopyTraits3D
		{
			UINT GetSubresCount(const D3D11_TEXTURE3D_DESC* desc) const
			{
				return desc->MipLevels;
			}

			UINT GetSampleCount(const D3D11_TEXTURE3D_DESC* desc) const
			{
				return 1;
			}

			HRESULT CreateResource(ID3D11Device* pDevice, const D3D11_TEXTURE3D_DESC *pDesc,
								  const D3D11_SUBRESOURCE_DATA *pInitialData, 
								  ID3D11Texture3D **ppTexture3D) const
			{
				return pDevice->CreateTexture3D(pDesc, pInitialData, ppTexture3D);
			}
		};

		return CopyResource<D3D11_TEXTURE3D_DESC>(
				GetReal(), GetRealDevice(), pNewDevice, CopyTraits3D());
	}

}

