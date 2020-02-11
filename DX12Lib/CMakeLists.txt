cmake_minimum_required( VERSION 3.12.1 ) # Latest version of CMake when this file was created.

set( HEADER_FILES
    inc/Application.h
    inc/Buffer.h
    inc/ByteAddressBuffer.h
    inc/CommandList.h
    inc/CommandQueue.h
    inc/ConstantBuffer.h
    inc/d3dx12.h
    inc/Defines.h
    inc/DescriptorAllocation.h
    inc/DescriptorAllocator.h
    inc/DescriptorAllocatorPage.h
    inc/DX12LibPCH.h
    inc/DynamicDescriptorHeap.h
    inc/Events.h
    inc/Game.h
    inc/GenerateMipsPSO.h
    inc/Helpers.h
    inc/HighResolutionClock.h
    inc/IndexBuffer.h
    inc/KeyCodes.h
    inc/Mesh.h
    inc/PanoToCubemapPSO.h
    inc/RenderTarget.h
    inc/Resource.h
    inc/ResourceStateTracker.h
    inc/RootSignature.h
    inc/StructuredBuffer.h
    inc/Texture.h
    inc/TextureUsage.h
    inc/ThreadSafeQueue.h
    inc/UploadBuffer.h
    inc/VertexBuffer.h
    inc/Window.h
)

set( SOURCE_FILES
    src/DX12LibPCH.cpp
    src/Application.cpp
    src/Buffer.cpp
    src/ByteAddressBuffer.cpp
    src/CommandQueue.cpp
    src/CommandList.cpp
    src/ConstantBuffer.cpp
    src/DescriptorAllocation.cpp
    src/DescriptorAllocator.cpp
    src/DescriptorAllocatorPage.cpp
    src/DynamicDescriptorHeap.cpp
    src/Game.cpp
    src/GenerateMipsPSO.cpp
    src/HighResolutionClock.cpp
    src/IndexBuffer.cpp
    src/Mesh.cpp
    src/PanoToCubemapPSO.cpp
    src/RenderTarget.cpp
    src/Resource.cpp
    src/ResourceStateTracker.cpp
    src/RootSignature.cpp
    src/StructuredBuffer.cpp
    src/Texture.cpp
    src/UploadBuffer.cpp
    src/VertexBuffer.cpp
    src/Window.cpp
)

set( SHADER_FILES
    Resources/Shaders/GenerateMips_CS.hlsl
    Resources/Shaders/PanoToCubemap_CS.hlsl
)

set( RESOURCE_FILES 
    DX12Lib.rc
    Resources/Icon/app_icon.ico
	${SHADER_FILES}
)

source_group( "Resources" FILES DX12Lib.rc resource.h )
source_group( "Resources\\Icon" FILES Resources/Icon/app_icon.ico )
source_group( "Resources\\Shaders" FILES ${SHADER_FILES} )

add_library( DX12Lib STATIC
    ${HEADER_FILES}
    ${SOURCE_FILES}
    ${RESOURCE_FILES}
)

target_include_directories( DX12Lib
    PUBLIC inc
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/$(Platform)/$(Configuration)/Shaders"
)

target_link_libraries( DX12Lib 
	PUBLIC DirectXTex
    PUBLIC d3d12.lib
    PUBLIC dxgi.lib
    PUBLIC dxguid.lib
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/DX12Lib.dir/${CMAKE_CFG_INTDIR}/DX12Lib.res # This is the only way I could figure out how to link the compiled resource file.
)

# Enable precompiled header files.
set_source_files_properties( ${SOURCE_FILES} 
    PROPERTIES 
        COMPILE_FLAGS /Yu"DX12LibPCH.h"
)

set_source_files_properties( src/DX12LibPCH.cpp
    PROPERTIES
        COMPILE_FLAGS /Yc"DX12LibPCH.h"
)

set_source_files_properties( ${SHADER_FILES}
    PROPERTIES
        VS_SHADER_MODEL 5.1
        VS_SHADER_OUTPUT_HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/$(Platform)/$(Configuration)/Shaders/%(Filename).h"
        VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:ON>
        VS_SHADER_ENABLE_DEBUG $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ON>
)

set_source_files_properties( Resources/Shaders/GenerateMips_CS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Compute
        VS_SHADER_VARIABLE_NAME g_GenerateMips_CS
)

set_source_files_properties( Resources/Shaders/PanoToCubemap_CS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Compute
        VS_SHADER_VARIABLE_NAME g_PanoToCubemap_CS
)
