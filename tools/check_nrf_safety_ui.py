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
 boot()
 for c in 'EE':command(c)
 initial=state()
 if initial['custom']==0:
  command('B');command('B');assert state()['page']==3;command('D');command('E');assert state()['custom']==1 and state()['page']==9;select_slot(1);bank_action(1);command('E');initial=state()
 assert initial['custom']==1 and initial['applied']==1 and initial['defaultslot']==0 and initial['running']==0
 frame('01-default-controller.png')
 # Real RF stream to the existing four-byte echo peer; no flight/motor actuators.
 command('E');time.sleep(.8);before=state();assert before['running']==1 and before['TX']>0
 command('B');after=state();assert after['page']==0 and after['running']==1 and after['TX']>before['TX']
 for c in 'UDLRX':command(c)
 after=state();assert after['page']==0 and after['running']==1 and after['ch']==initial['ch'] and after['source']==initial['source']
 port.write(b'@DE AD\n');time.sleep(.35);assert state()['running']==1
 port.reset_input_buffer();port.write(b'S');text=port.read(4096);assert b'FRAME240x536' not in text and b'[SCREEN] busy' in text
 command('A');stopped=state();time.sleep(.4);assert state()['running']==0 and state()['TX']==stopped['TX']
 hub(0);assert state()['page']==1;command('R');assert state()['ch']!=(initial['ch']);time.sleep(1);frame('02-draft-not-applied.png')
 return_control();assert state()['ch']==initial['ch'] and state()['running']==0
 # Restore the auto-saved draft to its original channel without altering the applied snapshot.
 hub(0);command('R');command('L');return_control()
 hub(3);assert state()['page']==9;frame('03-presets.png');select_slot(1);bank_action(1);command('E')
 applied=state();assert applied['page']==0 and applied['applied']==1 and applied['defaultslot']==0 and applied['running']==0
 frame('04-applied-controller.png');boot()
 for c in 'EE':command(c)
 assert state()['applied']==1 and state()['defaultslot']==0 and state()['ch']==initial['ch'] and state()['running']==0
 frame('05-reboot-default.png')
 hub(4);assert state()['page']==11;frame('06-directions.png');command('X');assert state()['page']==12;frame('07-setup-menu.png');command('X')
 # Only use a genuinely empty slot and restore default preset1 before leaving.
 hub(3);occupied={}
 for number in range(1,6):select_slot(number);occupied[number]=state()['occupied']
 empty=next((i for i in range(5,0,-1) if not occupied[i]),None)
 if empty:
  select_slot(empty);bank_action(0);command('E');assert state()['occupied']==1
  bank_action(1);command('E');assert state()['defaultslot']==empty-1
  hub(3);select_slot(empty);bank_action(2);command('E');assert state()['confirm']==1;frame('08-delete-confirm.png');command('E');assert state()['occupied']==0
  return_control();assert state()['custom']==0 and state()['armed']==0
  command('B');command('B');assert state()['page']==3;frame('09-standard-setup.png');command('D');command('E')
  assert state()['custom']==1 and state()['page']==9;select_slot(1);bank_action(1);command('E');assert state()['applied']==1 and state()['defaultslot']==0
 command('Q');command('Q')
print('PASS: active stream rejects setup/nav/payload/capture, A stop, draft isolation, durable preset default, context return, empty-slot delete fallback and original preset1 restored')
