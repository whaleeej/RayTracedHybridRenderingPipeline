#pragma once

/**
 *  @file Buffer.h
 *  @date October 22, 2018
 *  @author Jeremiah van Oosten
 *
 *  @brief Abstract base class for buffer resources.
 */

#include "Resource.h"

class Buffer : public Resource
{
public:
    Buffer(const std::wstring& name = L"");
    Buffer( const D3D12_RESOURCE_DESC& resDesc,
        size_t numElements, size_t elementSize,
        const std::wstring& name = L"" );
    
    /**
     * Create the views for the buffer resource.
     * Used by the CommandList when setting the buffer contents.
     */
    virtual void CreateViews(size_t numElements, size_t elementSize) = 0;

protected:

private:

};