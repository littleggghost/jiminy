# Minimum version required
cmake_minimum_required(VERSION 3.10)

# Project name
project(${LIBRARY_NAME}_pywrap VERSION ${BUILD_VERSION})

# Add cpp sources
set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/Module.cc")

# Make library
add_library(${PROJECT_NAME} SHARED ${SRC})
add_definitions(-DBOOST_PYTHON_STATIC_LIB)
add_definitions(-DBOOST_PYTHON_STATIC_MODULE)
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${BUILD_VERSION}
)

# Substitute Docstring @copydoc flags with C++ Doxygen documentations
pythonDocstingSubstitution()

# Set include directory (build folder because of the substitution)
target_include_directories(${PROJECT_NAME} PUBLIC
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/${PROJECT_NAME}/include/>"
)

# Link with other libraries
target_link_libraries(${PROJECT_NAME}
        PUBLIC ${LIBRARY_NAME}_core eigenpy "${BOOST_PYTHON_LIB}"
)
target_include_directories(${PROJECT_NAME}
        SYSTEM PRIVATE ${NUMPY_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS}
)

# Activate C++14
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

# Install C++ headers
install(DIRECTORY "include/${LIBRARY_NAME}"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Generate Cmake configuration files used by find_package
install(TARGETS ${PROJECT_NAME}
        EXPORT  ${PROJECT_NAME}Config
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

exportCmakeConfigFiles()
