"""Refresh three repository screenshots from COM11; keeps control settings unchanged."""
import time,sys
from pathlib import Path
import serial
from PIL import Image
root=Path(__file__).resolve().parents[1];out=root/'logs/nrf_interop';out.mkdir(parents=True,exist_ok=True)
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
 boot();enter()
 for c in 'BBBBBBB':command(c)
 command('E');command('B');command('B')
 assert 'ch=2 rate=1 width=5 crc=1 ack=1 dynamic=0 rxlen=4 txlen=4' in status()
 def exchange(payload,expected):
  port.write(('@'+payload+'\n').encode());time.sleep(1);port.reset_input_buffer();command('E')
  deadline=time.monotonic()+4;matched=0;lines=[]
  while time.monotonic()<deadline:
   line=port.readline().decode(errors='replace').strip()
   if line:lines.append(line)
   if expected in line:matched+=1
  command('E');last=status();print('echo matches',matched,flush=True)
  assert matched>=3,(expected,lines,last)
  assert 'RX=0 ' not in last,last
  return matched
 exchange('41 54 31 21','[NRFRAW RX 4] 41 54 31 21')
 frame('hex-large.png');command('R');frame('text-large.png')
 for c in 'BBBBBBBB':command(c)
 frame('receive-page.png');command('L');frame('receive-hex.png')
 command('B')
 exchange('12 34 AB CD','[NRFRAW RX 4] 12 34 AB CD')
 Path(root/'logs/nrf_interop/evidence.txt').write_text(status(),encoding='utf-8')
 for c in 'BBBBBBB':command(c)
 command('E');command('B');command('B');frame('final-stopped.png')
 command('Q');command('Q')
print('PASS: two distinct payloads echoed over RF by STM32; returned stopped.')
