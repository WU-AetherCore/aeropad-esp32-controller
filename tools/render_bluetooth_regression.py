from pathlib import Path
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1];out=root/'logs/bluetooth_regression';docs=root/'docs/images';docs.mkdir(parents=True,exist_ok=True)
for f in out.glob('*.rgb565'):
 d=f.read_bytes();assert len(d)==257280
 pixels=[]
 for i in range(0,len(d),2):
  v=(d[i]<<8)|d[i+1];r=(v>>11)&31;g=(v>>5)&63;b=v&31;pixels.append(((r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2)))
 im=Image.new('RGB',(240,536));im.putdata(pixels);im.save(f.with_suffix('.png'))
def sheet(names,target):
 im=Image.new('RGB',(260*len(names),566),(28,31,36));draw=ImageDraw.Draw(im)
 for i,n in enumerate(names):draw.text((i*260+10,5),n,fill='white');im.paste(Image.open(out/(n+'.png')),(i*260+10,25))
 im.save(docs/target)
sheet(['format_0','format_1','format_2','format_3'],'formats.png')
sheet(['device_actions','serial_before_0','remote_0','actions_after_0'],'serial-return.png')
sheet(['icon','period_ms_2000'],'icon-and-2000ms.png')
print('Decoded current regression frames and copied selected, non-scan UI images to docs/images')
