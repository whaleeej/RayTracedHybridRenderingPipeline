#include "WrappedD3D11ClassLinkage.h"

namespace rdcboost
{

	WrappedD3D11ClassLinkage::WrappedD3D11ClassLinkage(
		ID3D11ClassLinkage* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11DeviceChild(pReal, pDevice)
	{

	}

	HRESULT STDMETHODCALLTYPE WrappedD3D11ClassLinkage::GetClassInstance(
		LPCSTR pClassInstanceName, UINT InstanceIndex, ID3D11ClassInstance **ppInstance)
	{
		HRESULT res = GetReal()->GetClassInstance(pClassInstanceName, InstanceIndex, ppInstance);
		if (ppInstance && *ppInstance)
		{
			ID3D11ClassInstance* pReal = *ppInstance;
			auto it = m_WrappedClassInstances.find(pReal);
			if (it == m_WrappedClassInstances.end())
			{
				SWrappedClassInstanceInfo info;
				info.isCreate = false;
				info.classInstanceName = pClassInstanceName;
				info.instanceIndex = InstanceIndex;
				auto pInstance = new WrappedD3D11ClassInstance(this, info, pReal, m_pWrappedDevice);
				m_WrappedClassInstances[pReal] = pInstance;

				*ppInstance = pInstance;
			}
			else
			{
				*ppInstance = it->second;
				(*ppInstance)->AddRef();
			}

			pReal->Release();
		}

		return res;
	}

	HRESULT STDMETHODCALLTYPE WrappedD3D11ClassLinkage::CreateClassInstance(
		LPCSTR pClassTypeName, UINT ConstantBufferOffset, UINT ConstantVectorOffset, 
		UINT TextureOffset, UINT SamplerOffset, ID3D11ClassInstance **ppInstance)
	{
		HRESULT res = GetReal()->CreateClassInstance(pClassTypeName, ConstantBufferOffset, 
													 ConstantVectorOffset, TextureOffset, 
													 SamplerOffset, ppInstance);
		if (ppInstance && *ppInstance)
		{
			ID3D11ClassInstance* pReal = *ppInstance;
			auto it = m_WrappedClassInstances.find(pReal);
			if (it == m_WrappedClassInstances.end())
			{
				SWrappedClassInstanceInfo info;
				info.isCreate = true;
				info.classTypeName = pClassTypeName;
				info.constantBufferOffset = ConstantBufferOffset;
				info.constantVectorOffset = ConstantVectorOffset;
				info.textureOffset = TextureOffset;
				info.samplerOffset = SamplerOffset;
				auto pInstance = new WrappedD3D11ClassInstance(this, info, pReal, m_pWrappedDevice);

				*ppInstance = pInstance;
			}
			else
			{
				*ppInstance = it->second;
				(*ppInstance)->AddRef();
			}

			pReal->Release();
		}

		return res;
	}

	void WrappedD3D11ClassLinkage::OnClassInstanceReleased(ID3D11ClassInstance* pInstance)
	{
		Assert(m_WrappedClassInstances.erase(pInstance) == 1);
	}

	void WrappedD3D11ClassLinkage::CopyInstancesToDevice(ID3D11Device* pNewDevice)
	{
		auto oldInstances = m_WrappedClassInstances;

		m_WrappedClassInstances.clear();
		for (auto it = oldInstances.begin(); it != oldInstances.end(); ++it)
		{
			it->second->SwitchToDevice(pNewDevice);
			m_WrappedClassInstances[it->second->GetReal()] = it->second;
		}
	}

	ID3D11DeviceChild* WrappedD3D11ClassLinkage::CopyToDevice(ID3D11Device* pNewDevice)
	{
		ID3D11ClassLinkage* pNewClassLinkage = NULL;
		pNewDevice->CreateClassLinkage(&pNewClassLinkage);
		return pNewClassLinkage;
	}

	WrappedD3D11ClassInstance::WrappedD3D11ClassInstance(
		WrappedD3D11ClassLinkage* pClassLinkage, 
		const SWrappedClassInstanceInfo& createInfo,
		ID3D11ClassInstance* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11DeviceChild(pReal, pDevice),
		m_pClassLinkage(pClassLinkage),
		m_CreateInfo(createInfo)
	{
		m_pClassLinkage->AddRef();
	}

	WrappedD3D11ClassInstance::~WrappedD3D11ClassInstance()
	{
		m_pClassLinkage->OnClassInstanceReleased(GetReal());
		m_pClassLinkage->Release();
	}

	ID3D11DeviceChild* WrappedD3D11ClassInstance::CopyToDevice(ID3D11Device* pNewDevice)
	{
		HRESULT res;
		ID3D11ClassInstance* pNewInstance = NULL;
		if (!m_CreateInfo.isCreate)
		{
			res = m_pClassLinkage->GetReal()->GetClassInstance(
					m_CreateInfo.classInstanceName.c_str(),
					m_CreateInfo.instanceIndex, 
					&pNewInstance);
		}
		else
		{
			res = m_pClassLinkage->GetReal()->CreateClassInstance(
					m_CreateInfo.classTypeName.c_str(), 
					m_CreateInfo.constantBufferOffset,
					m_CreateInfo.constantVectorOffset,
					m_CreateInfo.textureOffset,
					m_CreateInfo.samplerOffset,
					&pNewInstance);
		}
		
		if (FAILED(res))
		{
			LogError("WrappedD3D11ClassInstance::CopyToDevice failed %d", res);
		}

		return pNewInstance;
	}

}

