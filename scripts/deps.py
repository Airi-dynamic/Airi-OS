import argparse
import hashlib
import json
import shutil
import subprocess
import tarfile
from pathlib import Path
from pathlib import PurePosixPath
from tools import ROOT, tool


# 给出路径，对文件计算摘要
def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


# 从指定 URL 下载到指定路径并校验 Hash 的函数
def download(url, path, sha):
    # 已有就跳过
    if path.is_file() and digest(path) == sha:
        return

    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".download")
    try:
        subprocess.run(
            ["curl", "-fsSL", "--retry", "3", url, "-o", str(temporary)],
            check=True,
            capture_output=True,
        )
    except subprocess.CalledProcessError as error:
        temporary.unlink(missing_ok=True)
        raise RuntimeError(f"下载资源失败：{url}。请检查网络连接后重试。") from error
    if digest(temporary) != sha:
        temporary.unlink(missing_ok=True)
        raise RuntimeError(f"资源 {url} 的 SHA-256 和预期不匹配")
    temporary.replace(path)


# 逐个下载所有依赖的函数
def prepare(force=False):
    # 读配置
    lock = json.load((ROOT / "config/deps.lock.json").read_text())

    # 开始下载 limine BootLoader
    dep = lock["limine"]
    archive = ROOT / "third_party/limine_binary.tar.gz"
    if force or not archive.is_file() or digest(archive) != dep["sha256"]:
        download(dep["url"], archive, dep["sha256"])
    dest = ROOT / "third_party/limine"
    expected = {name: sha for name, sha in dep["files"].items()}
    if force or any(
        not (dest / name).is_file() or digest(dest / name) != sha
        for name, sha in expected.items()
    ):
        dest.mkdir(parents=True, exist_ok=True)
        with tarfile.open(archive) as tar:
            for member in tar.getmembers():
                archive_path = PurePosixPath(member.name)
                parts = archive_path.parts

                if archive_path.is_absolute() or ".." in parts:
                    raise RuntimeError(f"归档中包含不安全路径：{member.name}")
                if len(parts) > 1:
                    parts = parts[1:]
                if not parts:
                    continue

                target = (dest / Path(*parts)).resolve()

                try:
                    target.relative_to(dest.resolve())
                except ValueError as error:
                    raise RuntimeError(
                        f"归档路径超出目标目录：{member.name}"
                    ) from error

                if member.isdir():
                    target.mkdir(parents=True, exist_ok=True)
                elif member.isfile():
                    extracted = tar.extractfile(member)
                    if extracted is None:
                        raise RuntimeError(f"无法读取归档文件：{member.name}")
                    target.parent.mkdir(parents=True, exist_ok=True)
                    with extracted, target.open("wb") as output:
                        shutil.copyfileobj(extracted, output)
                else:
                    raise RuntimeError(f"归档中包含不支持的文件类型：{member.name}")

    # 开始下载 limine 协议头（以便内核与 BootLoader 通信）
    header = lock["protocol"]
    protocol_path = ROOT / "third_party/limine.h"
    if (
        force
        or not protocol_path.is_file()
        or digest(protocol_path) != header["sha256"]
    ):
        download(header["url"], protocol_path, header["sha256"])

    # 下载 QEMU 虚拟机的 OVMF 固件
    firmware = ROOT / "third_party/firmware"
    firmware.mkdir(parents=True, exist_ok=True)
    for item in lock["firmware"]:
        target = firmware / item["name"]
        if force or not target.exists() or item["sha256"] != digest(target):
            qemu = Path(tool("qemu-system-x86_64", "qemu")).resolve()
            candidates = [
                qemu.parent.parent / "share/qemu" / item["name"],
                Path("/opt/homebrew/opt/qemu/share/qemu") / item["name"],
                Path("/usr/share/qemu") / item["name"],
                Path("/usr/share/OVMF") / item["name"],
            ]
            source = next(
                (p for p in candidates if p.is_file() and digest(p) == item["sha256"]),
                None,
            )
            if not source:
                raise RuntimeError(
                    f"找不到摘要匹配的固件：{item['name']}。\n"
                    f"请使用参考版本 QEMU {lock['qemu_reference_version']}。\n"
                    "或者将匹配的固件放入 third_party/firmware。"
                )
            shutil.copyfile(source, target)
    return lock


# 入口函数
def main():
    parser = argparse.ArgumentParser(description="一键下载与校验 Airi-OS 依赖资源")
    parser.add_argument(
        "--force", action="store_true", help="忽略已存在文件，强制重新下载并覆盖"
    )
    args = parser.parse_args()

    print("正在准备项目依赖...")
    try:
        lock = prepare(force=args.force)
        print(
            f"准备完成：limine {lock['limine']['version']}，协议头 {lock['protocol']['commit'][:12]}，固件 {len(lock['firmware'])} 项"
        )
        print(f"输出目录：{ROOT / 'third_party'}")
    except Exception as exc:
        raise SystemExit(f"依赖准备失败：{exc}") from exc


if __name__ == "__main__":
    main()
