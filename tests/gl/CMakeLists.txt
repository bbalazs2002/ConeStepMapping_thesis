add_executable(tests_gl
    gl_test_main.cpp
    test_texturemanager.cpp
    test_mesh_build.cpp
    test_scene_integration.cpp
)

target_link_libraries(tests_gl PRIVATE
    CSM_lib
    GTest::gtest
)

# CTest registration — label "GL" for filtering; self-hosted runner only
add_test(NAME GL COMMAND tests_gl)
set_tests_properties(GL PROPERTIES LABELS "GL")
