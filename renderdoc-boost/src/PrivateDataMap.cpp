#include "PrivateDataMap.h"

namespace rdcboost
{

	PrivateDataMap::SPrivateData::SPrivateData(UINT DataSize, const void* pData) : 
		m_DataSize(DataSize), m_pData(new char[DataSize])
	{
		memcpy(m_pData, pData, DataSize);
	}

	PrivateDataMap::SPrivateData::SPrivateData(const SPrivateData& rhs) : 
		m_DataSize(rhs.m_DataSize)
	{
		if (rhs.m_pData && rhs.m_DataSize)
		{
			m_pData = new char[rhs.m_DataSize];
			memcpy(m_pData, rhs.m_pData, rhs.m_DataSize);
		}
		else
		{
			m_pData = NULL;
		}
	}

	PrivateDataMap::SPrivateData& PrivateDataMap::SPrivateData::operator=(const SPrivateData& rhs)
	{
		delete[] m_pData;
		m_DataSize = rhs.m_DataSize;
		if (rhs.m_pData && rhs.m_DataSize)
		{
			m_pData = new char[m_DataSize];
			memcpy(m_pData, rhs.m_pData, m_DataSize);
		}
		else
		{
			m_pData = NULL;
		}

		return *this;
	}


	void PrivateDataMap::SetPrivateData(REFGUID guid, UINT DataSize, const void *pData)
	{
		if (DataSize == 0 || pData == NULL)
			m_PrivateDatas.erase(guid);
		else
			m_PrivateDatas[guid] = SPrivateData(DataSize, pData);
	}

	void PrivateDataMap::SetPrivateDataInterface(REFGUID guid, const IUnknown *pData)
	{
		auto it = m_PrivateInterfaces.find(guid);
		if (pData != NULL)
		{
			const_cast<IUnknown*>(pData)->AddRef();
			if (it != m_PrivateInterfaces.end())
			{
				const_cast<IUnknown*>(it->second)->Release();
				it->second = pData;
			}
			else
			{
				m_PrivateInterfaces.insert(std::make_pair(guid, pData));
			}
		}
		else
		{
			if (it != m_PrivateInterfaces.end())
			{
				const_cast<IUnknown*>(it->second)->Release();
				m_PrivateInterfaces.erase(it);
			}
		}
	}

}

