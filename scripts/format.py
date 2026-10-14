import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
SKIP_DIRS = {
    ".git",
    ".venv",
    "venv",
    "build",
    "dist",
    "target",
    "node_modules",
    "__pycache__",
}
PYTHON_SUFFIXES = {".py"}
C_SUFFIXES = {".c", ".h"}
CPP_SUFFIXES = {".cc", ".cpp", ".cxx", ".hh", ".hpp", ".hxx"}


def source_files():
    python_files = []
    c_files = []
    cpp_files = []

    for current, directories, filenames in os.walk(ROOT):
        directories[:] = sorted(
            name for name in directories if name not in SKIP_DIRS
        )
        for filename in filenames:
            path = Path(current) / filename
            if path.suffix in PYTHON_SUFFIXES:
                python_files.append(path)
            elif path.suffix in C_SUFFIXES:
                c_files.append(path)
            elif path.suffix in CPP_SUFFIXES:
                cpp_files.append(path)

    return sorted(python_files), sorted(c_files), sorted(cpp_files)


def main():
    parser = argparse.ArgumentParser(
        description="Format project Python and C/C++ source files."
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="check formatting without changing files",
    )
    args = parser.parse_args()

    python_files, c_files, cpp_files = source_files()
    if not any((python_files, c_files, cpp_files)):
        print("没有找到受支持的源文件。")
        return 0

    tasks = []
    if python_files:
        ruff = shutil.which("ruff")
        if ruff is None:
            print("缺少 ruff；请先安装 Python 格式化工具 ruff。", file=sys.stderr)
            return 2
        command = [ruff, "format"]
        if args.check:
            command.append("--check")
        tasks.append(
            (
                "Python",
                command + [str(path) for path in python_files],
                len(python_files),
            )
        )

    if c_files or cpp_files:
        clang_format = shutil.which("clang-format")
        if clang_format is None:
            print(
                "缺少 clang-format；请先安装 C/C++ 格式化工具 clang-format。",
                file=sys.stderr,
            )
            return 2
        command = [clang_format]
        if args.check:
            command.extend(["--dry-run", "--Werror"])
        else:
            command.append("-i")
        tasks.append(
            (
                "C/C++",
                command + [str(path) for path in c_files + cpp_files],
                len(c_files) + len(cpp_files),
            )
        )

    failed = False
    for language, command, file_count in tasks:
        print(f"格式化 {language} 文件（{file_count} 个）...")
        result = subprocess.run(command, cwd=ROOT, check=False)
        failed |= result.returncode != 0

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
