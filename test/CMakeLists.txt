# Criterion is a test-only dependency and is never linked into the server, so a
# tree without it still builds and runs normally -- the tests just disappear.
# Install it with: apt-get install libcriterion-dev
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(CRITERION QUIET IMPORTED_TARGET criterion)
endif()

if(NOT CRITERION_FOUND)
    message(STATUS "Criterion not found, skipping tests (apt-get install libcriterion-dev)")
    return()
endif()

message(STATUS "Criterion ${CRITERION_VERSION} found, tests enabled")

# One binary per source module under test. Criterion registers the tests itself
# and supplies main(), so there is no runner to keep in sync.
foreach(suite test_utils test_cache test_serve)
    add_executable(${suite} ${suite}.c helpers.c)

    target_include_directories(${suite} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

    target_compile_options(${suite} PRIVATE
        $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>)

    # server_core carries the include paths and -pthread with it.
    target_link_libraries(${suite} PRIVATE server_core PkgConfig::CRITERION)

    # Criterion runs each test in its own process, which is what keeps the
    # process-global page cache from leaking between cases. --verbose names each
    # test as it runs, so a failure in CI points straight at the case.
    add_test(NAME ${suite} COMMAND ${suite} --verbose)
endforeach()
