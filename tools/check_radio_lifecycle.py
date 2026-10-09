"""COM11 wireless ownership regression; never start vehicle motion."""
import serial,time,re,sys
from pathlib import Path
sys.path.insert(0,str(Path.home()/'.platformio/packages/tool-esptoolpy'))
from esptool.reset import HardReset
with serial.Serial('COM11',115200,timeout=.3) as p:
    p.set_buffer_size(rx_size=1048576)
    def cmd(c):p.write(c.encode());time.sleep(.3)
    def read():time.sleep(.5);return p.read_all().decode(errors='replace')
    def idle():
        p.reset_input_buffer();cmd('F');s=read();print(s,flush=True)
        m=re.search(r'\[RADIO STATUS\] (.*)',s);assert m,s
        d={k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',m[1])}
        assert d=={'mode':-1,'wifi':0,'adv':0,'scan':0,'hid':0,'module':0},d
        config=re.search(r'CONFIG\s*=\s*0x([0-9a-f]+)',s,re.I);assert config,s
        assert int(config[1],16)&2==0,'NRF PWR_UP is still set'
    p.dtr=False;HardReset(p)();time.sleep(8);p.dtr=True;idle()
    # WiFi portal twice, preserving routing credentials and boot preference.
    for c in 'RREE':cmd(c)
    cmd('E');time.sleep(1);cmd('T');s=read();print(s);assert 'hotspot=1' in s
    cmd('Q');idle();cmd('Q')
    cmd('B');time.sleep(3);cmd('T');print(read());cmd('Q');idle()
    for c in 'EE':cmd(c)
    cmd('E');time.sleep(1);cmd('T');s=read();print(s);assert 'hotspot=1' in s
    cmd('Q');cmd('Q');idle()
    # Enter NRF diagnostic without enabling transmission, exit -> hardware off.
    for c in 'RRRERRE':cmd(c)
    cmd('T');print(read());cmd('Q');idle();cmd('Q')
    # Module -> idle -> module -> idle -> HID -> idle, without stack teardown.
    for c in 'RRREE':cmd(c)
    cmd('T');s=read();print(s);assert '[BLEPROTO]' in s
    cmd('Q');idle();cmd('E');cmd('T');s=read();assert '[BLEPROTO]' in s
    cmd('Q');idle();cmd('Q');cmd('B');time.sleep(1);cmd('Q');idle()
    print('PASS: startup, WiFi twice, HID twice, NRF and BLE protocol twice; every exit WiFi OFF, no BLE activities/connections, NRF hardware PWR_UP clear')
