# @brief 新增单元测试的可执行文件
# @param target_name 可执行文件名称
# @param file_list 需要编译的 .cpp 文件，长度可变
#
# @example 创一个 hello world 测试程序:
#     # 用法一：简单指定测试程序的 cpp 文件
#     create_test(TEST_hello hello_impl.cpp main.cpp)
#
#     # 用法二：程序程序要指定额外的编译参数和链接库
#     #        链接 pthread 库和添加编译参数 -Wextra
#     create_test(
#         TEST_hello
#         FILES hello_impl.cpp main.cpp
#         LIBS pthread  
#         OPTIONS -Wextra
#     )
function(create_test target_name ...)
    set(ars_FILES "FILES")              # 读取 .cpp 文件名称
    set(ars_LIBS "LIBS")                # 读取链接库名称
    set(ars_OPTIONS "OPTIONS")          # 读取编译参数
    set(arg_read_status ${ars_FILES})   # 参数解析状态
    
    set(file_list) # 需要编译的文件
    set(lib_list)  # 需要链接的库
    set(option_list)  # 需要添加的编译参数

    # 遍历下标 1 到 argv_len 的全部参数
    set(argv_index 1)
    list(LENGTH ARGV argv_len)
    while(argv_index LESS ${argv_len})
        set(status_change false)
        list(GET ARGV ${argv_index} argv_value)

        # message(STATUS "当前解析 ${arg_read_status} ${argv_index} ${argv_value}")

        # 检查是否读取到 FILES LIBS OPTIONS，修改 arg_read_status 的值，调整解析状态
        if (${argv_value} STREQUAL ${ars_FILES})
            set(arg_read_status ${ars_FILES})
            set(status_change true)
        endif()

        if (${argv_value} STREQUAL ${ars_LIBS})
            set(arg_read_status ${ars_LIBS})
            set(status_change true)
        endif()

        if (${argv_value} STREQUAL ${ars_OPTIONS})
            set(arg_read_status ${ars_OPTIONS})
            set(status_change true)
        endif()

        if (${status_change})
            math(EXPR argv_index "${argv_index} + 1")
            list(GET ARGV ${argv_index} argv_value)
            # message(STATUS "解析改变 ${arg_read_status} ${argv_index} ${argv_value}")
        endif()


        # 根据当前 arg_read_status 的值，将参数添加到对应的列表里
        if(${arg_read_status} STREQUAL ${ars_FILES})
            list(APPEND file_list ${argv_value})
        endif()

        if(${arg_read_status} STREQUAL ${ars_LIBS})
            list(APPEND lib_list ${argv_value})
        endif()

        if(${arg_read_status} STREQUAL ${ars_OPTIONS})
            list(APPEND option_list ${argv_value})
        endif()
        
        math(EXPR argv_index "${argv_index} + 1")
    endwhile()

    message(
        STATUS 
        "增加单元测试 ${target_name}; " 
        "编译文件 ${file_list}; " 
        "编译参数 ${option_list}; "
        "链接库 ${lib_list}; "
    )

    add_executable(
        ${target_name}
        ${file_list}
    )

    target_link_libraries(
        ${target_name} PRIVATE
        ${lib_list}
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
        -Wall
        ${option_list}
    )

    add_test(
        NAME ${target_name}
        COMMAND ${target_name}
    )
endfunction()