#pragma once

#include <RdcBoostPCH.h>

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12Device;

class WrappedD3D12ObjectBase { //interface for m_pReal and switchToDevice
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

template<typename NestedType>
class WrappedD3D12Object: public WrappedD3D12ObjectBase, public NestedType{ // account for wrapped layer for object, also the base of all d3d12Class
public:
	WrappedD3D12Object(ID3D12Object* pReal) : WrappedD3D12ObjectBase(pReal), m_Ref(1)();
	~WrappedD3D12ObjectBase();

protected:
	unsigned int m_Ref;
	PrivateDataMap m_PrivateData;
};


RDCBOOST_NAMESPACE_END