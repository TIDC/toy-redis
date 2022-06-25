# @brief 新增单元测试的可执行文件
# @param target_name 可执行文件名称
# @param file_list 需要编译的 .cpp 文件，长度可变
#
# @example 创一个 hello world 测试程序:
#     add_test(TEST_hello hello_impl.cpp main.cpp)
function(add_test target_name file_list)
    set(argv_index 2)
    list(LENGTH ARGV argv_len)

    while(argv_index LESS ${argv_len})
        list(GET ARGV ${argv_index} argv_value)
        list(APPEND file_list ${argv_value})
        math(EXPR argv_index "${argv_index} + 1")
    endwhile()

    message(STATUS "增加单元测试 ${target_name}, 编译文件 ${file_list}")

    add_executable(
        ${target_name}
        ${file_list}
    )

    target_include_directories(
        ${target_name} PRIVATE
        ..
    )

    target_compile_options(
        ${target_name} PRIVATE
        -O2
        -g
        -ggdb
    )
endfunction()