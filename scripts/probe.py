import shutil
import subprocess
import sys

def find_tool(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        raise RuntimeError(f"找不到可执行文件：{name}")
    return path

def main() -> int:
    name = sys.argv[1] if len(sys.argv) == 2 else "python3"
    try:
        path = find_tool(name)
        command = [path, "--version"]
        print(f"运行：{command}", flush=True)
        subprocess.run(command, check=True, timeout=10)
    except (RuntimeError, OSError, subprocess.SubprocessError) as error:
        print(f"错误：{error}", file=sys.stderr)
        return 1
    return 0

if __name__ == "__main__":
    raise SystemExit(main())