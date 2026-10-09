"""COM11 BLE protocol workspace regression: settings, five slots, restart and UI."""
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
    if '[BLEPROTO]' in line:
     result={k:int(v) for k,v in re.findall(r'(\w+)=(\d+)',line)};print(result,flush=True);return result
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
   port.close();time.sleep(.15);port.open();return
  raise RuntimeError('Incomplete framebuffer')



 boot()
 # Root fourth item = Bluetooth center. Its new first menu entry is protocol control.
 for c in 'RRRE':command(c)
 frame('01-first-menu-icon.png');command('E');initial=state();assert initial['page']==0
 if initial['defaultslot']==4:
  for c in 'DDDDEDDD':command(c)
  command('D');command('R');command('R');command('E');command('E');command('Q')
  # Reset draft using built-in preset after cleaning an interrupted fixture.
  for c in 'DDDDDE':command(c)
  command('E');command('B')
 # Always reset this new fixture's draft before comparing the +25 edit.
 for c in 'DDDDDE':command(c)
 frame('15-single-c30d.png');command('E');command('B')
 frame('02-protocol-center.png')
 command('D');command('E');time.sleep(6);assert state()['page']==2
 frame('03-nearby-devices.png');state();command('Q')
 # Edit speed draft and open save via A. Use only a previously empty slot.
 command('D');command('D');command('E');assert state()['page']==3
 command('L');time.sleep(1);assert state()['speed']==3450 and state()['saved']==1
 command('D');command('R');assert state()['angular']==1500
 time.sleep(2);frame('04-parameters.png');command('B');assert state()['page']==7
 frame('09-directions.png');command('B');
 for _ in range(6):command('D')
 assert state()['field']==6
 frame('14-knob-settings.png');command('A');assert state()['page']==5
 for _ in range(5):
  if state()['slot']==5:break
  command('D')
 assert state()['slot']==5 and state()['occupied']==0
 command('E');assert state()['occupied']==1 and state()['action']==1
 frame('05-saved-preset.png');command('E');applied=state();assert applied['page']==1 and applied['defaultslot']==4 and applied['streaming']==0
 frame('06-control.png');boot()
 for c in 'RRREE':command(c)
 restored=state();assert restored['defaultslot']==4 and restored['speed']==3450 and restored['streaming']==0
 # Clear only our new slot. Default falls back to built-in C30D APP, stopped.
 for c in 'DDDDE':command(c)
 for _ in range(4):command('D')
 command('R');command('R');command('E');command('E');deleted=state();assert deleted['occupied']==0 and deleted['defaultslot']==255
 command('Q')
 # All custom editor screens, without connecting or starting movement.
 for c in 'DDDDDED':command(c)
 command('E');command('B')
 for c in 'DDDE':command(c)
 assert state()['page']==4 and state()['kind']==3
 frame('10-byte-mapping.png');command('L');assert state()['map0']==0
 command('B');command('R');assert state()['page']==8 and state()['byte0']==1
 frame('11-constants.png');command('B');assert state()['page']==9
 frame('12-stop-frame.png');command('Q')
 # Restore the supplied C30D APP default and its draft before leaving.
 for c in 'DDDDDE':command(c)
 command('E');command('Q');command('Q');command('Q')
 print('PASS: first menu icon, draft autosave, empty-slot save/apply, power-cycle default restore, delete fallback; stopped throughout')
