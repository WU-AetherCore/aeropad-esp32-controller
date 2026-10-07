"""Refresh three repository screenshots from COM11; keeps control settings unchanged."""
import time,sys
from pathlib import Path
import serial
from PIL import Image
root=Path(__file__).resolve().parents[1];out=root/'logs/network_menu';out.mkdir(parents=True,exist_ok=True)
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
 for c in 'RRE':command(c)
 frame('wifi-only.png')
 command('R');frame('wifi-only-next.png')
 command('E');frame('wifi-settings.png')
 command('Q');frame('wifi-return.png')
 command('Q')
print('PASS: WiFi menu entered and exited on COM11.')
