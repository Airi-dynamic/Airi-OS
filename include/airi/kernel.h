#ifndef AIRI_KERNEL_H
#define AIRI_KERNEL_H
#include <airi/base.h>
#include <airi/boot.h>

void airi_serial_init(void);           // 配置外部输出串口 COM1 UART
void airi_serial_write(const char *s); // 同步发送字符
void airi_log(const char *format, ...);

_Noreturn void airi_halt(void); // 将 CPU 卡在死循环中实现逻辑停机
_Noreturn void airi_panic(const char *message);
bool airi_dashboard_draw(const struct airi_boot_info *info); // 预留的绘图接口
#endif