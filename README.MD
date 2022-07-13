# Toy-Redis

## 编译运行脚本
```shell
# run test
sh script/test.sh
# run
sh script/run.sh
```

## 项目目录结构
```
.
├── CMakeLists.txt  
├── README.MD
├── base            // 基本容器和工具库的实现
├── ipc             // ipc 封装
├── net             // 事件循环网络库封装
├── toy-redis       // 主要业务逻辑
├── tests           // 单元测试
├── script          // 开发脚本
├── cmake           // cmake 脚本
└── main.cpp        // 程序入口
```