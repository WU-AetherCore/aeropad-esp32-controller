import serial,time,sys
from pathlib import Path
sys.path.insert(0,str(Path.home()/'.platformio/packages/tool-esptoolpy'))
from esptool.reset import HardReset
out=Path(__file__).resolve().parents[1]/'logs/ble_entry.txt'
records=[]
with serial.Serial('COM11',115200,timeout=.5) as p:
    def collect(seconds):
        data=b'';end=time.monotonic()+seconds
        while time.monotonic()<end:data+=p.read_all();time.sleep(.05)
        records.append(data.decode(errors='replace'));return data
    for name,c in [('gamepad',b'B'),('module',b'V')]:
        p.dtr=False;HardReset(p)();data=collect(6)
        p.write(c);data=collect(10)
        assert b'abort()' not in data and b'Rebooting' not in data and b'Guru Meditation' not in data,name+' crashed'
        assert b'[BLE] advertising' in data,name+' initialization missing'
        print(name,data.decode(errors='replace'))
        p.write(b'Q');collect(1)
        for entry in [b'B',b'V',b'B',b'V']:
            p.write(entry);data=collect(4)
            assert not any(s in data for s in [b'abort()',b'Rebooting',b'Guru Meditation']), 'switching crashed'
            p.write(b'Q');collect(1)
print('PASS: both BLE entries and repeated page switching without reboot.')
out.write_text('\n'.join(records),encoding='utf8')
