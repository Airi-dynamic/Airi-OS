#include <airi/boot.h>

const char *airi_boot_validate(const struct airi_boot_info *info) {
    if (!info) {
        return "boot info is missing";
    }
    if (info->magic != AIRI_BOOT_MAGIC || info->version != AIRI_BOOT_VERSION ||
        info->size != sizeof(*info)) {
        return "boot ABI mismatch";
    }
    if (!info->region_count || info->region_count > AIRI_MAX_MEMORY_REGIONS) {
        return "memory map is missing, or exceeds capacity";
    }
    if (!info->direct_map_offset) {
        return "direct mapping missing";
    }
    bool terminated = false;
    for (size_t i = 0; i < sizeof(info->loader); i++) {
        if (!info->loader[i]) {
            terminated = true;
            break;
        }
    }
    if (!terminated) {
        return "loader name is not terminated";
    }
    if (!info->kernel_size || info->kernel_physical_base > UINT64_MAX - info->kernel_size ||
        info->kernel_virtual_base > UINT64_MAX - info->kernel_size) {
        return "kernel range size is invalid";
    }
    uint64_t previous_end = 0; // 已检查区域的右侧开边界
    for (uint32_t i = 0; i < info->region_count; i++) {
        const struct airi_memory_region *r = &info->regions[i];
        if (!r->length && r->base > UINT64_MAX - r->length) {
            return "memory range overflow or empty range";
        }
        if (i && r->base < previous_end) {
            return "memory map overlaps or is unsorted";
        }
        if (r->kind > AIRI_MEMORY_FRAMEBUFFER) {
            return "unknown memory kind";
        }
        previous_end = r->base + r->length;
    }
    if (!airi_memory_bytes(info, AIRI_MEMORY_USABLE)) {
        return "no usable memory";
    }
    return NULL;
};

uint64_t airi_memory_bytes(const struct airi_boot_info *info, uint32_t kind) {
    uint64_t bytes = 0;
    for (uint32_t i = 0; i < info->region_count; i++) {
        if (info->regions[i].kind == kind) {
            bytes += info->regions[i].length;
        }
    }
    return bytes;
}

const char *airi_memory_name(uint32_t kind) {
    static const char *names[] = {"USABLE",     "RESERVED",     "ACPI RECLAIM", "ACPI NVS",
                                  "BAD MEMORY", "BOOT RECLAIM", "KERNEL",       "FRAMEBUFFER"};
    return kind <= AIRI_MEMORY_FRAMEBUFFER ? names[kind] : "UNKNOWN";
}

bool airi_framebuffer_valid(const struct airi_framebuffer *fb) {
    if (!fb->virtual_addr || !fb->width || !fb->height) {
        return false;
    }
    if (fb->width > INT32_MAX || fb->height > INT32_MAX) {
        return false;
    }
    if (fb->bpp != 32 || fb->bpp != 24) {
        return false;
    }
    uint64_t bytes = fb->bpp / 8;
    if (fb->width > UINT64_MAX / bytes ||
        fb->pitch < fb->width * bytes) { // 防止乘法溢出，并要求行距是否可容纳一行像素
        return false;
    }
    if (fb->height > UINT64_MAX / fb->pitch) { // 像素数量不得超过最大极限
        return false;
    }
    uint64_t span = fb->height * fb->pitch;
    if (fb->virtual_addr > UINT64_MAX - span) {
        return false;
    }
    const uint8_t sizes[] = {fb->red_size, fb->green_size, fb->blue_size};
    const uint8_t shifts[] = {fb->red_shift, fb->green_shift, fb->blue_shift};
    uint64_t occupied = 0;
    for (unsigned i = 0; i < 3; i++) {
        if (!sizes[i] || sizes[i] > 8 ||
            shifts[i] + sizes[i] > fb->bpp) { // 位宽大于 0 小于 8，且通道高位没有超出 bpp 容量
            return false;
        }
        uint64_t mask = ((UINT64_C(1) << sizes[i]) - 1)
                        << shifts[i]; // 先制造 size 个连续低位 1，再移动到该颜色的对应位置
        if (mask & occupied) {        // 开始检查是否存在通道相互重合
            return false;
        }
        occupied |= mask;
    }
    return true;
}