cmake_minimum_required(VERSION 3.10)

project(shared_spin_mutex VERSION 0.1)

########################################################################################################################
# If inside subdirectory.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(${PROJECT_NAME}_sub OFF)
else()
    set(${PROJECT_NAME}_sub ON)
endif()

########################################################################################################################
# Library.
set(headers include/shared_spin_mutex.hpp)
add_library(${PROJECT_NAME} INTERFACE ${headers})
target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE $<INSTALL_INTERFACE:include>)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

########################################################################################################################
# Testing.
option(${PROJECT_NAME}_TESTING "Enable testing" OFF)
if(${PROJECT_NAME}_TESTING)
    find_package(Boost 1.71 COMPONENTS system thread QUIET REQUIRED)
    find_package(Fmt QUIET REQUIRED)
    add_executable(test_${PROJECT_NAME} test/main.cpp)
    target_link_libraries(test_${PROJECT_NAME} PRIVATE ${PROJECT_NAME} ${Boost_LIBRARIES} fmt::fmt-header-only)
    set_target_properties(test_${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
endif()

########################################################################################################################
# Install.
if(NOT ${PROJECT_NAME}_sub)
    install(FILES ${headers} DESTINATION include)
    include(CMakePackageConfigHelpers)
    set(CONFIG_PACKAGE_INSTALL_DIR lib/cmake/${PROJECT_NAME})
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
        COMPATIBILITY SameMajorVersion
    )
    install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}-targets DESTINATION lib)
    install(EXPORT ${PROJECT_NAME}-targets DESTINATION ${CONFIG_PACKAGE_INSTALL_DIR})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
        @ONLY
    )
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
        DESTINATION ${CONFIG_PACKAGE_INSTALL_DIR}
    )
endif()
