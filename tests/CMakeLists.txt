project(allocator_tests CXX)

add_executable(test_allocator)
target_sources(test_allocator PRIVATE 
test_balloc.cpp
test_balloc_critical.cpp
)

set_target_properties(test_allocator PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    LINKER_LANGUAGE CXX
)

target_include_directories(test_allocator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common/catch
)

target_link_libraries(test_allocator PRIVATE block_allocator)

add_test(NAME allocator_tests COMMAND test_allocator)
