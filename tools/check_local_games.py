import serial,time,sys
from pathlib import Path
from PIL import Image
sys.path.insert(0,str(Path.home()/'.platformio/packages/tool-esptoolpy'))
from esptool.reset import HardReset
out=Path(__file__).resolve().parents[1]/'logs/local_games';out.mkdir(exist_ok=True)
with serial.Serial('COM11',115200,timeout=2) as p:
    p.set_buffer_size(rx_size=1048576);p.dtr=False;HardReset(p)()
    deadline=time.monotonic()+15
    while time.monotonic()<deadline:
        if b'Ready: main menu' in p.readline():break
    else:raise RuntimeError('Boot failed')
    def cmd(c,wait=.15):p.write(c.encode());time.sleep(wait)
    def status():p.reset_input_buffer();cmd('T');return p.readline().decode(errors='replace')
    def frame(name):
        p.reset_input_buffer();p.write(b'S')
        for _ in range(20):
            if b'FRAME240x536' in p.readline():break
        else:raise RuntimeError('No frame')
        raw=p.read(257280);assert len(raw)==257280
        vals=[int.from_bytes(raw[i:i+2],'big') for i in range(0,len(raw),2)]
        im=Image.new('RGB',(240,536));im.putdata([((v>>11)*255//31,((v>>5)&63)*255//63,(v&31)*255//31) for v in vals]);im.save(out/(name+'.png'))
    for c in 'REE':cmd(c,.35)
    assert 'playing=0' in status();frame('snake_ready');cmd('E');cmd('A');assert 'paused=1' in status();frame('snake_pause');cmd('A');time.sleep(4);print(status());assert 'over=1' in status();frame('snake_end');cmd('E');cmd('A');assert 'over=0' in status();cmd('Q')
    cmd('R');cmd('E');assert 'launched=0' in status();frame('brick_ready');cmd('E');time.sleep(.4);cmd('A');assert 'paused=1' in status();frame('brick_pause');cmd('A');time.sleep(3);print(status());cmd('Q');cmd('Q')
print('PASS: game entries, snake start/pause/wall collision/restart, breakout launch/pause/resume, screenshots and exits.')
