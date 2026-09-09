add_subdirectory(common)
add_subdirectory(iop)

if(USE_AI)
  add_subdirectory(ai)
endif(USE_AI)

if(USE_MCP)
  add_subdirectory(mcp)
endif(USE_MCP)

add_cmocka_test(test_sample
                SOURCES test_sample.c
                LINK_LIBRARIES cmocka)

add_cmocka_test(test_capabilities
                SOURCES test_capabilities.c
                LINK_LIBRARIES lib_darktable cmocka::cmocka)

add_cmocka_test(test_essentials_export
                SOURCES test_essentials_export.c
                LINK_LIBRARIES lib_darktable cmocka::cmocka)

# Windows: main-method requires the wrapper provided by lib-darktable
if(WIN32)
    target_link_libraries(test_sample PRIVATE lib_darktable)
    _copy_required_library(test_sample lib_darktable)
endif(WIN32)

find_package(Python3 QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
  add_test(NAME test_essentials_capability_registration
           COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/check_capability_registry.py)
  if(USE_MCP)
    # the schemas in data/capabilities are published as the wire contract, so a
    # response that does not satisfy one is a bug in the emitter or the schema
    add_test(NAME test_mcp_capability_contract
             COMMAND ${Python3_EXECUTABLE}
                     ${PROJECT_SOURCE_DIR}/tools/check_mcp_capability_contract.py
                     $<TARGET_FILE:darktable-mcp>)
  endif()
endif()

# HDR exposure-bracket auto-alignment. Calls the public dt_hdr_alignment_* API
# exported from lib_darktable (which carries the OpenCV-linked objects); the test
# itself adapts at runtime to OpenCV-present / -absent builds.
add_cmocka_test(test_hdr_alignment
                SOURCES test_hdr_alignment.c
                LINK_LIBRARIES lib_darktable cmocka)

if(WIN32)
    _copy_required_library(test_hdr_alignment lib_darktable)
endif(WIN32)

# White-box tests for the internal (static) helpers of the HDR alignment code.
# Following test_filmicrgb.c, the implementation is #included directly for static
# access; lib_darktable supplies what it links against (OpenCV, glib, ...).
# C++ because hdr_alignment.cc is (OpenCV 4 has no C API).
add_cmocka_test(test_hdr_alignment_internal
                SOURCES test_hdr_alignment_internal.cc
                LINK_LIBRARIES lib_darktable cmocka)

if(WIN32)
    _copy_required_library(test_hdr_alignment_internal lib_darktable)
endif(WIN32)
