OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_START)   // _start 的地址写入 ELF 文件 header 的 e_entry 字段
PHDRS {         // 开始定义 ELF 文件 header 信息
    requests PT_LOAD FLAGS(4);  // requests 段，需要被加载，读写权限为可读
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
}
SECTIONS {
    . = 0xffffffff80000000;     // 设置 GNU ld 的位置分配计数器初始值为 HHDM 常见起点
    __kernel_start = .;         // 记录内核地址起点

    // 开始指定如何整理不同文件的 sections 最终成为单一文件
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
        *(COMMON)       // 历史兼容，并非所有项目都会产生 COMMON sections
    } :bss
    . = ALIGN(4096);
    __kernel_end = .;   // 记录内核地址终点，存储在 symbol table
    /DISCARD/ : {       // 临时手段，用 GNU ld 的特殊输出节写法丢弃部分信息，内核还无法支持复杂特性
        *(.eh_frame*)   // 保存栈展开等相关元数据
        *(.note*)       // 各种 ELF note 元数据
    }
}

