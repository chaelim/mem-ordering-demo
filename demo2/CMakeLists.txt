project(demo2)

include(../cmake/BuildSettings.cmake)
include_directories(../inc)

file(GLOB SRCFILES *.cpp)
file(GLOB INCFILES ../inc/*.h)

add_executable(${PROJECT_NAME}  ${SRCFILES} ${INCFILES})
target_link_libraries(${PROJECT_NAME} gtest_main)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    COMPILE_DEFINITIONS "$<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_WARNINGS>"
    COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/EHsc>"
)

# Turn on CMake testing capabilities
enable_testing()

# Add test cases
add_test(NAME ${PROJECT_NAME}
        CONFIGURATIONS Release
        COMMAND ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})