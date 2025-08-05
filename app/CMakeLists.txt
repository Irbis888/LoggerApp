add_executable(log_app main.cpp)
target_link_libraries(log_app logger)

if (WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET log_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:logger>
            $<TARGET_FILE_DIR:log_app>
    )
endif()