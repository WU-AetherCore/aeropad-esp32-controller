"""Refresh three repository screenshots from COM11; keeps control settings unchanged."""
import time,sys
from pathlib import Path
import serial
from PIL import Image
root=Path(__file__).resolve().parents[1];out=root/'logs/nrf_generic';out.mkdir(parents=True,exist_ok=True)
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
 def status():
  port.reset_input_buffer();command('T')
  for _ in range(12):
   line=port.readline().decode(errors='replace').strip()
   if '[NRFRAW]' in line:print(line,flush=True);return line
  raise RuntimeError('No NRF status')
 def enter():
  for c in 'ERRRE':command(c)
 boot();enter();frame('stats.png')
 assert 'running=0 page=0 ch=2 rate=1 width=5 crc=1 ack=1 dynamic=0 rxlen=4 txlen=4' in status()
 for i in range(1,9):command('B');frame('page%d.png'%i)
 command('B');command('B');command('R');time.sleep(1);assert 'ch=3' in status() and 'saved=1' in status()
 command('Q');command('Q');boot();enter();assert 'ch=3' in status() and 'running=0' in status()
 command('B');command('L');time.sleep(1);assert 'ch=2' in status()
 for c in 'BBBBBB':command(c)
 command('E');time.sleep(1);assert 'txlen=4' in status()
 command('Q');command('Q')
print('PASS: 9 pages, preset and NVS reset persistence.')
