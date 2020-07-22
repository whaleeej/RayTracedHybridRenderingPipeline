#pragma once
#include <d3d11.h>

namespace rdcboost
{
	struct SDeviceCreateParams
	{
		IDXGIAdapter* pAdapter;
		D3D_DRIVER_TYPE DriverType;
		HMODULE Software;
		UINT Flags;
		D3D_FEATURE_LEVEL* pFeatureLevels;
		UINT FeatureLevels;
		UINT SDKVersion;
		DXGI_SWAP_CHAIN_DESC* SwapChainDesc;

		SDeviceCreateParams(IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software,
							UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels,
							UINT SDKVersion, const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc) :
							pAdapter(pAdapter), DriverType(DriverType), Software(Software),
							Flags(Flags), FeatureLevels(FeatureLevels), SDKVersion(SDKVersion),
							SwapChainDesc(NULL)
		{
			if (pSwapChainDesc)
				SwapChainDesc = new DXGI_SWAP_CHAIN_DESC(*pSwapChainDesc);

			if (pAdapter != NULL)
				pAdapter->AddRef();

			if (pFeatureLevels != NULL && FeatureLevels != 0)
			{
				this->pFeatureLevels = new D3D_FEATURE_LEVEL[FeatureLevels];
				memcpy(this->pFeatureLevels, pFeatureLevels,
					   sizeof(D3D_FEATURE_LEVEL) * FeatureLevels);
			}
			else
			{
				this->pFeatureLevels = NULL;
			}
		}

		SDeviceCreateParams(const SDeviceCreateParams& rhs) :
			pAdapter(rhs.pAdapter), DriverType(rhs.DriverType), Software(rhs.Software),
			Flags(rhs.Flags), FeatureLevels(rhs.FeatureLevels), SDKVersion(rhs.SDKVersion),
			SwapChainDesc(NULL)
		{
			if (rhs.SwapChainDesc)
				SwapChainDesc = new DXGI_SWAP_CHAIN_DESC(*rhs.SwapChainDesc);

			if (pAdapter != NULL)
				pAdapter->AddRef();

			if (rhs.pFeatureLevels != NULL && rhs.FeatureLevels != 0)
			{
				this->pFeatureLevels = new D3D_FEATURE_LEVEL[rhs.FeatureLevels];
				memcpy(this->pFeatureLevels, rhs.pFeatureLevels,
					   sizeof(D3D_FEATURE_LEVEL) * rhs.FeatureLevels);
			}
			else
			{
				this->pFeatureLevels = NULL;
			}
		}

		SDeviceCreateParams& operator=(const SDeviceCreateParams& rhs)
		{
			pAdapter = rhs.pAdapter;
			DriverType = rhs.DriverType;
			Software = rhs.Software;
			Flags = rhs.Flags;
			FeatureLevels = rhs.FeatureLevels;
			SDKVersion = rhs.SDKVersion;

			delete SwapChainDesc;
			if (rhs.SwapChainDesc)
				SwapChainDesc = new DXGI_SWAP_CHAIN_DESC(*rhs.SwapChainDesc);

			if (pAdapter != NULL)
				pAdapter->AddRef();

			if (rhs.pFeatureLevels != NULL && rhs.FeatureLevels != 0)
			{
				this->pFeatureLevels = new D3D_FEATURE_LEVEL[rhs.FeatureLevels];
				memcpy(this->pFeatureLevels, rhs.pFeatureLevels,
					   sizeof(D3D_FEATURE_LEVEL) * rhs.FeatureLevels);
			}
			else
			{
				this->pFeatureLevels = NULL;
			}

			return *this;
		}

		~SDeviceCreateParams()
		{
			if (pAdapter)
				pAdapter->Release();

			delete[] pFeatureLevels;
			delete SwapChainDesc;
		}
	};
}

