"""Generate CLion's code model from the real PlatformIO build configuration."""
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
env = dict(os.environ, PYTHONUTF8='1', PYTHONIOENCODING='utf-8')
result = subprocess.run(
    [sys.executable, '-m', 'platformio', 'project', 'metadata', '-e', 'aeropad', '--json-output'],
    cwd=root, env=env, capture_output=True, text=True, encoding='utf-8',
)
if result.returncode:
    print(result.stdout, result.stderr)
    raise SystemExit(result.returncode)
data = json.loads(result.stdout)['aeropad']
includes = []
# The metadata's toolchain scope also includes the RISC-V ULP compiler.
# Let CMake discover built-in Xtensa headers from the selected compiler.
includes.extend(data.get('includes', {}).get('build', []))
raw_flags = data.get('cxx_flags', [])
flags = shlex.split(raw_flags) if isinstance(raw_flags, str) else list(raw_flags)
for i, flag in enumerate(flags[:-1]):
    if flag == '-include' and not Path(flags[i + 1]).is_absolute():
        flags[i + 1] = (root / flags[i + 1]).as_posix()
def cmake_list(name, values):
    return 'set(' + name + '\n' + ''.join(
        '  "' + str(x).replace('\\', '/').replace('"', '\\"').replace(';', '\\;') + '"\n'
        for x in dict.fromkeys(values)
    ) + ')\n'
Path(sys.argv[1]).write_text(
    cmake_list('PIO_INCLUDES', includes)
    + cmake_list('PIO_DEFINES', data.get('defines', []))
    + cmake_list('PIO_CXX_FLAGS', flags), encoding='utf-8',
)
print('CLion code model generated from PlatformIO')
