# Airi-OS

x86-64 Linux 系统下主要为熟悉存储与 I/O 在系统调用层及以下架构的学习用操作系统项目。
目前为止在开发上不会有 Agent 系统的直接编码参与。

## 格式化代码

macOS 上先安装工具：

```sh
brew install ruff
brew install llvm
export PATH="$(brew --prefix llvm)/bin:$PATH"
```

然后在项目根目录执行：

```sh
python3 scripts/format.py
```

要只检查格式而不改文件：

```sh
python3 scripts/format.py --check
```