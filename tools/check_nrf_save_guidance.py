"""COM11 regression: per-vehicle NVS profiles, confirmation, mapping and framebuffer captures."""
import time,sys,re
from pathlib import Path
import serial
from PIL import Image
root=Path(__file__).resolve().parents[1];out=root/'logs/nrf_safety_ui';out.mkdir(parents=True,exist_ok=True)
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
   time.sleep(.15);port.reset_input_buffer();port.write(b'T');text='';deadline=time.monotonic()+3
   while time.monotonic()<deadline:
    text+=port.readline().decode(errors='replace')
    if ('[NRFPROFILE]' in text and '\n' in text.split('[NRFPROFILE]',1)[1]) or ('[NRFDIR]' in text and '\n' in text.split('[NRFDIR]',1)[1]):break
   if '[NRFPROFILE]' in text or '[NRFDIR]' in text:
    result={k:int(v) for k,v in re.findall(r'(\w+)=(\d+)',text)};result['custom']=int('[NRFPROFILE]' in text);print(result,flush=True);return result
   port.close();time.sleep(.15);port.open()
  raise RuntimeError('Incomplete status')
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
   im=Image.new('RGB',(240,536));im.putdata([((v>>11)*255//31,((v>>5)&63)*255//63,(v&31)*255//31) for v in words]);im.save(out/name);port.close();time.sleep(.15);port.open();return
  raise RuntimeError('Incomplete framebuffer')


 def hub(index):
  assert state()['custom']==1 and state()['running']==0;command('B');assert state()['page']==12
  for _ in range(index):command('D')
  command('E')
 def return_control():command('X');command('X');assert state()['page']==0
 def select_slot(number):
  for _ in range(5):
   if state()['slot']==number:return
   command('D')
  raise RuntimeError('Cannot select slot')
 def bank_action(action):
  for _ in range(3):
   if state()['action']==action:return
   command('R')
  raise RuntimeError('Cannot select action')
 boot();command('E');command('E');hub(0);command('A')
 assert state()['page']==9 and state()['action']==0
 frame('12-save-shortcut.png')
 occupied={}
 for n in range(1,6):select_slot(n);occupied[n]=state()['occupied']
 empty=next((n for n in range(5,0,-1) if not occupied[n]),None)
 assert empty,'No empty test slot; preserve existing profiles'
 select_slot(empty);bank_action(0);command('E');saved=state()
 assert saved['occupied']==1 and saved['action']==1 and saved['page']==9
 frame('13-saved-guidance.png')
 # Saving does not apply; remove only the new test slot.
 bank_action(2);command('E');command('E');assert state()['occupied']==0
 return_control();final=state();assert final['defaultslot']==0 and final['running']==0
 command('Q');command('Q')
 print('PASS: A save shortcut, empty-slot save, success advances to apply, existing default preserved, test slot removed')
