.section .text
.global _start
.type _start, @function
_start:
    cli     // clear interrupt flag，关可屏蔽中断，内核还不支持
    cld     // clear direction flag，设置 DF 为 0，字符串指令按递增方向用地址
    lea airi_stack_top(%rip), %rsp  // instruction pointer 寄存器加上栈基址，载入 stack pointer
    and $-16, %rsp                  // 栈地址 16 字节对齐，补码值 -16 相当于全 1 的整数减去 +15
    xor %ebp, %ebp                  // 对 base pointer（栈帧基址）清零
    call airi_limine_start
1:
    hlt                 // 这里不应该再被执行，只是兜底停机
    jmp 1b              // 汇编器语法，b 表示向后找最近标签 1，恢复执行后再次停机
.size _start, .-_start  // 记录元数据，即 _start 对应函数大小
.section .bss
.balign 16

airi_stack_bottom:
    .skip 65536                     // bss 段声明语法，要求加载 ELF 时分配 64 kiB 初始栈
airi_stack_top:
.section .note.GNU-stack,"",@progbits
// 定义或切换到一个名为 .note.GNU-stack 的 Section
// 区段的标志（Flags）为空，包含程序数据（Program Bits）
// 即在编译和链接时需要为它分配空间并记录其属性。
