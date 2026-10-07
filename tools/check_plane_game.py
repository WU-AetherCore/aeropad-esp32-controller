import serial,time,sys
from pathlib import Path
from PIL import Image
sys.path.insert(0,str(Path.home()/'.platformio/packages/tool-esptoolpy'))
from esptool.reset import HardReset
out=Path(__file__).resolve().parents[1]/'logs/plane_game';out.mkdir(exist_ok=True)
with serial.Serial('COM11',115200,timeout=2) as p:
    p.set_buffer_size(rx_size=1048576);p.dtr=False;HardReset(p)()
    deadline=time.monotonic()+15
    while time.monotonic()<deadline:
        if b'Ready: main menu' in p.readline():break
    else:raise RuntimeError('Boot failed')
    def cmd(c):p.write(c.encode());time.sleep(.25)
    def status():p.reset_input_buffer();cmd('T');s=p.readline().decode(errors='replace');print(s);return s
    def frame(name):
        p.reset_input_buffer();p.write(b'S')
        for _ in range(20):
            if b'FRAME240x536' in p.readline():break
        else:raise RuntimeError('No frame')
        raw=p.read(257280);assert len(raw)==257280
        vals=[int.from_bytes(raw[i:i+2],'big') for i in range(0,len(raw),2)]
        im=Image.new('RGB',(240,536));im.putdata([((v>>11)*255//31,((v>>5)&63)*255//63,(v&31)*255//31) for v in vals]);im.save(out/(name+'.png'))
    for c in 'RERRE':cmd(c)
    assert 'playing=0' in status();frame('ready');cmd('E');time.sleep(2);cmd('A');assert 'paused=1' in status();frame('paused');cmd('A');cmd('B');assert 'bombs=2' in status();time.sleep(2);cmd('A');frame('action');cmd('Q');cmd('Q')
print('PASS: plane entry/start/pause/resume/bomb/screen capture/return.')
