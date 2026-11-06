OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)   /* ELF header 的入口地址设为汇编入口 _start。 */
PHDRS {         /* 定义 ELF 程序头；PT_LOAD 描述装载到内存的段。 */
    requests PT_LOAD FLAGS(4);  /* requests 段只读。 */
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
}
SECTIONS {
    . = 0xffffffff80000000;     /* 设置内核高半区虚拟地址起点。 */
    __kernel_start = .;         /* 记录内核映像起点。 */

    /* 把输入目标文件的 section 整理到内核 ELF 的输出 section 中。 */
    .limine_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :requests
    . = ALIGN(4096);
    .text : {
        *(.text, .text.*)
    } :text
    . = ALIGN(4096);
    .rodata : {
        *(.rodata, .rodata.*)
    } :rodata
    . = ALIGN(4096);
    .data : {
        *(.data, .data.*)
    } :data
    .bss : {
        *(.bss, .bss.*)
        *(COMMON)       /* 兼容旧式未初始化全局变量的 COMMON 输入 section。 */
    } :data
    . = ALIGN(4096);
    __kernel_end = .;   /* 记录内核映像终点。 */
    /DISCARD/ : {       /* 丢弃当前内核尚未使用的元数据。 */
        *(.eh_frame*)   /* 栈展开信息。 */
        *(.note*)       /* ELF note 元数据。 */
    }
}
