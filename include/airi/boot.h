#ifndef AIRI_BOOT_H
#define AIRI_BOOT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
Day-2 讲到 PCIe 时，问题是：
1. 设备如何被找到，以及设备的配置空间窗口 ECAM 在哪儿？
2. 每台机器的 ECAM 位置可能不同，内核不能把某个固定地址写死。

ACPI 在这里扮演的是平台说明书的角色：
    - **固件**准备 ACPI 表，描述这台机器的硬件平台。
    - Bootloader 把 ACPI 根指针，也就是 RSDP，交给内核。
    - 内核从 RSDP 找到后续的表，例如 XSDT 和 MCFG。
    - MCFG 描述 PCIe ECAM 配置空间的位置；
    - 内核再通过页表映射相应物理地址，读取设备配置空间并扫描 BDF。
这对应 Day-2 里“ACPI 来辅助，但我们无力关心”的那里：
    - 现在开始做的是把平台说明信息读进内核的接入层
        - ACPI 不只是“电源管理接口”，它还包含硬件描述表
        - 而 ECAM 的位置是由 ACPI 的 MCFG 表提供的。
*/

#define AIRI_BOOT_MAGIC UINT64_C(0x41495249424f4f54)
#define AIRI_BOOT_VERSION 1
#define AIRI_MAX_MEMORY_REGIONS 128 // 预期最多接受 128 个内存布局描述
enum airi_memory_kind {
    AIRI_MEMORY_USABLE,
    AIRI_MEMORY_RESERVED,
    AIRI_MEMORY_ACPI_RECLAIM, // 硬件和电源信息表等信息所占用的临时空间
    AIRI_MEMORY_ACPI_NVS,     // 非易失性的，硬件和电源信息占用内存空间，休眠等与固件交互的行为要用
    AIRI_MEMORY_BAD,          // 内存区域不可动
    AIRI_MEMORY_BOOT_RECLAIM, // 引导器使用的内存范围
    AIRI_MEMORY_KERNEL,       // 内核及其装载内容所占范围
    AIRI_MEMORY_FRAMEBUFFER   // 显示屏像素缓冲区
};
struct airi_memory_region {
    uint64_t base;
    uint64_t length;
    uint32_t kind;
    uint32_t reserved;
};
struct airi_framebuffer {
    uint64_t virtual_addr;         // 可写虚拟地址，显示驱动来操作
    uint64_t physical_addr;        // 对应物理起点，不要直接写
    uint64_t width, height, pitch; // pitch 是两行起点之间的字节距离
    uint16_t bpp;                  // bit count per pixel
    uint8_t red_size, red_shift, green_size, green_shift, blue_size, blue_shift;
    // 描述上述这个 bpp 的空间怎么分配
    // 每个颜色用多少位描述，低位起点从其中第几位开始
};
struct airi_boot_info {
    uint64_t magic;             // 类似哈希值的标志数字
    uint32_t version, size;     // 版本和本信息结构体
    uint64_t direct_map_offset; // 直接映射偏移量
    uint64_t kernel_physical_base, kernel_virtual_base, kernel_size;
    // 内核影像（image，可以理解为一段二进制数据段）在物理地址，虚拟地址的起点，以及块大小
    uint32_t region_count, reserved; // 本信息结构体含有几个有效区域记录
    char loader[32];                 // 保存引导器的名字
    struct airi_framebuffer framebuffer;
    struct airi_memory_region regions[AIRI_MAX_MEMORY_REGIONS];
};

const char *airi_boot_validate(const struct airi_boot_info *info);
bool airi_framebuffer_valid(const struct airi_framebuffer *fb);
uint64_t airi_memory_bytes(const struct airi_boot_info *info, uint32_t kind);
const char *airi_memory_name(uint32_t kind);
_Noreturn void airi_kernel_main(const struct airi_boot_info *info);
#endif