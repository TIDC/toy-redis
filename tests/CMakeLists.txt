find_package(GTest)
if (NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://gitlab.com/zjlian/googletest.git
        GIT_TAG release-1.12.0
    )
    FetchContent_MakeAvailable(googletest)
endif()

add_compile_options(
    -Wall
    -Wno-unused-function
)

# SDS 单元测试
create_test(
    TEST_sds
    FILES test_sds.cpp
    LIBS gtest_main
)

create_test(
    TEST_ref_optional
    FILES test_reference_optional.cpp
    LIBS gtest_main
)

create_test(
    TEST_zipmap
    FILES test_zipmap.cpp
    LIBS gtest_main
)

create_test(
    TEST_dictionary
    FILES test_dictionary.cpp
    LIBS gtest_main
)

create_test(
    TEST_poller
    FILES test_poller.cpp
    LIBS gtest_main
)

create_test(
    TEST_timer
    FILES test_timer.cpp
    LIBS gtest_main
)

create_test(
    TEST_io_service
    FILES test_io_service.cpp
    LIBS gtest_main
)

create_test(
    TEST_iterator
    FILES test_iterator.cpp
    LIBS gtest_main
)

create_test(
    TEST_msg_que
    FILES test_message_queue.cpp
    LIBS gtest_main
)

create_test(
    TEST_defer
    FILES test_defer.cpp
    LIBS gtest_main
)