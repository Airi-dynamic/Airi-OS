#include <airi/kernel.h>
#include <limine.h>

// GCC 编译器扩展语法，显式为下一行指定编译器行为
// volatile 声明数据可能在 C 环境不知情的情况下被操作，不能随意优化。但无法保证线程安全等高级语义
// static 要求静态存储期，外部不可见
// 支持指定多个属性操作
// 首先要求无论编译器计算出如何的变量使用状态，都保留而不优化掉
// 其次要求放在一个指定名称的 ELF section 中
// 这里在自定义 section，正常 ELF 产物无此类 section
__attribute__((used, section(".limine_requests_start"))) static volatile uint64_t start[] =
    LIMINE_REQUESTS_START_MARKER;
__attribute__((used, section(".limine_requests"))) static volatile uint64_t revision[] =
    LIMINE_BASE_REVISION(6);
__attribute__((used, section(".limine_requests_end"))) static volatile uint64_t end[] =
    LIMINE_REQUESTS_END_MARKER;

_Noreturn void airi_limine_start(void) {
    airi_serial_init();      // 初始化串口设备
    airi_serial_write("\n"); // 结束可能残留在终端的固件控制字符
    if (LIMINE_BASE_REVISION_SUPPORTED(revision)) {
        airi_panic("Limine revision 6 (requested by AIRI-OS) is not supported. Boot Failed.");
    }
    airi_log("AIRI-OS init sequence completed. Entering kernel.");
    airi_halt();
}