#!/usr/bin/env python3
import re
import shutil
import subprocess
import sys
from pathlib import Path

import yaml

ZIMM = Path("/opt/zimm")
BUILD = Path("/work/build")
ZE_BUILD = Path("/opt/harness/ze_build.cpp")
TARGETS_YML = Path("/opt/harness/targets.yml")


def tee_run(cmd, log: Path, cwd: Path) -> int:
    """Run cmd, streaming merged output to the console and to log."""
    proc = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, errors="replace")
    with log.open("w") as f:
        assert proc.stdout is not None
        for line in proc.stdout:
            f.write(line)
            f.flush()
            print(line, end="", flush=True)
    return proc.wait()

def parse_targets(targets):
    items = []
    for targetType, names in targets.items():
        for name in names:
            items.append(f'{{TargetType::{targetType}, "{name}"}}')
    return "{" + ", ".join(items) + "}"

def parse_deps(deps):
    items = [f'{{"{dep["ns"]}", "{dep["pkg"]}"}}' for dep in deps]
    return "{" + ", ".join(items) + "}"

def targets_yml_to_cpp_str():
    data = yaml.safe_load(TARGETS_YML.read_text(encoding="utf-8"))
    return ",\n    ".join(
        '{"%s", "%s", %s, %s, "%s"}' % (
            name,
            pkg.get("args", ""),
            parse_targets(pkg["targets"]),
            parse_deps(pkg.get("deps", [])),
            pkg.get("hints", ""))
        for name, pkg in data["packages"].items())

def prepare_ze_build():
    text = ZE_BUILD.read_text(encoding="utf-8")
    newText = re.sub(
        r"const std::vector<Package> packages = {};",
        lambda _: f"const std::vector<Package> packages = {{{targets_yml_to_cpp_str()}}};",
        text, count=1, flags=re.S)
    ZE_BUILD.write_text(newText, encoding="utf-8")


def main() -> int:
    if BUILD.exists():
        shutil.rmtree(BUILD)
    BUILD.mkdir(parents=True)

    prepare_ze_build()

    compileCmd = ["g++", str(ZE_BUILD), "-std=c++26", "-g",
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
