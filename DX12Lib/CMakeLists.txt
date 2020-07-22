cmake_minimum_required( VERSION 3.12.1 )

#### format project catalogue
file(GLOB SOURCE_Buildin false inc/Buildin/*.h src/Buildin/*.cpp)
file(GLOB SOURCE_DXRuntime false inc/DXRuntime/*.h src/DXRuntime/*.cpp)
file(GLOB SOURCE_Framework false inc/Framework/*.h src/Framework/*.cpp)
file(GLOB SOURCE_GRAbstract false inc/GRAbstract/*.h src/GRAbstract/*.cpp)
file(GLOB SOURCE_DXResource false inc/DXResource/*.h src/DXResource/*.cpp)
file(GLOB SOURCE_Utility false inc/Utility/*.h src/Utility/*.cpp )
file(GLOB SOURCE_Dep false inc/Dep/*.h src/Dep/*.cpp )

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
source_group( "Dep" FILES ${SOURCE_Dep} )
source_group( "DXRuntime" FILES ${SOURCE_DXRuntime} )
source_group( "Framework" FILES ${SOURCE_Framework} )
source_group( "GRAbstract" FILES ${SOURCE_GRAbstract} )
source_group( "DXResource" FILES ${SOURCE_DXResource} )
source_group( "Utility" FILES ${SOURCE_Utility} )
source_group( "Resources\\Shaders" FILES ${SHADER_FILES} )
source_group( "Resources" FILES DX12Lib.rc resource.h )
source_group( "Resources\\Icon" FILES Resources/Icon/app_icon.ico )

### build project
file(GLOB_RECURSE HEADER_FILES false inc/*.h)
file(GLOB_RECURSE SOURCE_FILES false src/*.cpp)

add_compile_definitions(_DISABLE_EXTENDED_ALIGNED_STORAGE)
add_compile_definitions(_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING)

add_library( DX12Lib STATIC
    ${HEADER_FILES}
    ${SOURCE_FILES}
    ${RESOURCE_FILES}
)

target_include_directories( DX12Lib
    PUBLIC inc/Buildin
    PUBLIC inc/Dep
    PUBLIC inc/DXRuntime
    PUBLIC inc/Framework
    PUBLIC inc/GRAbstract
    PUBLIC inc/DXResource
    PUBLIC inc/Utility
    PUBLIC ../renderdoc-boost/include
    PRIVATE "${CMAKE_BINARY_DIR}/Data/Shaders/DX12Lib"
)

target_link_libraries( DX12Lib 
    PUBLIC DirectXTex
    PUBLIC renderdoc-boost
    PUBLIC d3d12.lib
    PUBLIC dxgi.lib
    PUBLIC dxguid.lib
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/DX12Lib.dir/${CMAKE_CFG_INTDIR}/DX12Lib.res # This is the only way I could figure out how to link the compiled resource file.
)

###set properties
set_source_files_properties( ${SHADER_FILES}
    PROPERTIES
        VS_SHADER_MODEL 5.1
		VS_SHADER_OBJECT_FILE_NAME "${CMAKE_BINARY_DIR}/Data/Shaders/DX12Lib/%(Filename).cso"
        VS_SHADER_OUTPUT_HEADER_FILE "${CMAKE_BINARY_DIR}/Data/Shaders/DX12Lib/%(Filename).h"
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
