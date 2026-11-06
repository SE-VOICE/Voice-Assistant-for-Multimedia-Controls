Import("env")
import sys
import subprocess
import shutil
from pathlib import Path
env.Append(LIBS=["crypto"])
if sys.platform == "darwin":
    brew = shutil.which("brew") or next((p for p in ["/opt/homebrew/bin/brew", "/usr/local/bin/brew"] if Path(p).exists()), "brew")
    prefix = subprocess.check_output([brew, "--prefix", "openssl@3"], text=True).strip()
    env.Append(CPPPATH=[prefix + "/include"], LIBPATH=[prefix + "/lib"])
