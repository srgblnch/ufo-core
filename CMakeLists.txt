cmake_minimum_required(VERSION 2.6)
project(ufo C)

set(TARNAME "libufo")

set(UFO_DESCRIPTION         "UFO high-speed image processing core library")
set(UFO_DESCRIPTION_SUMMARY "UFO high-speed image processing core library")

#{{{ Library version
set(UFO_VERSION_MAJOR "0")
set(UFO_VERSION_MINOR "8")
set(UFO_VERSION_PATCH "0")
set(UFO_GIR_VERSION "${UFO_VERSION_MAJOR}.0")
#}}}
#{{{ Package version
set(PACKAGE_VERSION_MAJOR ${UFO_VERSION_MAJOR})
set(PACKAGE_VERSION_MINOR ${UFO_VERSION_MINOR})
set(PACKAGE_VERSION_PATCH ${UFO_VERSION_PATCH})
set(PACKAGE_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}")
set(PACKAGE_NAME ${TARNAME})
set(PACKAGE_TARNAME ${TARNAME})
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "http://ufo.kit.edu/ufo/newticket")
#}}}
#{{{ CMake
enable_testing()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/common/cmake")

# Add configure-like prefixes and directories
include(ConfigurePaths)
include(PkgConfigVars)
configure_paths(UFO)

set(UFO_INCLUDEDIR  "${UFO_INCLUDEDIR}/ufo-${UFO_VERSION_MAJOR}")
set(UFO_KERNELDIR   "${UFO_DATADIR}/ufo")
set(UFO_PLUGINDIR   "${UFO_LIBDIR}/ufo")
#}}}
#{{{ Options

option(WITH_TESTS "Build test suite" ON)
option(WITH_DEPRECATED_OPENCL_1_1_API "Build with deprecated OpenCL 1.1 API" ON)

if (WITH_DEPRECATED_OPENCL_1_1_API)
    add_definitions ("-DCL_USE_DEPRECATED_OPENCL_1_1_APIS")
endif ()

#}}}
#{{{ Dependencies
set(PKG_GLIB2_MIN_REQUIRED "2.30")
set(PKG_JSON_GLIB_MIN_REQUIRED "0.10.0")
set(PKG_ZMQ_MIN_REQUIRED "2.1")

find_package(OpenCL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(MPI)

pkg_check_modules(GLIB2 glib-2.0>=${PKG_GLIB2_MIN_REQUIRED} REQUIRED)
pkg_check_modules(GOBJECT2 gobject-2.0>=${PKG_GLIB2_MIN_REQUIRED} REQUIRED)
pkg_check_modules(GMODULE2 gmodule-2.0>=${PKG_GLIB2_MIN_REQUIRED} REQUIRED)
pkg_check_modules(GTHREAD2 gthread-2.0>=${PKG_GLIB2_MIN_REQUIRED} REQUIRED)
pkg_check_modules(GIO2 gio-2.0>=${PKG_GLIB2_MIN_REQUIRED} REQUIRED)
pkg_check_modules(JSON_GLIB json-glib-1.0>=${PKG_JSON_GLIB_MIN_REQUIRED} REQUIRED)
pkg_check_modules(ZMQ libzmq>=${PKG_ZMQ_MIN_REQUIRED})
pkg_check_modules(PYTHON python)

set(UFOCORE_DEPS
    m
    ${OPENCL_LIBRARIES}
    ${GLIB2_LIBRARIES}
    ${GOBJECT2_LIBRARIES}
    ${GMODULE2_LIBRARIES}
    ${GTHREAD2_LIBRARIES}
    ${GIO2_LIBRARIES}
    ${JSON_GLIB_LIBRARIES})

if(PYTHON_FOUND)
    option(WITH_PYTHON "Enable Python multithreading support" ON)

    if (WITH_PYTHON)
        include_directories(${PYTHON_INCLUDE_DIRS})
        list(APPEND UFOCORE_DEPS ${PYTHON_LIBRARIES})
    endif ()
endif()

if (MPI_C_FOUND)
    option(WITH_MPI "Build with MPI support" OFF)

    if (WITH_MPI)
        set(CMAKE_C_COMPILER ${MPI_C_COMPILER})
        include_directories(${MPI_C_INCLUDE_PATH})
        list(APPEND UFOCORE_DEPS ${MPI_C_LIBRARIES})
    endif ()
endif ()

if (ZMQ_FOUND)
    option(WITH_ZMQ "Build with ZMQ support" ON)

    if (WITH_ZMQ)
        link_directories(${ZMQ_LIBRARY_DIRS})
        include_directories(${ZMQ_INCLUDE_DIRS})
        list(APPEND UFOCORE_DEPS ${ZMQ_LIBRARIES})
    endif ()
endif ()

#{{{ Link dirs of dependencies
link_directories(
    ${GLIB2_LIBRARY_DIRS}
    ${JSON_GLIB_LIBRARY_DIRS})
#}}}
#{{{ Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GLIB2_INCLUDE_DIRS}
    ${GIO2_INCLUDE_DIRS}
    ${OPENCL_INCLUDE_DIRS}
    ${JSON_GLIB_INCLUDE_DIRS})
#}}}
#}}}
#{{{ Definitions
add_definitions("-std=c99 -pedantic -Wall -Wextra -fPIC")

if (CMAKE_COMPILER_IS_GNUCC OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"))
    add_definitions("-Wcast-align
                     -Wcast-qual
                     -Winline
                     -Wmissing-declarations
                     -Wmissing-prototypes
                     -Wnested-externs
                     -Wno-deprecated-declarations
                     -Wno-long-long
                     -Wno-missing-field-initializers
                     -Wno-unused-parameter
                     -Wpointer-arith
                     -Wredundant-decls
                     -Wshadow
                     -Wstrict-prototypes
                     -Wwrite-strings")
endif()

add_definitions(-DG_LOG_DOMAIN=\"Ufo\")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
#}}}
#{{{ Subdirectories
add_subdirectory(ufo)
add_subdirectory(docs)
add_subdirectory(bin)

if (WITH_TESTS)
    add_subdirectory(tests)
endif()
#}}}
#{{{ CPack
set(CPACK_PACKAGE_DESCRIPTION ${UFO_DESCRIPTION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${UFO_DESCRIPTION_SUMMARY})
set(CPACK_PACKAGE_NAME ${TARNAME})

set(CPACK_PACKAGE_CONTACT "matthias.vogelgesang@kit.edu")
set(CPACK_PACKAGE_VENDOR "Karlsruhe Institute of Technology/IPE")
set(CPACK_PACKAGE_VERSION_MAJOR ${PACKAGE_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PACKAGE_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PACKAGE_VERSION_PATCH})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PACKAGE_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
set(VERSION ${PACKAGE_VERSION})

set(CPACK_GENERATOR "DEB;RPM;")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "/build/;.git/;tags;.swp;${CPACK_SOURCE_IGNORE_FILES}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PACKAGE_NAME}-${PACKAGE_VERSION}" CACHE INTERNAL "tarball basename")

#{{{ Debian
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libglib2.0-0 (>= ${PKG_GLIB2_MIN_REQUIRED}),
     libjson-glib-1.0-0 (>= ${PKG_JSON_GLIB_MIN_REQUIRED}),
     libzmq1 (>= ${PKG_ZMQ_MIN_REQUIRED})")
#}}}
#{{{ RPM
set(CPACK_RPM_PACKAGE_AUTOREQPROV " no")
set(CPACK_RPM_PACKAGE_REQUIRES "libjson-glib-1_0-0, libzmq1")
#}}}

include(CPack)
#}}}
