#
# We create two targets that call the unit test binaries in core/tests in
# different ways:
#    - test
#    - gtest
# The first target uses CMake's own testing facilities and gives a simple
# overview if a certain test module passed or not. 'gtest' on the other hand
# runs the `gtester` tool which outputs failing test cases and creates a summary
# HTML file called results.html using the `gtester-report` tool.
#
cmake_minimum_required(VERSION 2.8)

# configure unit tests
set(TEST_BINARIES
    buffer
    )

foreach(binary ${TEST_BINARIES})
    add_executable(${binary} ${binary}.c)
    target_link_libraries(${binary} ufo ${GLIB2_LIBRARIES})
    add_test(${binary} ${binary})
endforeach()

add_custom_target(gtest
    gtester ${TEST_BINARIES} -o results.xml 
)

# non-unit test
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test.json ${CMAKE_CURRENT_BINARY_DIR}/test.json)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test.js ${CMAKE_CURRENT_BINARY_DIR}/test.js)

add_executable(runjson test.c)
add_test(runjson runjson)
target_link_libraries(runjson ufo ${GLIB2_LIBRARIES})

if (VALA_FOUND)
    add_custom_target(foobar-vala ALL 
        ${VALA_EXECUTABLE} -o foobar-vala ${CMAKE_CURRENT_SOURCE_DIR}/test.vala)
    add_test(foobar-vala foobar-vala)
endif ()