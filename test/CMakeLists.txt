
set(TEST_LIST
    parse_maps
    write_read_mdata
    ptracer_attach
    
    make_ckpt
    restore
    make_ckpt_threads
    restore_threads
)

# add the executables cpp
foreach (test_name ${TEST_LIST})
	add_executable(${test_name} ${test_name}.cpp)
	target_link_libraries(${test_name} PRIVATE reck)
    add_test(NAME "${test_name}_test" COMMAND ${test_name})
endforeach (test_name)

set_tests_properties(restore_test PROPERTIES DEPENDS make_ckpt_test)
set_tests_properties(restore_threads_test PROPERTIES DEPENDS make_ckpt_threads_test)