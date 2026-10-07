"""COM11 integration check of real solving, animated auto-advance and saved progress."""
import serial,time,sys,re,json
from pathlib import Path
from PIL import Image
sys.path.insert(0,str(Path.home()/'.platformio/packages/tool-esptoolpy'))
from esptool.reset import HardReset
out=Path(__file__).resolve().parents[1]/'logs/sokoban';out.mkdir(exist_ok=True)
levels=json.loads((out.parents[1]/'docs/sokoban_levels.json').read_text())['levels']
with serial.Serial('COM11',115200,timeout=3) as p:
 p.set_buffer_size(rx_size=1048576);p.dtr=False
 def boot():
  p.reset_input_buffer();HardReset(p)();deadline=time.monotonic()+15
  while time.monotonic()<deadline:
   if b'Ready: main menu' in p.readline():return
  raise RuntimeError('Boot failed')
 def cmd(c,delay=.12):p.write(c.encode());time.sleep(delay)
 def status():
  p.reset_input_buffer();cmd('T');s=p.readline().decode(errors='replace');print(s);return s
 def frame(name):
  p.reset_input_buffer();p.write(b'S')
  for _ in range(20):
   if b'FRAME240x536' in p.readline():break
  else:raise RuntimeError('No frame')
  raw=p.read(257280);assert len(raw)==257280
  vals=[int.from_bytes(raw[i:i+2],'big') for i in range(0,len(raw),2)]
  im=Image.new('RGB',(240,536));im.putdata([((v>>11)*255//31,((v>>5)&63)*255//63,(v&31)*255//31) for v in vals]);im.save(out/(name+'.png'))
 def enter():
  for c in 'RERRRRR':cmd(c)
  frame('menu');cmd('E')
 boot();enter();s=status();level=int(re.search(r'level=(\d+)',s)[1]);assert 'selecting=1' in s;frame('select');cmd('E');frame('board')
 solution=levels[level-1]['solution']
 for c in solution[:4]:cmd(c)
 assert 'steps=4' in status();cmd('A');cmd('A');assert 'steps=2' in status();cmd('E')
 for c in solution:cmd(c,.035)
 assert 'won=1 celebrating=1' in status();cmd('A');assert 'won=0 celebrating=0' in status();cmd(solution[-1]);assert 'won=1 celebrating=1' in status();frame('celebration')
 if level<10:
  deadline=time.monotonic()+6
  while time.monotonic()<deadline:
   s=status()
   if f'level={level+1} ' in s:break
   time.sleep(.2)
  else:raise RuntimeError('Automatic next level did not occur')
  assert 'steps=0' in s and 'selecting=0' in s;frame('next')
  for stage in range(level+1,11):
   if stage in (5,10):frame(f'level_{stage:02}')
   for c in levels[stage-1]['solution']:cmd(c,.035)
   s=status();assert f'level={stage} ' in s and 'won=1 celebrating=1' in s
   if stage<10:
    deadline=time.monotonic()+6
    while time.monotonic()<deadline:
     s=status()
     if f'level={stage+1} ' in s:break
     time.sleep(.2)
    else:raise RuntimeError(f'No automatic transition after stage {stage}')
   else:
    frame('final');frame('final_motion')
    assert Image.open(out/'final.png').tobytes()!=Image.open(out/'final_motion.png').tobytes(),'Animation did not change'
    time.sleep(3.3);assert 'level=10 ' in status() and 'won=1' in status()
    cmd('E');assert 'level=1 steps=0' in status();cmd('B');assert 'selecting=1' in status();frame('replay_select');cmd('E')
 cmd('Q');cmd('Q');boot();enter();assert 'level=1 ' in status() and 'unlocked=10' in status();cmd('Q');cmd('Q')
print('PASS: ten actual-device solutions, animated frame changes, nine automatic transitions, final stop/replay, undo/cancel/restart/select/return/reboot progress and framebuffer capture.')
