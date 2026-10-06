from pathlib import Path
import hashlib
Import('env')
# SCons does not reliably discover headers forced in with -include. Include
# the configuration digest in compiler flags so edits rebuild affected objects.
config = Path(env.subst('$PROJECT_DIR')) / 'include/tft_sprite_setup.h'
revision = '0x' + hashlib.sha256(config.read_bytes()).hexdigest()[:8]
env.Append(CPPDEFINES=[('AEROPAD_TFT_CONFIG_REV', revision)])
