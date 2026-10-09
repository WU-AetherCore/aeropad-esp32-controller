"""Real WHEELTEC-IOS BLE UART link: centered traffic and stop, no injected movement."""
import time,sys,re
from pathlib import Path
import serial
from PIL import Image
root=Path(__file__).resolve().parents[1];out=root/'logs/ble_protocol_ui';out.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,str(Path.home()/'.platformio/packages/tool-esptoolpy'))
from esptool.reset import HardReset
with serial.Serial('COM11',115200,timeout=1) as port:
 port.set_buffer_size(rx_size=1048576)
 def command(c):
  port.write(c.encode());time.sleep(.22)
 def boot():
  port.dtr=False;port.reset_input_buffer();HardReset(port)();deadline=time.monotonic()+20
  while time.monotonic()<deadline:
   if b'Ready: main menu' in port.readline():port.dtr=True;return
  raise RuntimeError('No startup')
 def enter(mode):
  for c in ('EEBBBE' if mode==1 else 'EREBBBE'):command(c)
 def state():
  for attempt in range(3):
   time.sleep(.15);port.reset_input_buffer();port.write(b'T');deadline=time.monotonic()+3;text=''
   while time.monotonic()<deadline:
    line=port.readline().decode(errors='replace');text+=line
    if '[BLEPROTO]' in text and re.search(r'feedbackAge=\d+',text):
     line=text[text.rfind('[BLEPROTO]'):]
     result={k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',line)};print(result,flush=True);return result
   port.close();time.sleep(.15);port.open()
  raise RuntimeError('No BLEPROTO status: '+text)
 def frame(name):
  for attempt in range(3):
   time.sleep(.3);port.reset_input_buffer();port.write(b'S');deadline=time.monotonic()+3
   while time.monotonic()<deadline:
    if b'FRAME240x536' in port.readline():break
   else:continue
   raw=bytearray();deadline=time.monotonic()+15
   while len(raw)<257280 and time.monotonic()<deadline:raw+=port.read(min(8192,257280-len(raw)))
   if len(raw)!=257280:
    print('Retry framebuffer:',len(raw),flush=True);continue
   words=[int.from_bytes(raw[i:i+2],'big') for i in range(0,len(raw),2)]
   im=Image.new('RGB',(240,536));im.putdata([((v>>11)*255//31,((v>>5)&63)*255//63,(v&31)*255//31) for v in words]);im.save(out/name)
   if name!='01-first-menu-icon.png':
    bright=sum(min(pixel)>200 for pixel in im.crop((10,8,230,44)).getdata())
    assert bright>500,('Incomplete title',name,bright)
   return
  raise RuntimeError('Incomplete framebuffer')



 boot()
 for c in 'RRREE':command(c)
 command('D');command('E');time.sleep(6)
 port.reset_input_buffer();port.write(b'T');time.sleep(.4);listing=port.read_all().decode(errors='replace');print(listing,flush=True);(out/'wheeltec-scan.txt').write_text(listing,encoding='utf-8')
 matches=re.findall(r'\[DEVICE (\d+)\] ([0-9a-f:]+) WHEELTEC-IOS RSSI=.* UART=1',listing,re.I)
 assert len(matches)==1,'WHEELTEC-IOS not uniquely identified'
 index=int(matches[0][0])
 for _ in range(index):command('D')
 command('E');deadline=time.monotonic()+25
 while time.monotonic()<deadline:
  status=state()
  if status['state']==4:break
  assert status['state'] not in (5,6),'BLE connection failed'
  time.sleep(.4)
 assert status['state']==4
 frame('07-wheeltec-connected.png');command('Q');command('E');assert state()['page']==1
 # Validate live interlock and a zero-speed positive start/stop on the same device.
 command('E');time.sleep(.3);checked=state();assert checked['state']==4 and checked['ERR']==0
 if not checked['ready']:
  assert checked['streaming']==0 and checked['TX']==0
  frame('17-speed-knob-warning.png')
 else:
  assert checked['streaming']==1
  command('A');time.sleep(.5);assert state()['streaming']==0
 command('Q')
 # Never edit persistent speed or knob settings for a connectivity test.
 command('E');time.sleep(.8);running=state()
 assert running['ready']==1 and running['streaming']==1 and running['target']==0 and running['ERR']==0
 try:
  for _ in range(6):
   time.sleep(10);soak=state();assert soak['state']==4 and soak['streaming']==1 and soak['ERR']==0 and soak['target']==0
  print('PASS: 60-second real C30D centered-input soak; configuration unchanged',flush=True)
 finally:
  command('A');time.sleep(.5)
 stopped=state();time.sleep(.4);still=state()
 assert still['streaming']==0 and still['TX']==stopped['TX'] and still['ERR']==0
 command('Q');command('Q');command('Q')
 print('PASS: stop confirmed; no persistent configuration was modified')
