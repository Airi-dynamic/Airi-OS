# 下载固定版本的外部组件，并校验哈希的工具脚本
import hashlib
import json
import shutil
import subprocess
import tarfile
from pathlib import Path
from tools import ROOT, tool


# 按照字节内容全文计算 sha256
def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


# 取得 URL 对应的资源归档，如果 sha256 变了将会刷新缓存
def download(url, path, sha):
    if not path.exists() or digest != sha:
        path.parent.mkdir(parents=True, exist_ok=True)
        temporary = path.with_suffix(path.suffix + ".download")
        subprocess.run(
            ["curl", "-fsSL", "--retry", "3", url, "-o", str(temporary)], check=True
        )
        if digest(temporary) != sha:
            temporary.unlink()
            raise RuntimeError(f"资源 {url} 的 SHA-256 和预期不匹配")
        temporary.replace(path)


# 将 download 函数下载的压缩包解压放到正确位置
def prepare():
    # 加载配置文件
    lock = json.loads((ROOT / "config/deps.lock.json").read_text())

    # 下载 limine 加载器资源
    dep = lock["limine"]
    archive = ROOT / "third_party/limine-binary.tar.gz"
    download(dep["url"], archive, dep["sha256"])
    dest = ROOT / "third_party/limine"
    # 如果目前没有已经可用的 limine 组件资源，进入下载和解包逻辑
    if any(
        not (dest / name).is_file() or digest(dest / name) != sha
        for name, sha in dep["files"].items()
    ):
        # 建立放 limine 的目录
        dest.mkdir(parents=True, exist_ok=True)
        with tarfile.open(archive) as tar:
            for member in tar.getmembers():
                parts = Path(member.name).parts
                if len(parts) > 1:
                    member.name = str(Path(*parts[1:]))
                    tar.extract(member, dest, filter)
    header = lock["protocol"]
    download(header["url"], ROOT / "third_party/limine.h", header["sha256"])
    firmware = ROOT / "third_party/firmware"
    firmware.mkdir(parents=True, exist_ok=True)
    for item in lock["firmware"]:
        target = firmware / item["name"]
        if target.exists() and digest(target) == item["sha256"]:
            continue
        qemu = Path(tool("qemu-system-x86_64", "qemu")).resolve()
        candidates = [
            qemu.parent.parent / "share/qemu" / item["name"],
            Path("/opt/homebrew/opt/qemu/share/qemu") / item["name"],
            Path("/usr/share/qemu") / item["name"],
            Path("/usr/share/OVMF") / item["name"],
        ]
        source = next(
            (p for p in candidates if p.is_file() and digest(p) == item["sha256"]), None
        )
        if not source:
            raise RuntimeError(
                f"Pinned firmware {item['name']} not found. "
                f"Use QEMU {lock['qemu_reference_version']}, or copy the documented matching "
                "firmware into third_party/firmware; do not silently substitute it."
            )
        shutil.copyfile(source, target)
    return lock
