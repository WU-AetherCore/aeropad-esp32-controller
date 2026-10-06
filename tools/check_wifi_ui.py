"""Exercise the WiFi menu and capture real device frames without changing router credentials."""
import time,serial
from pathlib import Path
from PIL import Image
root=Path(__file__).resolve().parents[1]
out=root/'logs/wifi_validation';out.mkdir(exist_ok=True)
with serial.Serial('COM11',115200,timeout=2) as p:
    p.dtr=False;p.rts=True;p.dtr=p.dtr;time.sleep(.15);p.rts=False;p.dtr=p.dtr
    deadline=time.monotonic()+15
    while time.monotonic()<deadline:
        if b'Ready: main menu' in p.readline():break
    else:raise RuntimeError('Device did not boot')
    def cmd(c):p.write(c.encode());time.sleep(.5)
    # Start from main menu after boot.
    cmd('R');cmd('R');cmd('E');cmd('R');cmd('R');cmd('R');cmd('E')
    def status():
        p.reset_input_buffer();cmd('T')
        for _ in range(10):
            b=p.readline()
            if b'[WIFI]' in b:print(b.decode(errors='replace'));return b
        raise RuntimeError('WiFi status not received')
    assert b'hotspot=0' in status()
    cmd('E');time.sleep(2);assert b'hotspot=1' in status()
    p.reset_input_buffer();p.write(b'S')
    for _ in range(20):
        if b'FRAME240x536' in p.readline():break
    else:raise RuntimeError('No framebuffer')
    raw=p.read(257280);assert len(raw)==257280,len(raw)
    im=Image.new('RGB',(240,536));im.putdata([((v>>11)*255//31,((v>>5)&63)*255//63,(v&31)*255//31) for v in [int.from_bytes(raw[i:i+2],'big') for i in range(0,len(raw),2)]])
    im.save(out/'wifi.png')
    cmd('Q');cmd('E');assert b'hotspot=1' in status()
    cmd('E');time.sleep(1);assert b'hotspot=0' in status()
    original=b'auto=1' in status()
    for enabled in [not original,original]:
        cmd('A');assert (b'auto=1' in status())==enabled
        p.dtr=False;p.rts=True;p.dtr=p.dtr;time.sleep(.15);p.rts=False;p.dtr=p.dtr
        deadline=time.monotonic()+15
        while time.monotonic()<deadline:
            if b'Ready: main menu' in p.readline():break
        else:raise RuntimeError('Restart failed')
        cmd('R');cmd('R');cmd('E');cmd('R');cmd('R');cmd('R');cmd('E')
        assert (b'auto=1' in status())==enabled,'Boot connection preference not persisted'
    cmd('Q');cmd('Q')
print('PASS: hotspot on/off and page return; both boot WiFi preferences persisted across restart; original preference restored, credentials untouched.')
