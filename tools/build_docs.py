"""Doxygen XML와 Sphinx/Furo 사이트를 warning=error로 생성하고 API 계약을 검사한다."""
from pathlib import Path
import argparse
import os
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]


def check_api(xml_dir):
    count = 0
    for path in xml_dir.glob("*_8h.xml"):
        tree = ET.parse(path)
        for function in tree.findall(".//memberdef[@kind='function']"):
            name = function.findtext("name")
            brief = "".join(function.find("briefdescription").itertext()).strip()
            parameters = {node.text for node in function.findall("param/declname")}
            documented = {node.text for node in function.findall(
                ".//parameterlist[@kind='param']/parameteritem/parameternamelist/parametername")}
            returns = function.find(".//simplesect[@kind='return']")
            if not brief or parameters != documented or returns is None:
                raise RuntimeError(f"API 계약 누락: {name}: {parameters - documented}")
            count += 1
    if count != 14:
        raise RuntimeError(f"API 추출 개수 변경: {count}; 기대 목록과 함께 검토")
    print(f"PASS: {count} public API briefs/parameters/returns")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--doxygen", default="doxygen")
    args = parser.parse_args()
    version = subprocess.check_output([args.doxygen, "--version"], text=True).strip()
    if version.split()[0] != "1.18.0":
        raise RuntimeError(f"Doxygen 1.18.0 required, found {version}")
    (ROOT / "build/api/doxygen").mkdir(parents=True, exist_ok=True)
    subprocess.run([args.doxygen, "docs/api/Doxyfile"], cwd=ROOT, check=True)
    check_api(ROOT / "build/api/doxygen/xml")
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE="1")
    subprocess.run([sys.executable, "-m", "sphinx", "-b", "html", "-n", "-W",
                    "--keep-going", "docs/api", "build/api/html"],
                   cwd=ROOT, env=environment, check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
