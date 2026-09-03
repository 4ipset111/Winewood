cmake_minimum_required(VERSION 3.16)
project(Winewood VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(QC_BUILD_EXAMPLES OFF CACHE BOOL "Build QuarkCore examples" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
add_subdirectory(QuarkCore EXCLUDE_FROM_ALL)

if(NOT TARGET QuarkCore::QuarkCore)
	add_library(QuarkCore::QuarkCore ALIAS QuarkCore)
endif()

add_executable(winewood
	src/main.cpp
	src/resources.cpp
)

target_link_libraries(winewood PRIVATE
	QuarkCore::QuarkCore
	QuarkCore::OpenGL
)

target_include_directories(winewood PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(WIN32)
	set(WINEWOOD_RUNTIME_DLLS
		$<TARGET_FILE:QuarkCore::OpenGL>
		$<IF:$<CONFIG:Debug>,${CMAKE_CURRENT_SOURCE_DIR}/QuarkCore/external/SDL3/lib/debug/SDL3.dll,${CMAKE_CURRENT_SOURCE_DIR}/QuarkCore/external/SDL3/lib/release/SDL3.dll>
		$<IF:$<CONFIG:Debug>,${CMAKE_CURRENT_SOURCE_DIR}/QuarkCore/external/assimp/lib/debug/assimp-vc143-mtd.dll,${CMAKE_CURRENT_SOURCE_DIR}/QuarkCore/external/assimp/lib/release/assimp-vc143-mt.dll>
		$<IF:$<CONFIG:Debug>,${CMAKE_CURRENT_SOURCE_DIR}/QuarkCore/external/freetype/lib/debug/freetyped.dll,${CMAKE_CURRENT_SOURCE_DIR}/QuarkCore/external/freetype/lib/release/freetype.dll>
		$<IF:$<CONFIG:Debug>,${CMAKE_CURRENT_SOURCE_DIR}/QuarkCore/external/OpenAL/lib/debug/OpenAL32.dll,${CMAKE_CURRENT_SOURCE_DIR}/QuarkCore/external/OpenAL/lib/release/OpenAL32.dll>
	)

	add_custom_command(TARGET winewood POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${WINEWOOD_RUNTIME_DLLS}
			$<TARGET_FILE_DIR:winewood>
		COMMENT "Copying QuarkCore and dependency DLLs next to winewood..."
	)
endif()
