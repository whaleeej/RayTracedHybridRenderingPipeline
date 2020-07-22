#include "WrappedD3D11Async.h"
#include "Log.h"

namespace rdcboost
{

	WrappedD3D11Counter::WrappedD3D11Counter(ID3D11Counter* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11Async(pReal, pDevice)
	{
	}

	void STDMETHODCALLTYPE WrappedD3D11Counter::GetDesc(D3D11_COUNTER_DESC *pDesc)
	{
		GetReal()->GetDesc(pDesc);
	}

	ID3D11DeviceChild* WrappedD3D11Counter::CopyToDevice(ID3D11Device* pNewDevice)
	{
		// TODO_wzq 

		D3D11_COUNTER_DESC desc;
		GetReal()->GetDesc(&desc);

		ID3D11Counter* pNewCounter = NULL;
		if (FAILED(pNewDevice->CreateCounter(&desc, &pNewCounter)))
			LogError("CreateCounter failed when CopyToDevice");

		if (pNewCounter != NULL && m_bBeginIssued)
		{
			ID3D11DeviceContext* pImmediateContext = NULL;
			pNewDevice->GetImmediateContext(&pImmediateContext);

			if (pImmediateContext != NULL)
				pImmediateContext->Begin(pNewCounter);
		}

		return pNewCounter;
	}

}

