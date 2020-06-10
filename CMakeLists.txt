cmake_minimum_required( VERSION 3.12.0 )

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_compile_options(/MP)

# Make sure DLL and EXE targets go to the same directory.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # Output directory for static lib (.LIB)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Output directory for shared lib (.DLL)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Output directory for executables (.EXE)

project( DirectX12-Demo LANGUAGES CXX )

add_subdirectory( DirectXTex )
add_subdirectory( DX12Lib )
add_subdirectory( HybridRenderingPipeline )

# Set the startup project.
set_directory_properties( PROPERTIES 
    VS_STARTUP_PROJECT HybridRenderingPipeline
)
