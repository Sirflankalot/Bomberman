cmake_minimum_required(VERSION 3.1.0)
project(Bomberman)
if(${WIN32})
	add_compile_options(/std:c++latest)
else(${WIN32})
	add_compile_options(-std=c++1z -Wall -Wextra -Wpedantic)
endif(${WIN32})
include_directories(include)
link_directories(${PROJECT_BINARY_DIR})

file(GLOB_RECURSE SOURCES_BOMBERMAN "src/*.cpp")
file(GLOB_RECURSE HEADERS_BOMBERMAN "src/*.hpp")
file(GLOB_RECURSE UI_BOMBERMAN "src/*.ui")

# set(Boost_USE_STATIC_LIBS       OFF) # only find static libs
# set(Boost_USE_MULTITHREADED      ON)
# set(Boost_USE_STATIC_RUNTIME    OFF)
# find_package(Boost 1.36.0 COMPONENTS filesystem system)
# find_package(Qt5Widgets)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# qt5_wrap_cpp(moc_sources ${HEADERS_QTTEST})
# qt5_wrap_ui(uic_sources ${UI_QTTEST})

# if(Boost_FOUND)
# 	include_directories(${Boost_INCLUDE_DIRS})
# endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    link_libraries(${GLEW_LIBRARIES})
endif()

find_package(SDL2 REQUIRED)
if (SDL2_FOUND)
	include_directories(${SDL2_INCLUDE_DIR})
	link_libraries(${SDL2_LIBRARY})
endif()

find_package(OpenGL REQUIRED)
if (OPENGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIR})
	link_libraries(${OPENGL_LIBRARIES})
endif()

include_directories(.)
add_executable(Bomberman ${SOURCES_BOMBERMAN})

add_custom_command(TARGET Bomberman POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:Bomberman>/shaders)

add_custom_command(TARGET Bomberman POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/objects $<TARGET_FILE_DIR:Bomberman>/objects)

if(${WIN32})
	add_custom_command(TARGET Bomberman POST_BUILD
	                   COMMAND ${CMAKE_COMMAND} -E copy_directory
	                       ${CMAKE_SOURCE_DIR}/dependencies $<TARGET_FILE_DIR:Bomberman>)

	file( WRITE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.vcxproj.user" 
	    "<?xml version=\"1.0\" encoding=\"utf-8\"?>     \
	    <Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">
	    <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">
	        <LocalDebuggerWorkingDirectory>$(OutDir)</LocalDebuggerWorkingDirectory>
	        <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
	    </PropertyGroup>
	    <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='MinSizeRel|x64'\">
	        <LocalDebuggerWorkingDirectory>$(OutDir)</LocalDebuggerWorkingDirectory>
	        <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
	    </PropertyGroup>
	    <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">
	        <LocalDebuggerWorkingDirectory>$(OutDir)</LocalDebuggerWorkingDirectory>
	        <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
	    </PropertyGroup>
	    <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='RelWithDebInfo|x64'\">
	        <LocalDebuggerWorkingDirectory>$(OutDir)</LocalDebuggerWorkingDirectory>
	        <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
	    </PropertyGroup>
	    </Project>")
endif(${WIN32})

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES $<TARGET_FILE_DIR:Bomberman>/shaders)

# if(Boost_FOUND) 
# 	target_link_libraries(qttest ${Boost_LIBRARIES})
# endif()

# target_link_libraries (qttest Qt5::Widgets)
