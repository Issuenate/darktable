include_directories("${CMAKE_CURRENT_BINARY_DIR}/../")

add_executable(darktable-test-variables variables.c)
target_link_libraries(darktable-test-variables lib_darktable)

add_executable(darktable-test-lightroom-presets lightroom_presets.c)
target_link_libraries(darktable-test-lightroom-presets lib_darktable)
add_dependencies(darktable-test-lightroom-presets exposure grain splittoning bilat vignette rgbcurve colorzones)
add_test(NAME lightroom_presets
    COMMAND darktable-test-lightroom-presets "${DARKTABLE_DATADIR}" "${DARKTABLE_LIBDIR}")

if(WIN32)
    # This tester sets up a darktable instance (of sorts). Hence it expects libraries at ../lib/darktable
    # Easiest way to comply with this on Windows: Put tester executable in same directory as darktable executable
    set_target_properties(darktable-test-variables PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${DARKTABLE_BINDIR}
    )
    set_target_properties(darktable-test-lightroom-presets PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${DARKTABLE_BINDIR}
    )
endif(WIN32)

add_subdirectory(unittests)

add_executable(darktable-test-retouch-remove retouch_remove.c)
target_link_libraries(darktable-test-retouch-remove lib_darktable)
add_dependencies(darktable-test-retouch-remove retouch)
add_test(NAME retouch_remove
    COMMAND darktable-test-retouch-remove "${DARKTABLE_DATADIR}" "${DARKTABLE_LIBDIR}")
if(WIN32)
    set_target_properties(darktable-test-retouch-remove PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${DARKTABLE_BINDIR})
endif()
