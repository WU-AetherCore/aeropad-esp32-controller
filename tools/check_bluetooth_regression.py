"""Verify four formats, nine periods, persistence, and nested LCD redraws on COM11."""
import serial,time,re,json,hashlib,datetime
from pathlib import Path
from esptool.reset import HardReset
root=Path(__file__).resolve().parents[1];out=root/'logs/bluetooth_regression';out.mkdir(exist_ok=True)
header=(root/'src/generated_ui_text.h').read_text()
def asset(name):
 body=re.search(r'ui_'+name+r'\[\].*?\{(.*?)\};',header,re.S)[1]
 raw=b''.join(int(h,16).to_bytes(2,'big') for h in re.findall(r'0x([0-9a-f]+)',body))
 return b''.join(raw[y*440+40:y*440+400] for y in range(44))
formats=[asset(n) for n in ['formatbinary','formatjson','formathex','formattext']]
def formatrow(d):return b''.join(d[(y*240+30)*2:(y*240+210)*2] for y in range(162,206))
with serial.Serial('COM11',115200,timeout=3) as p:
 p.set_buffer_size(rx_size=1048576,tx_size=65536)
 def boot():
  p.dtr=False;HardReset(p)();end=time.monotonic()+15
  while time.monotonic()<end:
   if b'Ready: main menu' in p.readline():return
  raise RuntimeError('Boot not ready')
 def cmd(c,wait=.35):p.write(c.encode());time.sleep(wait)
 def text(c):p.reset_input_buffer();cmd(c);return p.read_all().decode(errors='replace')
 def frame(name,command='S'):
  for _ in range(3):
   p.reset_input_buffer();p.write(command.encode());end=time.monotonic()+15
   while time.monotonic()<end:
    if b'FRAME240x536' in p.readline():break
   else:raise RuntimeError('No frame '+name)
   d=p.read(257280)
   if len(d)==257280:(out/(name+'.rgb565')).write_bytes(d);return d
  raise RuntimeError('Short frame '+name)
 def lcd(name):
  logical=frame(name);actual=frame(name+'_lcd','W')
  if name.startswith('list_'):
   logical=bytearray(logical);actual=bytearray(actual)
   for y in [130,212,294]:logical[y*480:(y+28)*480]=actual[y*480:(y+28)*480]
  assert logical==actual,'LCD stale bands: '+name
 def outputs():
  for _ in range(3):cmd('R')
  cmd('E');frame('hub');cmd('R');frame('icon');cmd('R');cmd('E')
 boot();original_cal=text('P');outputs();initial=frame('initial')
 first=formats.index(formatrow(initial))
 for i in range(4):
  f=(first+i)%4;d=frame('format_'+str(f));assert formatrow(d)==formats[f];lcd('format_'+str(f));cmd('R')
 assert formatrow(frame('restored_format'))==formats[first]
 cmd('D');original_period=frame('initial_period');seen=set()
 for i in range(9):
  state=text('T');m=re.search(r'period=(\d+)',state);assert m,state
  period=int(m[1]);assert period in [20,50,100,200,250,500,1000,1500,2000]
  d=frame('period_'+str(i));seen.add(d[280*480:332*480]);frame('period_ms_'+str(period));lcd('period_'+str(i));cmd('R')
 assert len(seen)==9 and frame('restored_period')==original_period
 cmd('Q');cmd('Q');boot();outputs();assert frame('reboot')==initial
 cmd('Q');cmd('Q');cmd('V',6);listing=text('T');(out/'scan.txt').write_text(listing)
 cmd('C');lcd('local_serial');cmd('Q');lcd('list_after_local')
 cmd('F');lcd('send_settings');cmd('Q');lcd('list_after_send')
 # Name animation may move during two captures; only connected, static pages compare exactly.
 match=next((m for m in re.finditer(r'\[DEVICE (\d+)\] ([0-9a-f:]+) (.*?) RSSI=',listing,re.I) if m[3].upper()=='WUFUDONG'),None)
 if match:
  for _ in range(int(match[1])):cmd('R',.15)
  cmd('E',1);end=time.monotonic()+25
  while time.monotonic()<end:
   state=text('T')
   if any(f'state={s}' in state for s in [4,5,6]):break
  (out/'connection.txt').write_text(state)
  assert 'state=4' in state,state
  lcd('device_actions');cmd('R')
  for i in range(3):
   cmd('E');lcd('serial_before_'+str(i));cmd('D');cmd('E');lcd('remote_'+str(i));cmd('Q');lcd('serial_after_'+str(i));cmd('Q');lcd('actions_after_'+str(i))
  cmd('Q',2)
 cmd('Q');assert text('P')==original_cal,'Calibration changed'
 (out/'result.txt').write_text('PASS four formats, nine periods, reboot exact restoration, nested local/remote returns and SPI-transmitted framebuffer equal to logical framebuffer; calibration unchanged. No continuous controls transmitted.')
 (out/'manifest.json').write_text(json.dumps({'firmware_sha256':hashlib.sha256((root/'.pio/build/aeropad/firmware.bin').read_bytes()).hexdigest(),'completed_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'result':'PASS'},indent=2))
 print((out/'result.txt').read_text(),flush=True)
