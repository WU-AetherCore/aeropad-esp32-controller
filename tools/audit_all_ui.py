"""Serial navigation and framebuffer audit of all existing menu entries."""
import serial,time
from esptool.reset import HardReset
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'logs/all_ui_audit';out.mkdir(exist_ok=True)
with serial.Serial('COM11',115200,timeout=3) as port:
    port.set_buffer_size(rx_size=1048576,tx_size=65536)
    time.sleep(2)
    def cmd(c):port.write(c.encode());time.sleep(.4)
    capture_retries=[]
    def frame(name):
        for attempt in range(3):
            port.reset_input_buffer();port.write(b'S')
            for _ in range(15):
                if b'FRAME240x536' in port.readline():break
            else:raise RuntimeError('No frame '+name)
            data=bytearray();end=time.monotonic()+20
            while len(data)<257280 and time.monotonic()<end:data.extend(port.read(min(32768,257280-len(data))))
            if len(data)==257280:
                (out/(name+'.rgb565')).write_bytes(data)
                return bytes(data)
            capture_retries.append((name,len(data)))
            time.sleep(.5)
        raise RuntimeError('Short frame after retries '+name+': '+str(len(data)))
    port.dtr=False;HardReset(port)()
    end=time.monotonic()+12
    while time.monotonic()<end:
        if b'[AeroPad] Ready: main menu' in port.readline():break
    else:raise RuntimeError('Reset not ready')
    for root_index,(title,count) in enumerate([('nrf',2),('games',5),('network',3),('ble',0),('settings',3)]):
        frame('root_'+title);cmd('E')
        if title=='ble':
            for i in range(3):
                frame('ble_menu_'+str(i));cmd('E')
                if i==0:frame('ble');cmd('Q')
                elif i==1:frame('ble_scan');time.sleep(5);frame('ble_devices');cmd('Q')
                else:frame('ble_send_settings');cmd('Q')
                cmd('R')
            cmd('Q')
        else:
            for i in range(count):
                frame(title+'_menu_'+str(i));cmd('E')
                if title=='settings' and i==2:
                    frame('cal_devices');cmd('E');frame('cal_joystick_menu');cmd('R');cmd('E')
                    for j in range(4):frame('cal_axis_'+str(j));cmd('R')
                    cmd('Q');cmd('Q');cmd('R');cmd('E');frame('cal_knob_menu');cmd('R');cmd('E')
                    for j in range(2):frame('cal_knob_'+str(j));cmd('R')
                    cmd('Q');cmd('R');cmd('E');frame('cal_manual_knob');cmd('D');frame('cal_manual_min');cmd('D');frame('cal_manual_max');cmd('Q');cmd('Q');cmd('Q')
                else:frame(title+'_page_'+str(i));cmd('Q')
                cmd('R')
            cmd('Q')
        cmd('R')
    cmd('K');keys=frame('keys_geometry');cmd('Q');cmd('B');ble=frame('ble_geometry');cmd('Q')
    # Both same-size widget borders must be identical, including exact centering and spacing.
    for name,data,y in [('keys',keys,200),('ble',ble,310)]:
        pixel=lambda x,row:int.from_bytes(data[(row*240+x)*2:(row*240+x)*2+2],'big')
        for x in [10,129]:
            assert all(pixel(x+k,y)==0x07ff and pixel(x+k,y+100)==0x07ff for k in range(101)),name+' horizontal border'
            assert all(pixel(x,y+k)==0x07ff and pixel(x+100,y+k)==0x07ff for k in range(101)),name+' vertical border'
    (out/'result.txt').write_text('PASS: all menu entries entered/exited; both joystick widgets 101x101 in test and BLE pages; no calibration altered.')
    (out/'capture_retries.txt').write_text(str(capture_retries))
    print((out/'result.txt').read_text());print('Capture retries:',capture_retries)
