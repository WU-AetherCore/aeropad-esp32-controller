"""Refresh three repository screenshots from COM11; keeps control settings unchanged."""
import time,sys
from pathlib import Path
import serial
from PIL import Image
root=Path(__file__).resolve().parents[1];out=root/'logs/nrf_bugfix';out.mkdir(parents=True,exist_ok=True)
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
 command('B');command('R') # CH3 deliberately has no peer.
 command('B');command('B') # Timing page.
 for c in 'DLDLLLLDLL':command(c) # delay15/count15/interval20ms
 frame('retry-stress-config.png')
 for c in 'BBBBBB':command(c) # Stats page.
 command('E');time.sleep(.3)
 latencies=[];page=0
 def fast_status():
  port.reset_input_buffer();port.write(b'T');deadline=time.monotonic()+1
  while time.monotonic()<deadline:
   line=port.readline().decode(errors='replace').strip()
   if '[NRFRAW]' in line:return line
  raise RuntimeError('NRF UI failed to respond')
 for i in range(27):
  start=time.monotonic();port.write(b'B');time.sleep(.035);line=fast_status();latencies.append(time.monotonic()-start);page=(page+1)%9
  assert 'page=%d '%page in line and 'running=1' in line,line
 assert max(latencies)<.25,latencies
 print('27 page changes during failed retries; max roundtrip=%.1fms'%(max(latencies)*1000),flush=True)
 command('E');assert 'running=0' in status()
 for c in 'BBBBBBB':command(c)
 command('E');command('B');command('B') # Restore CH2 reference.
 command('E');time.sleep(2);command('E');line=status();assert 'OK=0 ' not in line and 'RX=0 ' not in line,line
 frame('framed-stats.png');command('B');frame('framed-parameters.png');command('Q');command('Q')
 # Original diagnostic page now has both test protocols.
 boot()
 for c in 'ERRE':command(c)
 def dbg_status():
  port.reset_input_buffer();command('T')
  for _ in range(12):
   line=port.readline().decode(errors='replace').strip()
   if '[NRFDBG]' in line:print(line,flush=True);return line
  raise RuntimeError('No debug status')
 for c in 'BBB':command(c)
 line=dbg_status();frame('debug-presets.png')
 if 'mode=0' in line:command('D')
 command('E');assert 'mode=1' in dbg_status()
 command('B');command('E');time.sleep(2);line=dbg_status();assert 'ACK=0 ' not in line and 'RX=0 ' not in line,line
 frame('debug-reference-stats.png');command('B');frame('debug-reference-config.png');command('B');frame('debug-reference-data.png')
 command('E');assert 'running=0' in dbg_status();command('Q');command('E');assert 'mode=1' in dbg_status() and 'running=0' in dbg_status()
 command('Q');command('Q')
 Path(root/'logs/nrf_bugfix/result.txt').write_text('PASS: 27/27 page changes under maximum retries; max=%.1fms; raw and diagnostic reference preset RF roundtrip verified; stopped.\n'%(max(latencies)*1000),encoding='utf-8')
print('PASS: framed UI, retry stress, original debug Jiangxie preset, persistence.')
