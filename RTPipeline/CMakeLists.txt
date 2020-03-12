cmake_minimum_required( VERSION 3.12.1 ) # Latest version of CMake when this file was created.

set( HEADER_FILES
    inc/Camera.h
    inc/GameObject.h
    inc/Light.h
    inc/Material.h
    inc/RTPipeline.h
)

set( SRC_FILES
    src/Camera.cpp
    src/main.cpp
    src/Light.cpp
    src/Material.cpp
    src/RTPipeline.cpp
)

set( VERTEX_SHADER_FILES 
    shaders/Deferred_VS.hlsl
    shaders/PostProcessing_VS.hlsl
)

set( PIXEL_SHADER_FILES
    shaders/Deferred_PS.hlsl
    shaders/PostATrous_PS.hlsl
    shaders/PostTemporal_PS.hlsl
    shaders/PostLighting_PS.hlsl
)

set( RT_SHADER_FILES
    shaders/RayTracing.hlsl
)

set( SHADER_FILES
    ${VERTEX_SHADER_FILES}
    ${PIXEL_SHADER_FILES}
    ${RT_SHADER_FILES}
)

source_group( "Resources\\Shaders" FILES ${SHADER_FILES}  )

set_source_files_properties( ${SHADER_FILES} 
    PROPERTIES
        VS_SHADER_OBJECT_FILE_NAME "${CMAKE_BINARY_DIR}/Data/Shaders/RTPipeline/%(Filename).cso"
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

set_source_files_properties( ${RT_SHADER_FILES}
    PROPERTIES 
        HEADER_FILE_ONLY TRUE
)

add_compile_definitions(_DISABLE_EXTENDED_ALIGNED_STORAGE)
add_compile_definitions(_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING)

add_executable( RTPipeline WIN32
    ${HEADER_FILES} 
    ${SRC_FILES}
    ${SHADER_FILES}
)

target_include_directories( RTPipeline
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries( RTPipeline
    DX12Lib
    D3DCompiler.lib
    Shlwapi.lib
)

# Set debug database to enable the "Edit and Continue" feature.
target_compile_options( RTPipeline
    PRIVATE $<$<CONFIG:Debug>:/ZI>
)

# Set Local Debugger Settings (Command Arguments and Environment Variables)
set( COMMAND_ARGUMENTS "-wd \"${CMAKE_SOURCE_DIR}\"" )
configure_file( RTPipeline.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/RTPipeline.vcxproj.user @ONLY )
