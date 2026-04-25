cmake_minimum_required(VERSION 3.16)
project(cam-engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets)

set(CMAKE_PREFIX_PATH "/usr/local" ${CMAKE_PREFIX_PATH})
find_package(OpenCASCADE QUIET)

if(NOT OpenCASCADE_FOUND)
    if(WIN32)
        if(DEFINED ENV{OCCT_DIR})
            set(OpenCASCADE_INCLUDE_DIR "$ENV{OCCT_DIR}/inc")
            set(OpenCASCADE_LIBRARY_DIR "$ENV{OCCT_DIR}/win64/vc14/lib")
        else()
            set(OpenCASCADE_INCLUDE_DIR "C:/OpenCASCADE/inc")
            set(OpenCASCADE_LIBRARY_DIR "C:/OpenCASCADE/win64/vc14/lib")
        endif()
    else()
        set(OpenCASCADE_INCLUDE_DIR "/usr/local/include/opencascade")
        set(OpenCASCADE_LIBRARY_DIR "/usr/local/lib")
    endif()
    include_directories(${OpenCASCADE_INCLUDE_DIR})
    link_directories(${OpenCASCADE_LIBRARY_DIR})
endif()

find_package(OpenGL REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${Qt6Core_INCLUDE_DIRS}
    ${Qt6Widgets_INCLUDE_DIRS}
    ${Qt6OpenGL_INCLUDE_DIRS}
)

set(SOURCES
    src/main.cpp
    src/core/machine/MachineProfile.cpp
    src/core/postprocessor/PostProcessorRegistry.cpp
    src/core/postprocessor/GRBLPostProcessor.cpp
    src/core/postprocessor/LinuxCNCPostProcessor.cpp
    src/core/postprocessor/FanucPostProcessor.cpp
    src/core/postprocessor/Mach4PostProcessor.cpp
    src/core/cam/CamOperation.cpp
    src/core/cam/operations/FacingOp.cpp
    src/core/cam/operations/ContourOp.cpp
    src/core/cam/operations/PocketOp.cpp
    src/core/cam/operations/DrillingOp.cpp
    src/core/importer/CadImporter.cpp
    src/ui/viewport/Viewport3D.cpp
    src/ui/mainwindow/MainWindow.cpp
)

set(OCCT_LIBS
    TKBin TKBinL TKLCAF TKCAF TKCDF
    TKFeat TKOffset TKFillet TKBO TKBool
    TKMesh TKShHealing TKPrim
    TKTopAlgo TKGeomAlgo TKBRep
    TKGeomBase TKG3d TKG2d TKMath TKernel
)

if(WIN32)
    add_executable(cam-engine WIN32 ${SOURCES})
else()
    add_executable(cam-engine ${SOURCES})
endif()

target_link_libraries(cam-engine
    ${OCCT_LIBS}
    Qt6::Core Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets
    ${OPENGL_LIBRARIES}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(cam-engine GLU pthread dl)
endif()
if(WIN32)
    target_link_libraries(cam-engine opengl32 glu32)
endif()

target_include_directories(cam-engine PRIVATE
    ${OpenCASCADE_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

install(TARGETS cam-engine DESTINATION bin)

message(STATUS "========================================")
message(STATUS "CAM-ENGINE v${PROJECT_VERSION}")
message(STATUS "========================================")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Qt6:      ${Qt6_VERSION}")
message(STATUS "OCCT inc: ${OpenCASCADE_INCLUDE_DIR}")
message(STATUS "========================================")
