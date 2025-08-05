add_executable(log_tests logger_test.cpp)
target_link_libraries(log_tests logger)

if (WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET log_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:logger>
            $<TARGET_FILE_DIR:log_tests>
    )
endif()
