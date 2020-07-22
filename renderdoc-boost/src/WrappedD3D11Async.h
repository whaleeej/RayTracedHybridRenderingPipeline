#pragma once
#include "WrappedD3D11DeviceChild.h"

namespace rdcboost
{
	template <typename NestedType>
	class WrappedD3D11Async : public WrappedD3D11DeviceChild<NestedType>
	{
	public:
		WrappedD3D11Async(NestedType* pReal, WrappedD3D11Device* pDevice) : 
			WrappedD3D11DeviceChild(pReal, pDevice), m_bBeginIssued(false)
		{
		}

		virtual UINT STDMETHODCALLTYPE GetDataSize(void)
		{
			return GetReal()->GetDataSize();
		}

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
		{
			if (riid == __uuidof(ID3D11Asynchronous))
			{
				*ppvObject = static_cast<ID3D11Asynchronous*>(this);
				AddRef();
				return S_OK;
			}

			return WrappedD3D11DeviceChild::QueryInterface(riid, ppvObject);
		}

		void Begin()
		{
			m_bBeginIssued = true;
		}

		void End()
		{
			m_bBeginIssued = false;
		}


	protected:
		bool m_bBeginIssued;
	};

	template <typename NestedType>
	class WrappedD3D11QueryBase : public WrappedD3D11Async<NestedType>
	{
	public:
		WrappedD3D11QueryBase(NestedType* pReal, WrappedD3D11Device* pDevice) :
			WrappedD3D11Async(pReal, pDevice)
		{
		}

		virtual void STDMETHODCALLTYPE GetDesc(D3D11_QUERY_DESC *pDesc)
		{
			GetReal()->GetDesc(pDesc);
		}

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
		{
			if (riid == __uuidof(ID3D11Query))
			{
				*ppvObject = static_cast<ID3D11Query*>(this);
				AddRef();
				return S_OK;
			}

			return WrappedD3D11DeviceChild::QueryInterface(riid, ppvObject);
		}

	protected:
		template <typename T>
		struct Traits
		{
			static ID3D11DeviceChild* copyImp(ID3D11Device* pNewDevice, D3D11_QUERY_DESC* desc);
		};

		template <>
		struct Traits<ID3D11Query>
		{
			static ID3D11Asynchronous* copyImp(ID3D11Device* pNewDevice, D3D11_QUERY_DESC* desc)
			{
				ID3D11Query* pNewPredicate = NULL;
				if (FAILED(pNewDevice->CreateQuery(desc, &pNewPredicate)))
					LogError("CreateQuery failed when CopyToDevice");

				return pNewPredicate;
			}
		};

		template <>
		struct Traits<ID3D11Predicate>
		{
			static ID3D11Asynchronous* copyImp(ID3D11Device* pNewDevice, D3D11_QUERY_DESC* desc)
			{
				ID3D11Predicate* pNewPredicate = NULL;
				if (FAILED(pNewDevice->CreatePredicate(desc, &pNewPredicate)))
					LogError("CreateQuery failed when CopyToDevice");

				return pNewPredicate;
			}
		};

		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice)
		{
			// TODO_wzq 
			D3D11_QUERY_DESC desc;
			GetReal()->GetDesc(&desc);
			ID3D11Asynchronous* pNewAsync = Traits<NestedType>::copyImp(pNewDevice, &desc);
			if (m_bBeginIssued && pNewAsync != NULL)
			{
				ID3D11DeviceContext* pImmediateContext = NULL;
				pNewDevice->GetImmediateContext(&pImmediateContext);

				if (pImmediateContext != NULL)
					pImmediateContext->Begin(pNewAsync);
			}

			return pNewAsync;
		}
	};

	class WrappedD3D11Counter : public WrappedD3D11Async<ID3D11Counter>
	{
	public:
		WrappedD3D11Counter(ID3D11Counter* pReal, WrappedD3D11Device* pDevice);
		virtual void STDMETHODCALLTYPE GetDesc(D3D11_COUNTER_DESC *pDesc);

	protected:
		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice);
	};

	typedef WrappedD3D11QueryBase<ID3D11Query> WrappedD3D11Query;
	typedef WrappedD3D11QueryBase<ID3D11Predicate> WrappedD3D11Predicate;
}

