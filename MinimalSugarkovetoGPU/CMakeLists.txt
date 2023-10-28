cmake_minimum_required(VERSION 3.11)


project(MinimalSugarkovetoGPU
        VERSION 0.0.0
        LANGUAGES CXX
        DESCRIPTION "kecske")

set(SRC_FILES ./src/framework.cpp ./src/Skeleton.cpp)
set(HEADER_FILES ./src/framework.h)

option(I_LIKE_PAIN "Enable pedantic build" OFF)
option(CLANG_TOOLING "Enable compile commands" OFF)

if (${CLANG_TOOLING})
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ./cmake)

set(SRC_FOLDER ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_FOLDER ${CMAKE_SOURCE_DIR}/include)

file(MAKE_DIRECTORY ${SRC_FOLDER})
file(MAKE_DIRECTORY ${INCLUDE_FOLDER})
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake)

file(DOWNLOAD http://cg.iit.bme.hu/~szirmay/grafika/framework.cpp ${SRC_FOLDER}/framework.cpp)
file(DOWNLOAD http://cg.iit.bme.hu/~szirmay/grafika/framework.h ${SRC_FOLDER}/framework.h)
file(DOWNLOAD http://cg.iit.bme.hu/~szirmay/grafika/Skeleton.cpp ${SRC_FOLDER}/Skeleton.cpp)

set_source_files_properties(${SRC_FOLDER}/Skeleton.cpp ${SRC_FOLDER}/framework.cpp
        ${SRC_FOLDER}/framework.h PROPERTIES GENERATED TRUE)

add_executable(program)
target_sources(program PRIVATE ${SRC_FILES} ${HEADER_FILES})
target_include_directories(program PRIVATE ${INCLUDE_FOLDER})
set_property(TARGET program PROPERTY CXX_STANDARD 14)

if (${I_LIKE_PAIN})
    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -Werror -pedantic -Wshadow -g")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if (UNIX)
    target_link_libraries(program PRIVATE GL glut GLU GLEW X11 m)
endif()

if (WIN32)
    include(ExternalProject)

    ExternalProject_Add(glut
            URL https://www.transmissionzero.co.uk/files/software/development/GLUT/freeglut-MinGW.zip
            SOURCE_DIR ${SRC_FOLDER}/freeglut
            BUILD_COMMAND ""
            CONFIGURE_COMMAND ""
            INSTALL_COMMAND ""
    )

    ExternalProject_Add(glew
            URL https://downloads.sourceforge.net/project/glew/glew/2.1.0/glew-2.1.0-win32.zip
            SOURCE_DIR ${SRC_FOLDER}/glew
            BUILD_COMMAND ""
            CONFIGURE_COMMAND ""
            INSTALL_COMMAND ""
    )

    add_dependencies(program glut glew)

    target_include_directories(program PRIVATE ${SRC_FOLDER}/freeglut/include ${SRC_FOLDER}/glew/include)
    if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
        target_link_directories(program PRIVATE ${SRC_FOLDER}/freeglut/lib/x64 ${SRC_FOLDER}/glew/lib/Release/x64)
        add_custom_command(TARGET program COMMAND ${CMAKE_COMMAND} -E copy ${SRC_FOLDER}/freeglut/bin/x64/freeglut.dll ${CMAKE_BINARY_DIR}/freeglut.dll)
        add_custom_command(TARGET program COMMAND ${CMAKE_COMMAND} -E copy ${SRC_FOLDER}/glew/bin/Release/x64/glew32.dll ${CMAKE_BINARY_DIR}/glew32.dll)
    else()
        target_link_directories(program PRIVATE ${SRC_FOLDER}/freeglut/lib ${SRC_FOLDER}/glew/lib/Release/Win32)
        add_custom_command(TARGET program COMMAND ${CMAKE_COMMAND} -E copy ${SRC_FOLDER}/freeglut/bin/freeglut.dll ${CMAKE_BINARY_DIR}/freeglut.dll)
        add_custom_command(TARGET program COMMAND ${CMAKE_COMMAND} -E copy ${SRC_FOLDER}/glew/bin/Release/Win32/glew32.dll ${CMAKE_BINARY_DIR}/glew32.dll)
    endif()
    target_link_libraries(program PRIVATE glew32 glu32 freeglut opengl32)
endif()