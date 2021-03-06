# Minimum version required
cmake_minimum_required (VERSION 3.10)

# Check if network is available
unset(BUILD_OFFLINE)
unset(BUILD_OFFLINE CACHE)
execute_process(COMMAND bash -c
                        "if ping -q -c 1 -W 1 8.8.8.8 ; then echo 0; else echo 1; fi"
                OUTPUT_STRIP_TRAILING_WHITESPACE
                OUTPUT_VARIABLE BUILD_OFFLINE)
if(${BUILD_OFFLINE})
    message("-- No internet connection. Not building external projects.")
endif()

# Set various flags
set(WARN_FULL "-Wall -Wextra -Weffc++ -pedantic -pedantic-errors \
               -Wcast-align -Wcast-qual -Wfloat-equal -Wformat=2 \
               -Wformat-nonliteral -Wformat-security -Wformat-y2k \
               -Wimport -Winit-self -Winvalid-pch -Wlong-long \
               -Wmissing-field-initializers -Wmissing-format-attribute \
               -Wmissing-noreturn -Wpacked -Wpointer-arith \
               -Wredundant-decls -Wshadow -Wstack-protector \
               -Wstrict-aliasing=2 -Wswitch-default -Wswitch-enum \
               -Wunreachable-code -Wunused -Wunused-parameter"
)
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wfatal-errors ${WARN_FULL}")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG ${WARN_FULL} -Wno-unused-parameter")

# Shared libraries need PIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Define GNU standard installation directories
include(GNUInstallDirs)

# Custom cmake module path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

# Build type
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

# Set Python version
unset(PYTHON_EXECUTABLE)
unset(PYTHON_EXECUTABLE CACHE)
find_program(PYTHON_EXECUTABLE python)
execute_process(COMMAND "${PYTHON_EXECUTABLE}" -c
                        "import sys; sys.stdout.write(';'.join([str(x) for x in sys.version_info[:3]]))"
                OUTPUT_VARIABLE _VERSION)
string(REPLACE ";" "." PYTHON_VERSION_STRING "${_VERSION}")
list(GET _VERSION 0 PYTHON_VERSION_MAJOR)
list(GET _VERSION 1 PYTHON_VERSION_MINOR)
set(PYTHON_VERSION ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
get_filename_component(PYTHON_ROOT ${PYTHON_EXECUTABLE} DIRECTORY)
get_filename_component(PYTHON_ROOT ${PYTHON_ROOT} DIRECTORY)
set(PYTHON_SITELIB ${PYTHON_ROOT}/lib/python${PYTHON_VERSION}/site-packages)
set(PYTHON_INSTALL_FLAGS "--upgrade --no-deps --force-reinstall ")

# Check permissions on Python site-package to determine whether to use user site
execute_process(COMMAND bash -c
                        "if test -w ${PYTHON_SITELIB} ; then echo 0; else echo 1; fi"
                OUTPUT_STRIP_TRAILING_WHITESPACE
                OUTPUT_VARIABLE PYTHON_RIGHT_SITELIB)
if(${PYTHON_RIGHT_SITELIB})
    message("-- No right on system site-package: ${PYTHON_SITELIB}. Using user site as fallback.")
    execute_process(COMMAND "${PYTHON_EXECUTABLE}" -m site --user-site
                    OUTPUT_VARIABLE PYTHON_SITELIB)
    set(PYTHON_INSTALL_FLAGS "${PYTHON_INSTALL_FLAGS} --user ")
endif()

# Add Python dependencies
set(PYTHON_INCLUDE_DIRS "")
if(EXISTS /usr/include/python${PYTHON_VERSION})
    list(APPEND PYTHON_INCLUDE_DIRS /usr/include/python${PYTHON_VERSION})
else(EXISTS /usr/include/python${PYTHON_VERSION})
    list(APPEND PYTHON_INCLUDE_DIRS ${PYTHON_ROOT}/include/python${PYTHON_VERSION})
endif(EXISTS /usr/include/python${PYTHON_VERSION})
message("-- Found Python: ${PYTHON_EXECUTABLE} (found version \"${PYTHON_VERSION}\")")

# Find Numpy and add it as a dependency
find_package(NumPy REQUIRED)

if(${PYTHON_VERSION_MAJOR} EQUAL 3)
    set(BOOST_PYTHON_LIB "boost_numpy3")
    list(APPEND BOOST_PYTHON_LIB "boost_python3")
else(${PYTHON_VERSION_MAJOR} EQUAL 3)
    set(BOOST_PYTHON_LIB "boost_numpy")
    list(APPEND BOOST_PYTHON_LIB "boost_python")
endif(${PYTHON_VERSION_MAJOR} EQUAL 3)

# Define Python install helpers
function(deployPythonPackage TARGET_NAME)
    install(CODE "execute_process(COMMAND pip install ${PYTHON_INSTALL_FLAGS} .
                                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/pypi/${TARGET_NAME})")
endfunction()

function(deployPythonPackageDevelop TARGET_NAME)
    install (CODE "EXECUTE_PROCESS (COMMAND pip install -e .
                                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/${TARGET_NAME})")
endfunction()

# Add missing include & lib directory(ies)
# TODO: Cleanup after support of find_package for Eigen and Pinocchio,
# namely after migration to Eigen 3.3.7 / Boost 1.71, and Pinocchio 2.4.X
link_directories(SYSTEM /opt/openrobots/lib)
link_directories(SYSTEM /opt/install/pc/lib)
include_directories(SYSTEM /opt/openrobots/include/)
include_directories(SYSTEM /opt/install/pc/include/)
include_directories(SYSTEM /opt/install/pc/include/eigen3/)

# Due to license considerations, we will only use the MPL2 parts of Eigen.
set(EIGEN_MPL2_ONLY 1)

# Add utils to define package version
include(CMakePackageConfigHelpers)