#pragma once

#include <RdcBoostPCH.h>

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12Device;

class WrappedD3D12ObjectBase {
public:
	WrappedD3D12ObjectBase(ID3D12Object* pReal): m_pReal(pReal){}
	virtual ~WrappedD3D12ObjectBase();

public: //virtual
	virtual void switchToDevice(ID3D12Device* pNewDevice) = 0;
	
public: //func
	ID3D12Object* getRealObject() { return m_pReal; }

protected:
	ID3D12Object* m_pReal;
};

class WrappedD3D12Object: public WrappedD3D12ObjectBase, public  {

};


RDCBOOST_NAMESPACE_END