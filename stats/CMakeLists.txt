add_executable(log_stats main.cpp)
target_link_libraries(log_stats logger)

if (WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET log_stats POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:logger>
            $<TARGET_FILE_DIR:log_stats>
    )
endif()