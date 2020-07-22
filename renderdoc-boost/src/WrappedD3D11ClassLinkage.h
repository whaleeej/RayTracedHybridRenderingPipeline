#pragma once
#include "WrappedD3D11DeviceChild.h"

namespace rdcboost
{
	struct SWrappedClassInstanceInfo
	{
		bool		isCreate;
		std::string classInstanceName;
		UINT		instanceIndex;
		std::string	classTypeName;
		UINT		constantBufferOffset;
		UINT		constantVectorOffset;
		UINT		textureOffset;
		UINT		samplerOffset;
	};

	class WrappedD3D11ClassInstance;
	class WrappedD3D11ClassLinkage : public WrappedD3D11DeviceChild<ID3D11ClassLinkage>
	{
	public:
		WrappedD3D11ClassLinkage(ID3D11ClassLinkage* pReal, WrappedD3D11Device* pDevice);

		virtual HRESULT STDMETHODCALLTYPE GetClassInstance(
			/* [annotation] */
			_In_  LPCSTR pClassInstanceName,
			/* [annotation] */
			_In_  UINT InstanceIndex,
			/* [annotation] */
			_Out_  ID3D11ClassInstance **ppInstance);

		virtual HRESULT STDMETHODCALLTYPE CreateClassInstance(
			/* [annotation] */
			_In_  LPCSTR pClassTypeName,
			/* [annotation] */
			_In_  UINT ConstantBufferOffset,
			/* [annotation] */
			_In_  UINT ConstantVectorOffset,
			/* [annotation] */
			_In_  UINT TextureOffset,
			/* [annotation] */
			_In_  UINT SamplerOffset,
			/* [annotation] */
			_Out_  ID3D11ClassInstance **ppInstance);

		virtual void OnClassInstanceReleased(ID3D11ClassInstance* pInstance);

		virtual void CopyInstancesToDevice(ID3D11Device* pNewDevice);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);

	private:
		std::map<ID3D11ClassInstance*, WrappedD3D11ClassInstance*> m_WrappedClassInstances;
	};

	class WrappedD3D11ClassInstance : public WrappedD3D11DeviceChild<ID3D11ClassInstance>
	{
	public:
		WrappedD3D11ClassInstance(WrappedD3D11ClassLinkage* pClassLinkage, 
								  const SWrappedClassInstanceInfo& createInfo,
								  ID3D11ClassInstance* pReal, WrappedD3D11Device* pDevice);

		virtual ~WrappedD3D11ClassInstance();

		virtual void STDMETHODCALLTYPE GetClassLinkage(
			/* [annotation] */
			_Out_  ID3D11ClassLinkage **ppLinkage)
		{
			*ppLinkage = m_pClassLinkage;
		}

		virtual void STDMETHODCALLTYPE GetDesc(
			/* [annotation] */
			_Out_  D3D11_CLASS_INSTANCE_DESC *pDesc)
		{
			GetReal()->GetDesc(pDesc);
		}

		virtual void STDMETHODCALLTYPE GetInstanceName(
			/* [annotation] */
			_Out_writes_opt_(*pBufferLength)  LPSTR pInstanceName,
			/* [annotation] */
			_Inout_  SIZE_T *pBufferLength)
		{
			GetReal()->GetInstanceName(pInstanceName, pBufferLength);
		}

		virtual void STDMETHODCALLTYPE GetTypeName(
			/* [annotation] */
			_Out_writes_opt_(*pBufferLength)  LPSTR pTypeName,
			/* [annotation] */
			_Inout_  SIZE_T *pBufferLength)
		{
			GetReal()->GetTypeName(pTypeName, pBufferLength);
		}

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);

	private:
		SWrappedClassInstanceInfo m_CreateInfo;
		WrappedD3D11ClassLinkage* m_pClassLinkage;
	};
}

