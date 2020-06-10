cmake_minimum_required( VERSION 3.12.1 ) # Latest version of CMake when this file was created.

file(GLOB_RECURSE HEADER_FILES false
    inc/*.h
)

file(GLOB_RECURSE SRC_FILES false
    src/*.cpp
)

file(GLOB_RECURSE VERTEX_SHADER_FILES false
    shaders/*_VS.hlsl
)

file(GLOB_RECURSE PIXEL_SHADER_FILES false
    shaders/*_PS.hlsl
)

file(GLOB_RECURSE COMPUTE_SHADER_FILES false
    shaders/*_CS.hlsl
)

set( RT_SHADER_FILES
    shaders/RayTracing.hlsl
)

set( SHADER_FILES
    ${VERTEX_SHADER_FILES}
    ${PIXEL_SHADER_FILES}
    ${COMPUTE_SHADER_FILES}
    ${RT_SHADER_FILES}
)

source_group( "Resources\\Shaders" FILES ${SHADER_FILES}  )

set_source_files_properties( ${SHADER_FILES} 
    PROPERTIES
        VS_SHADER_OBJECT_FILE_NAME "${CMAKE_BINARY_DIR}/Data/Shaders/HybridRenderingPipeline/%(Filename).cso"
        VS_SHADER_MODEL 5.1
        VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:ON>
        VS_SHADER_ENABLE_DEBUG $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ON>
)

set_source_files_properties( ${VERTEX_SHADER_FILES}
    PROPERTIES 
        VS_SHADER_TYPE Vertex
)

set_source_files_properties( ${PIXEL_SHADER_FILES}
    PROPERTIES 
        VS_SHADER_TYPE Pixel
)

set_source_files_properties( ${COMPUTE_SHADER_FILES}
    PROPERTIES 
        VS_SHADER_TYPE Compute
)

set_source_files_properties( ${RT_SHADER_FILES}
    PROPERTIES 
        HEADER_FILE_ONLY TRUE
)

add_compile_definitions(_DISABLE_EXTENDED_ALIGNED_STORAGE)
add_compile_definitions(_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING)

add_executable( HybridRenderingPipeline WIN32
    ${HEADER_FILES} 
    ${SRC_FILES}
    ${SHADER_FILES}
)

target_include_directories( HybridRenderingPipeline
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/assimp/include
)

target_link_directories(HybridRenderingPipeline
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/assimp
)

target_link_libraries( HybridRenderingPipeline
    DX12Lib
    D3DCompiler.lib
    Shlwapi.lib
    assimp-vc142-mt.lib
)

# Set debug database to enable the "Edit and Continue" feature.
target_compile_options( HybridRenderingPipeline
    PRIVATE $<$<CONFIG:Debug>:/ZI>
)

# Set Local Debugger Settings (Command Arguments and Environment Variables)
set( COMMAND_ARGUMENTS "-wd \"${CMAKE_SOURCE_DIR}\"" )
configure_file( HybridRenderingPipeline.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/HybridRenderingPipeline.vcxproj.user @ONLY )
