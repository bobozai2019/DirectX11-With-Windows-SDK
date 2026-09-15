cmake_minimum_required(VERSION 3.14)
# Also build this sample alone without importing unrelated Assimp demos.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(DynamicDrawWater LANGUAGES CXX)
    add_subdirectory("../../ImGui" "ImGui")
endif()
set(COMMON "${CMAKE_CURRENT_SOURCE_DIR}/../Common")
file(GLOB SHADERS CONFIGURE_DEPENDS Shaders/*.hlsl Shaders/*.hlsli)
set_source_files_properties(${SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)
add_executable(41_DynamicDraw_Water WIN32 Main.cpp GameApp.cpp GameApp.h WaterSimulation.cpp WaterSimulation.h ${SHADERS}
    "${COMMON}/d3dApp.cpp" "${COMMON}/CpuTimer.cpp" "${COMMON}/GpuTimer.cpp"
    "${COMMON}/DXTrace.cpp" "${COMMON}/ScreenGrab11.cpp" "${COMMON}/WICTextureLoader11.cpp")
target_include_directories(41_DynamicDraw_Water PRIVATE "${COMMON}")
target_compile_features(41_DynamicDraw_Water PRIVATE cxx_std_17)
target_compile_definitions(41_DynamicDraw_Water PRIVATE UNICODE _UNICODE)
target_compile_options(41_DynamicDraw_Water PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_link_libraries(41_DynamicDraw_Water PRIVATE ImGui d3d11 dxgi dxguid d3dcompiler winmm windowscodecs ole32)
set_target_properties(41_DynamicDraw_Water PROPERTIES FOLDER "Project 19-"
    OUTPUT_NAME "41 DynamicDraw Water" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>" VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:41_DynamicDraw_Water>")
add_custom_target(41_DynamicDraw_Water_Assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Shaders" "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/Shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Assets" "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/Assets")
set_target_properties(41_DynamicDraw_Water_Assets PROPERTIES FOLDER "Project 19-")
add_dependencies(41_DynamicDraw_Water 41_DynamicDraw_Water_Assets)
source_group("Shaders" FILES ${SHADERS})
