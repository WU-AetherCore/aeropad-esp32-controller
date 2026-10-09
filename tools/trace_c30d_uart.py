"""Passive C30D TX probe. USB-TTL RX/GND only. Never transmits bytes."""
import argparse,time,re,json
from pathlib import Path
import serial
parser=argparse.ArgumentParser();parser.add_argument('--port',required=True);parser.add_argument('--baud',type=int,default=0);parser.add_argument('--seconds',type=int,default=30);args=parser.parse_args()
if args.port.upper()=='COM11':raise SystemExit('COM11 is the controller; specify the separate USB-TTL port')
out=Path(__file__).resolve().parents[1]/'logs/c30d_uart';out.mkdir(parents=True,exist_ok=True)
def frames(raw):
 text=re.findall(rb'\{(?:A-?\d+:-?\d+:-?\d+:-?\d+(?::-?\d+)?|B-?\d+:-?\d+:-?\d+|C-?\d+:-?\d+:-?\d+)\}',raw)
 binary=[]
 for i in range(max(0,len(raw)-23)):
  p=raw[i:i+24]
  if p[0]==0x7b and p[23]==0x7d:
   check=0
   for b in p[:22]:check^=b
   if check==p[22]:binary.append(p)
 return text,binary
p=serial.Serial();p.port=args.port;p.timeout=.1;p.dtr=False;p.rts=False
try:
 choices=[args.baud] if args.baud else [9600,115200,19200,38400,57600]
 chosen=0
 for baud in choices:
  p.baudrate=baud;p.open();data=b'';deadline=time.monotonic()+2
  while time.monotonic()<deadline:data+=p.read(4096)
  t,b=frames(data);print(json.dumps({'baud':baud,'bytes':len(data),'APP':len(t),'ROS':len(b)}),flush=True)
  (out/f'detect-{baud}.bin').write_bytes(data)
  if t or b:chosen=baud;break
  p.close()
 if not chosen:raise SystemExit('No valid vendor feedback. Preserve raw captures; check TX/GND, power, UART and mode.')
 data=bytearray();start=time.monotonic();nextReport=start+1;lastByte=start
 while time.monotonic()-start<args.seconds:
  chunk=p.read(4096)
  if chunk:data+=chunk;lastByte=time.monotonic()
  if time.monotonic()>=nextReport:
   t,b=frames(data);last=t[-1].decode() if t else b[-1].hex(' ') if b else ''
   print(json.dumps({'elapsed':round(time.monotonic()-start,1),'baud':chosen,'bytes':len(data),'APP':len(t),'ROS':len(b),'silent_ms':round((time.monotonic()-lastByte)*1000),'last':last}),flush=True);nextReport=time.monotonic()+1
 (out/'passive-trace.bin').write_bytes(data)
 print('PASSIVE CAPTURE COMPLETE: no serial writes performed',flush=True)
finally:
 if p.is_open:p.close()
