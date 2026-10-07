"""Refresh three repository screenshots from COM11; keeps control settings unchanged."""
import time,sys
from pathlib import Path
import serial
from PIL import Image
root=Path(__file__).resolve().parents[1];out=root/'logs/nrf_polish';out.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,str(Path.home()/'.platformio/packages/tool-esptoolpy'))
from esptool.reset import HardReset
with serial.Serial('COM11',115200,timeout=3) as port:
 port.set_buffer_size(rx_size=1048576);port.dtr=False
 def boot():
  port.dtr=False;port.reset_input_buffer();HardReset(port)();deadline=time.monotonic()+15
  while time.monotonic()<deadline:
   if b'Ready: main menu' in port.readline():port.dtr=True;return
  raise RuntimeError('No startup confirmation')
 def command(c):port.write(c.encode());time.sleep(.18)
 def frame(name,attempt=0):
  time.sleep(.8)
  port.reset_input_buffer();port.write(b'S')
  for _ in range(30):
   if b'FRAME240x536' in port.readline():break
  else:raise RuntimeError('No framebuffer header')
  port.timeout=1;parts=[];total=0;deadline=time.monotonic()+20
  while total<240*536*2 and time.monotonic()<deadline:
   chunk=port.read(min(8192,240*536*2-total));parts.append(chunk);total+=len(chunk)
  raw=b''.join(parts);port.timeout=3
  if len(raw)!=240*536*2:
   if attempt<2:
    print('Retrying incomplete USB frame',len(raw),flush=True);time.sleep(.5);return frame(name,attempt+1)
   raise RuntimeError('Incomplete framebuffer: %d bytes'%len(raw))
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
 import re
 boot();enter()
 for c in 'BBBBBBBEBB':command(c)
 frame('stopped.png')
 for page in range(9):
  if page:command('B')
  frame('page%d.png'%page)
  if page==6:
   command('A');frame('auto-readonly.png');command('R');assert 'running=0' in status();command('A')
 command('B')
 def counts():
  line=status();m=re.search(r'TX=(\d+)',line);return int(m.group(1)),line
 for target in range(9):
  # Enter running state from statistics, then navigate without changing config.
  command('E');time.sleep(.15)
  for _ in range(target):command('B')
  command('E');n,line=counts();assert 'running=0' in line,line
  time.sleep(.3);n2,line=counts();assert n2==n,(n,n2,line)
  for _ in range((9-target)%9):command('B')
 print('PASS: 9/9 pages stopped with one action; TX counter remained frozen.',flush=True)
 command('E');frame('sending-green.png');command('E');frame('stopped-gray.png')
 command('Q');command('Q');boot()
 for c in 'ERREBBB':command(c)
 def dbg():
  port.reset_input_buffer();command('T')
  for _ in range(12):
   line=port.readline().decode(errors='replace').strip()
   if '[NRFDBG]' in line:return line
  raise RuntimeError('No diagnostic status')
 if 'mode=0' in dbg():command('D')
 command('E');command('B');command('B');frame('debug-editable.png')
 for c in 'DDD':command(c)
 frame('debug-skips-readonly-address.png')
 command('B');frame('debug-readonly-data.png')
 command('B');command('B');command('E');time.sleep(.8)
 command('B');command('B');command('B');command('E');line=dbg();assert 'running=0' in line
 n=int(re.search(r'TX=(\d+)',line).group(1));time.sleep(.3);assert int(re.search(r'TX=(\d+)',dbg()).group(1))==n
 command('Q');command('Q')
print('PASS: diagnostic readonly navigation, preset-page stop, returned stopped.')
