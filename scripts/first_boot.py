from pathlib import Path
import shutil
import subprocess
import time
from tools import ROOT, tool

work = ROOT / "build/first-boot"
work.mkdir(parents=True, exist_ok=True)

firmware = ROOT / "third_party/firmware"
# UEFI 会更新固件内容，因此每次需要用副本
shutil.copyfile(firmware / "edk2-i386-vars.fd", work / "vars.fd")

serial = work / "serial.log"
serial.unlink(missing_ok=True)  # 清除上次日志

command = [
    tool("qemu-system-x86_64", "qemu"),
    "-machine",  # 选择较现代的 PC 芯片组模型，包含 PCIe 根总线
    "q35",
    "-accel",  # 使用软件模拟 CPU，以便跨平台虚拟
    "tcg",
    "-cpu",  # 为 guest 提供通用 x86-64 CPU 型号
    "qemu64",
    "-smp",  # 只分配一个虚拟 CPU
    "1",
    "-m",  # 给 guest 分配的 RAM 容量
    "256M",
    "-vga",  # 保留标准 VGA 设备
    "std",
    "-display",  # 关闭图形窗口，改用串口日志观察启动输出
    "none",
    "-monitor",  # 关闭交互式监视器
    "none",
    "-no-reboot",  # guest 复位时让 QEMU 退出，而不是自动重新启动
    "-drive",  # 添加一个固件闪存设备；unit=0 装入只读数据，这里是 UEFI 固件代码
    f"if=pflash,format=raw,unit=0,readonly=on,file={firmware / 'edk2-x86_64-code.fd'}",
    "-drive",  # 这里装入是 UEFI 变量存储镜像，里面保存固件设置和启动状态
    f"if=pflash,format=raw,unit=1,file={work / 'vars.fd'}",
    "-cdrom",  # 把构建出的 ISO 作为虚拟光盘也挂载上去
    str(ROOT / "build/normal/airios.iso"),
    "-boot",  # d 表示优先从光驱启动
    "d",
    "-serial",  # 将虚拟机串口输出放到指定文件
    f"file:{serial}",
]

with (work / "qemu.log").open("w") as qemu_log:
    process = subprocess.Popen(command, stdout=qemu_log, stderr=qemu_log)
    try:
        deadline = time.monotonic() + 90
        while time.monotonic() < deadline:
            text = serial.read_text(errors="replace") if serial.exists() else ""
            if any(
                line.startswith("[AIRI OS]") and line.endswith("\n")
                for line in text.splitlines(keepends=True)
            ):
                print(text, end="")
                break
            if process.poll() is not None:
                raise RuntimeError("QEMU exited before the complete serial marker")
            time.sleep(0.1)
        else:
            raise RuntimeError("Boot timed out after 90 seconds")
    finally:
        process.terminate()  # guest 执行 hlt 后 QEMU 仍运行，需要由主机终止它
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
