CC := clang
# 立即展开右边变量并赋值。
LD := ld.lld
# 预留未使用。
HOST_CC := cc
# 条件赋值，如果为空就赋值。
PROFILE ?= normal
OUT := build/$(PROFILE)

SOURCES := kernel/lib/boot/limine.c kernel/lib/boot/info.c kernel/lib/base.c \
           kernel/lib/runtime.c kernel/lib/arch/x86_64/serial.c
OBJECTS := $(patsubst %.c,$(OUT)/%.o,$(SOURCES)) \
           $(OUT)/kernel/lib/arch/x86_64/entry.o
# GNU Make 内置 pattern substance 函数，% 代表匹配结果，语法是 $(patsubst 匹配模式, 替换模式, 输入文本)

# Clang target triple：CPU 架构为 x86-64，目标环境为无操作系统的 ELF 裸机。
# -g：生成调试信息，供调试器把机器指令对应回源码。
# -Wall：启用常见警告；
# -Wextra：启用额外警告；
# -Werror：把警告当错误处理。
# -ffreestanding：告诉编译器这是内核等独立运行环境，不假设有宿主机运行库和启动环境。
# -fno-builtin：不要把标准库函数名当作编译器内建实现；内核会提供自己的基础函数。
# -fno-stack-protector：不插入栈溢出保护代码，避免依赖尚未实现的 canary （金丝雀 / CSAPP 里的防注入）运行时。
# -fno-pic：不生成位置无关代码；
# -fno-pie：不生成位置无关可执行文件代码。内核由链接脚本放在约定地址，当前不采用 PIE/PIC 的重定位模型。
# -mno-red-zone：禁用 x86-64 SysV ABI 的 red zone，避免中断/异常使用栈时覆盖它。
# -mgeneral-regs-only：只使用通用寄存器，不生成 x87/MMX/SSE 等浮点或向量指令。
# -mcmodel=kernel：使用 x86-64 kernel code model，假定内核位于高半区的内核地址范围。
# -MMD：编译时生成 .d 头文件依赖文件，但不记录系统头文件。
# -MP：为依赖的头文件生成空规则，避免头文件被删除/改名后 Make 因旧依赖报错。
CFLAGS := --target=x86_64-unknown-none-elf -std=c11 -O2 -g -Wall -Wextra -Werror \
          -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie \
          -mno-red-zone -mgeneral-regs-only -mcmodel=kernel -MMD -MP
ASFLAGS := --target=x86_64-unknown-none-elf -g

# CPPFLAGS 是预处理器选项，通常由编译器在 C 编译阶段传给预处理器。
# -I<目录>：增加头文件搜索目录；尖括号/引号 include 会据此查找项目头文件。
CPPFLAGS := -Iinclude -Ithird_party

ifeq ($(PROFILE),no-framebuffer)
CPPFLAGS += -DAIRI_TEST_NO_FRAMEBUFFER	# -DNAME 定义预处理器注入的宏，源码可用 #ifdef 检查它。
endif

ifeq ($(PROFILE),edit-exprimental)
CPPFLAGS += -DAIRI_PANEL_TITLE='"LOCAL / MAINTENANCE CONSOLE"' \
            -DAIRI_DEMO_MESSAGE='"I changed the output and rebooted"'
endif

.PHONY: all kernel image host-test clean deps
all: image
kernel: $(OUT)/airios.elf

$(OUT)/%.o: %.c
	@mkdir -p $(dir $@)							# 调用 GNU Make 的内置 dir 函数，提取目录部分
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@		# $< 代表当前规则的第一个依赖文件，$@ 代表当前规则的目标文件

$(OUT)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

# -m elf_x86_64：选择 x86-64 ELF 链接目标（emulation），与内核架构一致。
# -nostdlib：不搜索链接器的默认库目录；本规则只使用明确给出的对象文件和链接脚本。
# -static：生成静态链接映像，不依赖动态链接器或共享库。
# -z max-page-size=0x1000：把 ELF 可加载段的最大页面对齐设为 0x1000（4 KiB）。
# -z noexecstack：标记程序栈不可执行，避免栈内数据被当成指令执行。
# -T kernel/linker.ld：用这个链接脚本安排内核各段的地址和布局。
$(OUT)/airios.elf: $(OBJECTS) kernel/linker.ld
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 \
		-z noexecstack -T kernel/linker.ld $(OBJECTS) -o $@

image: $(OUT)/airios.elf				# 可启动镜像通常还需要引导器文件、配置文件、目录结构等
	python3 scripts/image.py $(PROFILE)
-include $(OBJECTS:.o=.d)				# GNU Make 的变量替换引用 $(OBJECTS:模式=替换)

deps:
	python3 scripts/deps.py