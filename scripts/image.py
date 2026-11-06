import shutil
import subprocess
import sys
from tools import ROOT, tool

profile = sys.argv[1]  # 选择 Make 传入的构建配置，例如 normal
out = ROOT / "build" / profile
staging = out / "iso"

if staging.exists():  # 清理旧暂存目录，避免上一次遗留文件混入 ISO
    shutil.rmtree(staging)
(staging / "boot/limine").mkdir(parents=True)
(staging / "EFI/BOOT").mkdir(parents=True)

shutil.copyfile(
    out / "airios.elf", staging / "boot/airios.elf"
)  # 放到 limine.conf 指定的内核路径
shutil.copyfile(
    ROOT / "config/boot/limine.conf", staging / "limine.conf"
)  # 把启动项配置放到 ISO 根目录
shutil.copyfile(
    ROOT / "third_party/limine/BOOTX64.EFI", staging / "EFI/BOOT/BOOTX64.EFI"
)  # 提供 UEFI 默认启动路径 EFI/BOOT/BOOTX64.EFI
shutil.copyfile(
    ROOT / "third_party/limine/limine-uefi-cd.bin",
    staging / "boot/limine/limine-uefi-cd.bin",
)  # 放入 xorriso 要登记为 EFI 启动映像的 Limine 光盘映像

subprocess.run(
    [
        tool("xorriso", "xorriso"),
        "-as",  # 使用 mkisofs 参数兼容模式
        "mkisofs",
        "-R",  # 写入 Rock Ridge 扩展，保留 Unix 风格文件名和属性
        "-r",  # 规范化 ISO 文件所有者和权限
        "-J",  # 写入 Joliet 扩展，改善其他系统读取 ISO 文件名的兼容性
        "--efi-boot",  # 指定 ISO 内的 UEFI El Torito 启动映像路径
        "boot/limine/limine-uefi-cd.bin",
        "-efi-boot-part",  # 同时从指定映像生成 EFI 系统分区
        "--efi-boot-image",  # 将 EFI 启动映像作为 EFI 系统分区映像
        "--protective-msdos-label",  # 在 ISO 系统区域写入保护性 MBR 分区表
        str(staging),
        "-o",
        str(out / "airios.iso"),
    ],
    check=True,
)  # 在 xorriso 失败时抛出异常
print(f"Image: {out / 'airios.iso'}")
