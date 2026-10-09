import unittest,json,sys
from pathlib import Path
from dataclasses import asdict
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'examples'))
import host_receiver as rx
class ReceiverTests(unittest.TestCase):
 def test_all_sender_vectors_fragmented_and_concatenated(self):
  vectors=json.loads((ROOT/'docs/protocol_vectors.json').read_text())
  for v in vectors:
   with self.subTest(case=v['case'],format=v['format']):
    wire=bytes.fromhex(v['wire_hex']);d=rx.Decoder(v['format']);frames=[]
    for i in range(0,len(wire),3):frames+=d.feed(wire[i:i+3])
    self.assertEqual([asdict(f) for f in frames],[v['expected']]);self.assertEqual(len(d.feed(wire*2)),2)
 def test_binary_noise_crc_and_resync(self):
  v=json.loads((ROOT/'docs/protocol_vectors.json').read_text())[4];wire=bytes.fromhex(v['wire_hex']);bad=bytearray(wire);bad[-1]^=1;d=rx.Decoder('binary')
  self.assertEqual(len(d.feed(b'noise'+bad+wire)),1)
 def test_reject_invalid_and_overlong_lines(self):
  for fmt in ('json','hex','text'):
   d=rx.Decoder(fmt);self.assertEqual(d.feed(b'x'*300+b'\n'),[])
   self.assertEqual(d.feed(b'[]\n'),[])
  with self.assertRaises(ValueError):rx.validate(dict(v=1))
  base=json.loads((ROOT/'docs/protocol_vectors.json').read_text())[0]['expected'];d=dict(v=1,**base)
  for key,value in [('lx',101),('buttons',0x40000),('seq',256),('neutral',2)]:
   with self.assertRaises(ValueError):rx.validate(dict(d,**{key:value}))
 def test_photo_text_split_inside_field(self):
  wire=b'AP1 seq=86 LX=0 LY=0 RX=0 RY=0 KL=-88 KR=-74 BTN=00000 AX=40 AY=2 N=0\n'
  d=rx.Decoder('text');frames=[]
  for i in range(0,len(wire),20):
   result=d.feed(wire[i:i+20]);frames+=result
   if i+20<len(wire):self.assertEqual(result,[])
  self.assertEqual(len(frames),1)
  self.assertEqual(rx.display_line(frames[0]).encode()+b'\n',wire)
 def test_motor_enable_and_release(self):
  d=dict(v=1,seq=0,lx=0,ly=-100,rx=100,ry=0,kl=0,kr=0,buttons=1,ax=0,ay=0,neutral=0)
  self.assertEqual(rx.motor_intent(rx.validate(d)),(30,0));self.assertEqual(rx.motor_intent(rx.validate(dict(d,neutral=1))),(0,0));self.assertEqual(rx.motor_intent(rx.validate(dict(d,buttons=0))),(0,0))
if __name__=='__main__':unittest.main()
