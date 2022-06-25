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

# SDS 单元测试
create_test(
    TEST_sds
    FILES test_sds.cpp
    LIBS gtest_main
)
