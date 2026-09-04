#!/usr/bin/env python3
import shutil
import subprocess
import sys
from pathlib import Path

ZIMM = Path("/opt/zimm")
BUILD = Path("/work/build")
ZE_BUILD = Path("/opt/harness/ze_build.cpp")


def tee_run(cmd, log: Path, cwd: Path) -> int:
    """Run cmd, streaming merged output to the console and to log."""
    proc = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, bufsize=1)
    with log.open("w") as f:
        assert proc.stdout is not None
        for line in proc.stdout:
            f.write(line)
            f.flush()
            print(line, end="", flush=True)
    return proc.wait()


def main() -> int:
    if BUILD.exists():
        shutil.rmtree(BUILD)
    BUILD.mkdir(parents=True)

    compileCmd = ["g++", str(ZE_BUILD), "-std=c++23", "-g",
             f"-I{ZIMM / 'include'}", f"-L{ZIMM / 'lib64'}", "-lzimmermann",
             "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-o", str(BUILD / "ze_build")]

    if tee_run(compileCmd, BUILD / "compile.log", cwd=BUILD) != 0:
        print(f"FAIL  compile ze_build.cpp — see {BUILD / 'compile.log'}",
              file=sys.stderr)
        return 1

    rc = tee_run([str(BUILD / "ze_build")], BUILD / "ze_build.log", cwd=BUILD)
    if rc != 0:
        print("\nfailure logs copied to host: "
              "zimm_testing/cmake_import/from_docker/.zimm_cmake_find/<pkg>/configure.log",
              file=sys.stderr)
    return rc


if __name__ == "__main__":
    sys.exit(main())
