cmake_minimum_required( VERSION 3.12.1 ) # Latest version of CMake when this file was created.

file(GLOB SOURCE_Buildin false inc/Buildin/*.h src/Buildin/*.cpp)
file(GLOB SOURCE_DXObject false inc/DXObject/*.h src/DXObject/*.cpp)
file(GLOB SOURCE_Framework false inc/Framework/*.h src/Framework/*.cpp)
file(GLOB SOURCE_Geometry false inc/Geometry/*.h src/Geometry/*.cpp)
file(GLOB SOURCE_Resource false inc/Resource/*.h src/Resource/*.cpp)
file(GLOB SOURCE_Utility false inc/Utility/*.h src/Utility/*.cpp)

set( SHADER_FILES
    Resources/Shaders/GenerateMips_CS.hlsl
    Resources/Shaders/PanoToCubemap_CS.hlsl
)

set( RESOURCE_FILES 
    DX12Lib.rc
    Resources/Icon/app_icon.ico
	${SHADER_FILES}
)

source_group( "Buildin" FILES ${SOURCE_Buildin} )
source_group( "DXObject" FILES ${SOURCE_DXObject} )
source_group( "Framework" FILES ${SOURCE_Framework} )
source_group( "Geometry" FILES ${SOURCE_Geometry} )
source_group( "Resource" FILES ${SOURCE_Resource} )
source_group( "Utility" FILES ${SOURCE_Utility} )
source_group( "Resources\\Shaders" FILES ${SHADER_FILES} )
source_group( "Resources" FILES DX12Lib.rc resource.h )
source_group( "Resources\\Icon" FILES Resources/Icon/app_icon.ico )

file(GLOB_RECURSE HEADER_FILES false inc/*.h)
file(GLOB_RECURSE SOURCE_FILES false src/*.cpp)

add_library( DX12Lib STATIC
    ${HEADER_FILES}
    ${SOURCE_FILES}
    ${RESOURCE_FILES}
)

target_include_directories( DX12Lib
    PUBLIC inc/Buildin
    public inc/DXObject
    public inc/Framework
    public inc/Geometry
    public inc/Resource
    public inc/Utility
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/$(Platform)/$(Configuration)/Shaders"
)

target_link_libraries( DX12Lib 
	PUBLIC DirectXTex
    PUBLIC d3d12.lib
    PUBLIC dxgi.lib
    PUBLIC dxguid.lib
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/DX12Lib.dir/${CMAKE_CFG_INTDIR}/DX12Lib.res # This is the only way I could figure out how to link the compiled resource file.
)

# # Enable precompiled header files.
# set_source_files_properties( ${SOURCE_FILES} 
#     PROPERTIES 
#         COMPILE_FLAGS /Yu"DX12LibPCH.h"
# )

# set_source_files_properties( src/DX12LibPCH.cpp
#     PROPERTIES
#         COMPILE_FLAGS /Yc"DX12LibPCH.h"
# )

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
