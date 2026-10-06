import serial,time,sys
import statistics
from pathlib import Path
from PIL import Image
sys.path.insert(0,str(Path.home()/'.platformio/packages/tool-esptoolpy'))
from esptool.reset import HardReset
out=Path(__file__).resolve().parents[1]/'logs/device_monitor';out.mkdir(exist_ok=True)
with serial.Serial('COM11',115200,timeout=3) as p:
    p.set_buffer_size(rx_size=1048576);p.dtr=False;HardReset(p)()
    deadline=time.monotonic()+15
    while time.monotonic()<deadline:
        if b'Ready: main menu' in p.readline():break
    else:raise RuntimeError('Boot not ready')
    def cmd(c):p.write(c.encode());time.sleep(.4)
    for c in 'RRRRERRRE':cmd(c)
    p.read_all();latencies=[]
    for i in range(24):
        p.reset_input_buffer();start=time.monotonic();p.write(b'R')
        deadline=start+2
        while time.monotonic()<deadline:
            line=p.readline()
            if b'[MONITOR PAGE]' in line:break
        else:raise RuntimeError('Page switching did not respond')
        latencies.append((time.monotonic()-start)*1000)
    p.write(b'T');time.sleep(.2);print(p.read_all().decode(errors='replace'))
    print(f'Switch response ms: median={statistics.median(latencies):.1f} max={max(latencies):.1f}')
    images=[]
    for page in range(6):
        p.reset_input_buffer();p.write(b'S')
        for _ in range(20):
            if b'FRAME240x536' in p.readline():break
        else:raise RuntimeError('No frame')
        raw=p.read(257280);assert len(raw)==257280,len(raw)
        values=[int.from_bytes(raw[i:i+2],'big') for i in range(0,len(raw),2)]
        im=Image.new('RGB',(240,536));im.putdata([((v>>11)*255//31,((v>>5)&63)*255//63,(v&31)*255//31) for v in values]);im.save(out/f'{page+1}.png');images.append(im);cmd('R')
    montage=Image.new('RGB',(1440,536))
    for i,im in enumerate(images):montage.paste(im,(i*240,0))
    montage.save(out/'all_pages.png');cmd('Q')
    for name in ['monitor_menu','calibration_menu']:
        p.reset_input_buffer();p.write(b'S')
        for _ in range(20):
            if b'FRAME240x536' in p.readline():break
        else:raise RuntimeError('No menu frame')
        raw=p.read(257280);assert len(raw)==257280
        values=[int.from_bytes(raw[i:i+2],'big') for i in range(0,len(raw),2)]
        im=Image.new('RGB',(240,536));im.putdata([((v>>11)*255//31,((v>>5)&63)*255//63,(v&31)*255//31) for v in values]);im.save(out/f'{name}.png');cmd('L')
    cmd('Q')
print('PASS: captured six real device pages and returned to menu.')
