"""Four-format protocol reference, stream decoder and serial monitor.

python examples/host_receiver.py --port COM6 --format binary --baud 115200
This tool prints motor intent; it does not operate an actuator.
"""
import argparse,json,time
from dataclasses import dataclass,asdict
AXES=('lx','ly','rx','ry','kl','kr','ax','ay')
def crc16(data):
    v=0xffff
    for b in data:
        v^=b<<8
        for _ in range(8):v=((v<<1)^0x1021 if v&0x8000 else v<<1)&0xffff
    return v
@dataclass
class Frame:
    seq:int;lx:int;ly:int;rx:int;ry:int;kl:int;kr:int;buttons:int;ax:int;ay:int;neutral:int
def validate(d):
    if not isinstance(d,dict) or type(d.get('v')) is not int or d.get('v')!=1:raise ValueError('version')
    names=('seq',)+AXES+('buttons','neutral')
    if any(type(d.get(k)) is not int for k in names):raise ValueError('missing or non-integer field')
    if not 0<=d['seq']<=255 or not 0<=d['buttons']<=0x3ffff or d['neutral'] not in (0,1):raise ValueError('field bounds')
    if any(not -100<=d[k]<=100 for k in AXES):raise ValueError('axis bounds')
    if d['neutral']:d=dict(d,**{k:0 for k in AXES},buttons=0)
    return Frame(**{k:d[k] for k in names})
def binary(data):
    if len(data)!=20 or data[:4]!=b'\xa5\x5a\x01\x01' or data[13]&0xfc or data[16]&0xfe or data[17]:raise ValueError('header/reserved')
    if crc16(data[:18])!=int.from_bytes(data[18:20],'little'):raise ValueError('CRC')
    values=[b if b<128 else b-256 for b in data[5:11]+data[14:16]]
    return validate(dict(v=1,seq=data[4],buttons=int.from_bytes(data[11:14],'little'),neutral=data[16],**dict(zip(AXES,values))))
def line(data,fmt):
    s=data.decode('ascii').strip()
    if fmt=='json':return validate(json.loads(s))
    if fmt=='hex':
        tokens=s.split()
        if len(tokens)!=20 or any(len(t)!=2 for t in tokens):raise ValueError('HEX size')
        return binary(bytes(int(t,16) for t in tokens))
    if not s.startswith('AP1 '):raise ValueError('AP1 header')
    fields={}
    for item in s[4:].split():
        k,v=item.split('=')
        if k in fields:raise ValueError('duplicate field')
        fields[k]=int(v,16 if k=='BTN' else 10)
    mapping={'seq':'seq','LX':'lx','LY':'ly','RX':'rx','RY':'ry','KL':'kl','KR':'kr','BTN':'buttons','AX':'ax','AY':'ay','N':'neutral'}
    if set(fields)!=set(mapping):raise ValueError('AP1 fields')
    return validate(dict(v=1,**{mapping[k]:v for k,v in fields.items()}))
class Decoder:
    def __init__(self,fmt):
        if fmt not in ('binary','json','hex','text'):raise ValueError('format')
        self.fmt=fmt;self.buffer=bytearray();self.discard=False;self.errors=0
    def reset(self):self.buffer.clear();self.discard=False
    def feed(self,data):
        frames=[]
        for b in data:
            if self.fmt=='binary':
                self.buffer.append(b)
                if len(self.buffer)==20:
                    try:frames.append(binary(bytes(self.buffer)));self.buffer.clear()
                    except ValueError:self.errors+=1;del self.buffer[0]
            elif b==10:
                if self.buffer and not self.discard:
                    try:frames.append(line(bytes(self.buffer),self.fmt))
                    except (ValueError,UnicodeError,TypeError):self.errors+=1
                self.reset()
            elif not self.discard:
                if len(self.buffer)>=191:self.errors+=1;self.buffer.clear();self.discard=True
                else:self.buffer.append(b)
        return frames
def motor_intent(f):
    if f.neutral or not f.buttons&1:return (0,0)
    clamp=lambda n:max(-100,min(100,n))
    return (int(clamp(-f.ly+f.rx)*.3),int(clamp(-f.ly-f.rx)*.3))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--port',required=True);p.add_argument('--baud',type=int,default=115200);p.add_argument('--format',choices=('binary','json','hex','text'),default='binary');p.add_argument('--timeout-ms',type=int,default=300);args=p.parse_args()
    if args.timeout_ms<=0:p.error('timeout must be positive')
    import serial
    decoder=Decoder(args.format);last=None;previous=None;stopped=True
    with serial.Serial(args.port,args.baud,timeout=.02) as port:
        print('STOP: startup; no valid control frame yet')
        while True:
            now=time.monotonic()
            for f in decoder.feed(port.read(max(1,port.in_waiting))):
                # Repeated sequence numbers do not keep stale motion alive.
                if previous==f.seq:
                    if f.neutral:print(json.dumps(dict(frame=asdict(f),motor_intent=(0,0)),ensure_ascii=False))
                    continue
                previous=f.seq;last=time.monotonic();stopped=False
                print(json.dumps(dict(frame=asdict(f),motor_intent=motor_intent(f)),ensure_ascii=False))
            if last is not None and now-last>=args.timeout_ms/1000 and not stopped:
                print('STOP: valid-frame timeout; motor_intent=[0,0]');stopped=True;decoder.reset();previous=None
if __name__=='__main__':main()
