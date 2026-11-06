#include <airi/kernel.h>

#pragma region 硬件寄存器分布
#define COM1_BASE 0x3f8        // COM1 设备的寄存器占用的基址
#define COM1_THR COM1_BASE     // 可以作为传输保持寄存器用
#define COM1_RBR COM1_BASE     // 可以作为接收数据寄存器用
#define COM1_DLL COM1_BASE     // 可以配置 Buad Rate 除数低字节
#define COM1_IER COM1_BASE + 1 // 中断使能寄存器
#define COM1_DLM COM1_BASE + 1 // Buad Rate 除数高字节
#define COM1_FCR COM1_BASE + 2 // FIFO 写入控制寄存器
#define COM1_IIR COM1_BASE + 2 // 中断查询寄存器
#define COM1_LCR COM1_BASE + 3 // 线路控制寄存器
#define COM1_MCR COM1_BASE + 4 // 调制解调器控制和辅助寄存器
#define COM1_LSR COM1_BASE + 5 // 线路状态寄存器
#define COM1_MSR COM1_BASE + 6 // 调制解调器状态
#define COM1_SCR COM1_BASE + 7 // 通用暂存寄存器
#pragma endregion

// 第一个低级串口驱动程序
// COM1 的名字和值，以及连着一个串口设备均为历史规范
// 这里还没有 MMIO 和高级的中断机制，直接通过端口映射 IO 这种原始手段在写

static inline void out8(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t in8(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// 初始化串口设备的硬件驱动
void airi_serial_init(void) {
    // COM1 属于 UART 设备，UART 是一个概称，指代所有实现**异步**，**串行**收发的硬件控制器
    // 这里在为经典的 16550 芯片编写驱动
    // baud（波特）指每秒的信号变化次数
    // 0x3FB 是 UART 的线路控制寄存器。其中值的最高位叫 DLAB
    // 设置它的作用是切换寄存器解释方式，以用较少的地址控制较多的功能
    out8(COM1_LCR, 0b00000011); // 清理 DLAB

    // 中断使能寄存器
    // DLAB=0 时控制中断；DLAB=1 时写除数高字节
    out8(COM1_IER, 0); // 关闭中断，为了简单暂时轮询发送

    // UART 的线路控制寄存器
    out8(COM1_LCR, 0b10000000); // 设置 DLAB = 1，开始配置波特率
    // DLAB=0 时写数据；DLAB=1 时写波特率除数低字节
    out8(COM1_DLL, 1); // 设置 UART 时钟内部分频器，这里设置为 1 是常见的默认配置
    // DLAB=0 时控制中断；DLAB=1 时写除数高字节
    out8(COM1_DLM, 0); // 高位用不到，写 0

    // 波特率写完了，重新设置 UART 的线路控制寄存器
    out8(COM1_LCR, 0b00000011); // 清 DLAB，不用奇偶校验和中断
    // 1 个停止位 8 个数据位

    // 配置收发 FIFO
    out8(COM1_FCR, 0b11000111); // 启动 FIFO，清空接受和发送队列，中断触发阈值 14

    // 设置控制线路
    out8(COM1_MCR, 0b00001011); // 声明终端已就绪，请求发送
}

static void putchar_serial(char c) {
    for (unsigned n = 0; n < 1000000; n++) {
        if (in8(COM1_LSR) & 0b00100000) {
            out8(COM1_THR, (uint8_t)c);
            return;
        }
    }
    __asm__ volatile("pause"); // 未就绪时给 CPU 忙等提示，未进行任务切换
}

void airi_serial_write(const char *s) {
    while (*s) {
        if (*s == '\n') {
            putchar_serial('\r'); // 先发送回车，下一次发送原换行
        }
        putchar_serial(*s);
        s++;
    }
}

void airi_log(const char *fmt, ...) {
    char line[512];
    va_list args;
    va_start(args, fmt);
    // 调用先前封装的格式化工具函数
    airi_vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    airi_serial_write(line);
    airi_serial_write("\n");
}

_Noreturn void airi_halt() {
    for (;;) {
        __asm__ volatile("cli; hlt"); // cli 关闭中断，hlt 设置停止执行
    }
}

_Noreturn void airi_panic(const char *message) {
    airi_log("[AIRI-OS PANIC] %s", message);
    airi_halt();
}