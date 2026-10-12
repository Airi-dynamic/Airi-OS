import os
from pathlib import Path
import platform
import shutil

ROOT = Path(__file__).resolve().parent.parent

# 给出可执行文件名称为 name，所属软件包为 formula 的工具绝对路径
def tool(name, formula=None):
    variable = 'AIRI_' + name.upper().replace('-','=').replace('.','_')
    if variable in os.environ:
        value = os.environ[variable]
        candidate = Path(value).expanduser()
        if not value or not candidate.is_file() or not os.access(candidate, os.X_OK):
            raise RuntimeError(f'{variable} 必须是一个可执行文件. 检测到的路径 {value!r} 并不是.')
        return str(candidate.absolute())
    if platform.system() == 'Darwin' and formula:
        for prefix in ('/opt/homebrew', '/usr/local'):
            candidate = Path(prefix) / 'opt' / formula / 'bin' / name
            if candidate.is_file() and os.access(candidate, os.X_OK):
                return str(candidate)
    path = shutil.which(name)
    if path is not None:
        return path
    raise RuntimeError(f'缺少 {name}.\nmacOS: brew install llvm lld qemu xorriso mtools make; \n'        # 相邻字符串字面量会拼成同一消息
                       'Debian/Ubuntu: apt install clang lld llvm qemu-system-x86 ovmf xorriso mtools make; \n'        # Debian 系工具包名
                       'Arch/CachyOS: pacman -S clang lld llvm qemu-system-x86 edk2-ovmf xorriso mtools make. \n'      # Arch 系工具包名
                       'Firmware must also match config/deps.lock.json. \n')                                # 工具安装之外，固件还须匹配项目锁
    
# 生成根据给定构建配置（profile）和构建目标（target）的 make 指令字符串（这里以 list 对象形式给出）
def make_command(profile='normal', target='all'):
    if 'AIRI_GMAKE' in os.environ:
        make = tool('gmake', 'make')
    else:
        try:
            make = tool('gmake', 'make')
        except RuntimeError:
            make = tool('make')
    return [make, '-j8', f'PROFILE={profile}', f'CC={tool("clang", "llvm")}',   # make 程序名，并发数，配置和编译器赋值
            f'LD={tool("ld.lld", "lld")}', f'HOST_CC={tool("cc")}', target]     # 链接器选项，主机编译器，目标
    