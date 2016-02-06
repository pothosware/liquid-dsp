########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8.12)
project(liquid-dsp C)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

#local include precedence
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}) #prefer generated config.h
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#local helper find scripts
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

########################################################################
# Extract version
########################################################################
set(LIQUID_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/parser)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/liquid.h" liquid_h)
string(REGEX MATCH "\\#define LIQUID_VERSION          \"([0-9]+\\.[0-9]+\\.[0-9]+)\"" LIQUID_VERSION_MATCHES "${liquid_h}")
if(NOT LIQUID_VERSION_MATCHES)
    message(FATAL_ERROR "Failed to extract version number from liquid.h")
endif(NOT LIQUID_VERSION_MATCHES)
set(LIQUID_VERSION ${CMAKE_MATCH_1})

########################################################################
# Compiler specific flags
########################################################################
if(CMAKE_COMPILER_IS_GNUCXX)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")

endif(CMAKE_COMPILER_IS_GNUCXX)

if (APPLE)
endif(APPLE)

if(MSVC)
    add_compile_options(/MP) #multi-core build
endif(MSVC)

########################################################################
# FFTW3f dependency
########################################################################
find_package(FFTW3f)

message(STATUS "FFTW3F_INCLUDE_DIRS: ${FFTW3F_INCLUDE_DIRS}")
message(STATUS "FFTW3F_LIBRARIES: ${FFTW3F_LIBRARIES}")

set(HAVE_FFTW3_H ${FFTW3f_FOUND}) #config.h

if (FFTW3f_FOUND)
    include_directories(${FFTW3F_INCLUDE_DIRS})
    list(APPEND LIQUID_LIBS ${FFTW3F_LIBRARIES})
endif()

########################################################################
# lib math dependency
########################################################################
find_library(
    M_LIBRARY NAMES m
    PATHS /usr/lib /usr/lib64
)

if(M_LIBRARY)
    list(APPEND LIQUID_LIBS ${M_LIBRARY})
endif(M_LIBRARY)

########################################################################
# Architecture configuration
########################################################################
include(CheckTypeSize)
CHECK_TYPE_SIZE("int" SIZEOF_INT BUILTIN_TYPES_ONLY LANGUAGE C)
CHECK_TYPE_SIZE("unsigned int" SIZEOF_UNSIGNED_INT BUILTIN_TYPES_ONLY LANGUAGE C)

include(CheckIncludeFiles)
CHECK_INCLUDE_FILES("mmintrin.h" HAVE_MMINTRIN_H)
CHECK_INCLUDE_FILES("xmmintrin.h" HAVE_XMMINTRIN_H)
CHECK_INCLUDE_FILES("emmintrin.h" HAVE_EMMINTRIN_H)
CHECK_INCLUDE_FILES("pmmintrin.h" HAVE_PMMINTRIN_H)
CHECK_INCLUDE_FILES("smmintrin.h" HAVE_SMMINTRIN_H)

include(CheckCCompilerFlag)
macro(CHECK_C_COMPILER_FLAG_ADD flag variable)
    CHECK_C_COMPILER_FLAG(${flag} ${variable})
    if (${variable})
        add_compile_options(${flag})
    endif()
endmacro(CHECK_C_COMPILER_FLAG_ADD)

CHECK_C_COMPILER_FLAG_ADD("-mmmx" HAVE_MMX)
CHECK_C_COMPILER_FLAG_ADD("-msse" HAVE_SSE)
CHECK_C_COMPILER_FLAG_ADD("-msse2" HAVE_SSE2)
CHECK_C_COMPILER_FLAG_ADD("-msse3" HAVE_SSE3)
CHECK_C_COMPILER_FLAG_ADD("-mssse3" HAVE_SSSE3)
CHECK_C_COMPILER_FLAG_ADD("-msse4.1" HAVE_SSE41)
CHECK_C_COMPILER_FLAG_ADD("-msse4.2" HAVE_SSE42)
CHECK_C_COMPILER_FLAG_ADD("-mavx" HAVE_AVX)

#TODO detect ARCH_x86, ARCH_PPC, ARCH_NEON
set(ARCH_x86 TRUE)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.in.h
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

########################################################################
# dotproduct libs
########################################################################

#default sources are portable C versions
set(MLIBS_DOTPROD
    src/dotprod/src/dotprod_cccf.c
    src/dotprod/src/dotprod_crcf.c
    src/dotprod/src/dotprod_rrrf.c
    src/dotprod/src/sumsq.c
)

#replace with SIMD specific sources list
if (ARCH_x86 AND HAVE_SSE41 AND HAVE_SMMINTRIN_H)
    set(MLIBS_DOTPROD
        src/dotprod/src/dotprod_cccf.mmx.c
        src/dotprod/src/dotprod_crcf.mmx.c
        src/dotprod/src/dotprod_rrrf.mmx.c
        src/dotprod/src/sumsq.mmx.c
    )
elseif (ARCH_x86 AND HAVE_SSE2 AND HAVE_EMMINTRIN_H)
    set(MLIBS_DOTPROD
        src/dotprod/src/dotprod_cccf.mmx.c
        src/dotprod/src/dotprod_crcf.mmx.c
        src/dotprod/src/dotprod_rrrf.mmx.c
        src/dotprod/src/sumsq.mmx.c
    )
elseif (ARCH_PPC)
    set(MLIBS_DOTPROD
        src/dotprod/src/dotprod_cccf.c
        src/dotprod/src/dotprod_crcf.av.c
        src/dotprod/src/dotprod_rrrf.av.c
        src/dotprod/src/sumsq.c
    )
    add_compile_options(-fno-common -faltivec)
elseif (ARCH_NEON)
    set(MLIBS_DOTPROD
        src/dotprod/src/dotprod_cccf.neon.c
        src/dotprod/src/dotprod_crcf.neon.c
        src/dotprod/src/dotprod_rrrf.neon.c
        src/dotprod/src/sumsq.c
    )
    add_compile_options(-ffast-math -mcpu=cortex-a8 -mfloat-abi=softfp -mfpu=neon)
endif ()

########################################################################
# vector libs
########################################################################

# for now all vector operations are portable C versions
set(MLIBS_VECTOR
    src/vector/src/vectorf_add.port.c
    src/vector/src/vectorf_norm.port.c
    src/vector/src/vectorf_mul.port.c
    src/vector/src/vectorf_trig.port.c
    src/vector/src/vectorcf_add.port.c
    src/vector/src/vectorcf_norm.port.c
    src/vector/src/vectorcf_mul.port.c
    src/vector/src/vectorcf_trig.port.c
)

########################################################################
# import target sources
########################################################################
find_package(PythonInterp)

set(TARGET_SOURCES_CMAKE ${CMAKE_CURRENT_BINARY_DIR}/TargetSources.cmake)

#generate the lists of target sources using python script
execute_process(COMMAND ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmakeimport.py
    ${TARGET_SOURCES_CMAKE}
)

#include sources lists into this project
include(${TARGET_SOURCES_CMAKE})

########################################################################
# Build library
# Defaults to static, set BUILD_SHARED_LIBS=ON for shared
########################################################################
option(BUILD_SHARED_LIBS "enable shared library build" OFF)
add_library(liquid ${objects})
target_link_libraries(liquid ${LIQUID_LIBS})
set_target_properties(liquid PROPERTIES VERSION ${LIQUID_VERSION})
set_property(TARGET liquid PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(liquid PROPERTIES SOVERSION ${LIQUID_VERSION})
set_target_properties(liquid PROPERTIES VERSION ${LIQUID_VERSION})

install(TARGETS liquid
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

########################################################################
# Build unit test
########################################################################
option(BUILD_TESTING "enable autotest unit-testing" ON)
if (BUILD_TESTING)

    enable_testing()

    #autoscript helper to generate autotest_include.h
    add_executable(autoscript scripts/autoscript.c scripts/main.c)

    #generate autotest_include.h using autoscript
    #in the source directory because of relative include
    file(GLOB include_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
    add_custom_command(
        DEPENDS ${autotest_sources} ${include_headers}
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/autotest_include.h
        COMMAND autoscript ${PATHSEP} autotest ${autotest_sources}
            > ${CMAKE_CURRENT_SOURCE_DIR}/autotest_include.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    #create autotest library
    add_library(autotestlib STATIC
        autotest/autotestlib.c
        ${autotest_sources}
        ${autotest_extra_obj}
    )
    target_link_libraries(autotestlib liquid ${LIQUID_LIBS})
    set_property(TARGET autotestlib PROPERTY POSITION_INDEPENDENT_CODE TRUE)

    #create xautotest unit-test
    add_executable(xautotest autotest/autotest.c ${CMAKE_CURRENT_SOURCE_DIR}/autotest_include.h)
    target_link_libraries(xautotest autotestlib liquid ${LIQUID_LIBS})

    add_test(autotest xautotest)
endif ()

########################################################################
# Install headers
########################################################################
install(
    FILES include/liquid.h
    DESTINATION include
)

########################################################################
# Print summary
########################################################################
message(STATUS "Building liquid-dsp v${LIQUID_VERSION}")
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
