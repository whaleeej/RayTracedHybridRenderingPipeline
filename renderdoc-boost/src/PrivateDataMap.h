#pragma once
#include <map>
#include <d3d11.h>
#include "Log.h"

namespace rdcboost
{
	class PrivateDataMap
	{
	public:
		struct SPrivateData
		{
			UINT m_DataSize;
			void* m_pData;
			
			SPrivateData() : m_DataSize(0), m_pData(NULL)
			{
			}

			SPrivateData(UINT DataSize, const void* pData);

			~SPrivateData()
			{
				delete[] m_pData;
			}

			SPrivateData(const SPrivateData& rhs);

			SPrivateData& operator=(const SPrivateData& rhs);
		};

		~PrivateDataMap()
		{
			for (auto it = m_PrivateInterfaces.begin(); it != m_PrivateInterfaces.end(); ++it)
				const_cast<IUnknown*>(it->second)->Release();
		}

		void SetPrivateData(REFGUID guid, UINT DataSize, const void *pData);

		void SetPrivateDataInterface(REFGUID guid, const IUnknown *pData);

		template <typename T>
		void CopyPrivateData(T* pObject)
		{
			for (auto it = m_PrivateDatas.begin(); it != m_PrivateDatas.end(); ++it)
			{
				pObject->SetPrivateData(it->first, it->second.m_DataSize, it->second.m_pData);
			}

			for (auto it = m_PrivateInterfaces.begin(); it != m_PrivateInterfaces.end(); ++it)
			{
				pObject->SetPrivateDataInterface(it->first, it->second);
			}
		}

	private:
		template <typename T>
		class less_memcmp
		{
		public:
			bool operator()(const T& lhs, const T& rhs) const
			{
				return memcmp(&lhs, &rhs, sizeof(T)) < 0;
			}
		};
		std::map<GUID, SPrivateData, less_memcmp<GUID> > m_PrivateDatas;
		std::map<GUID, const IUnknown*, less_memcmp<GUID> > m_PrivateInterfaces;
	};
}

