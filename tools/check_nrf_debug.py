"""Refresh three repository screenshots from COM11; keeps control settings unchanged."""
import time,sys
from pathlib import Path
import serial
from PIL import Image
root=Path(__file__).resolve().parents[1];out=root/'logs/nrf_debug';out.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,str(Path.home()/'.platformio/packages/tool-esptoolpy'))
from esptool.reset import HardReset
with serial.Serial('COM11',115200,timeout=3) as port:
 port.set_buffer_size(rx_size=1048576);port.dtr=False
 def boot():
  port.reset_input_buffer();HardReset(port)();deadline=time.monotonic()+15
  while time.monotonic()<deadline:
   if b'Ready: main menu' in port.readline():return
  raise RuntimeError('No startup confirmation')
 def command(c):port.write(c.encode());time.sleep(.18)
 def frame(name):
  time.sleep(.8)
  port.reset_input_buffer();port.write(b'S')
  for _ in range(30):
   if b'FRAME240x536' in port.readline():break
  else:raise RuntimeError('No framebuffer header')
  raw=port.read(240*536*2)
  if len(raw)!=240*536*2:raise RuntimeError('Incomplete framebuffer')
  words=[int.from_bytes(raw[i:i+2],'big') for i in range(0,len(raw),2)]
  im=Image.new('RGB',(240,536));im.putdata([((v>>11)*255//31,((v>>5)&63)*255//63,(v&31)*255//31) for v in words]);im.save(out/name);print(name,flush=True)
 boot()
 for c in 'ERRE':command(c)
 # Force the original NDG1 profile if a previous interop test saved the raw preset.
 for c in 'BBB':command(c)
 command('U');command('E')
 # If U selected the reference profile, select NDG1 based on reported mode.
 port.reset_input_buffer();command('T')
 line=port.readline().decode(errors='replace')
 if 'mode=1' in line:command('U');command('E')
 command('B')
 frame('stats.png')
 def status():
  port.reset_input_buffer();command('T')
  for _ in range(12):
   line=port.readline().decode(errors='replace').strip()
   if '[NRFDBG]' in line:print(line,flush=True);return line
  raise RuntimeError('No NRF status')
 assert 'present=1 running=0 page=0' in status()
 command('E');time.sleep(1.2);line=status();assert 'running=1' in line
 command('E');assert 'running=0' in status()
 frame('tested.png');command('B');frame('parameters.png')
 command('R');assert 'channel=77' in status()
 command('L');assert 'channel=76' in status()
 command('D');command('R');assert 'rate=1' in status();command('L')
 command('D');command('R');assert 'power=1' in status();command('L')
 command('D');command('R');assert 'role=1' in status();command('L')
 command('D');command('R');assert 'period=500' in status();command('L')
 command('B');frame('data.png');command('Q');frame('menu.png')
 command('E');assert 'present=1 running=0 page=0 channel=76' in status();command('Q');command('Q')
print('PASS: chip detection, start/stop, channel edit, 3 pages, exit/reentry')
