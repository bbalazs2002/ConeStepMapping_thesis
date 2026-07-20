add_executable(tests_no_gl
    test_transform.cpp
    test_commandqueue.cpp
    test_materialmanager.cpp
    test_modelloader.cpp
    test_scenemanager.cpp
)

target_link_libraries(tests_no_gl PRIVATE
    CSM_lib
    GTest::gtest
    GTest::gtest_main
)

# CTest registration — label "NoGL" for filtering in CI
add_test(NAME NoGL COMMAND tests_no_gl)
set_tests_properties(NoGL PROPERTIES LABELS "NoGL")
