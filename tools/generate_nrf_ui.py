from pathlib import Path
from PIL import Image,ImageFont,ImageDraw
root=Path(__file__).resolve().parents[1]
labels=[('title','NRF 调试',30),('present','模块已检测',28),('absent','未检测到模块',28),('stats','收发统计',24),('settings','射频参数',24),('data','收发数据',24),('channel','频道',24),('rate','速率',24),('power','功率',24),('role','本机角色',24),('period','发送间隔',24),('stopped','测试已停止',24),('running','测试发送中',24),('tx','发送 / 应答',24),('rx','有效 / 异常接收',24),('success','应答成功率',24),('noise','信道能量检测',24),('hit','检测到强信号',24),('quiet','未检测到强信号',24),('nav','B切页 · X退出',20),('operate','O启停 · A清零统计',20),('adjust','上下选项 · 左右调整',20),('peer','对端需相同射频参数',20),('ack','应答不代表执行动作',20),('last','最近接收序号',24),('local','本机采样',24),('remote','对端采样',24),('none','尚未收到测试包',24)]
labels += [('drone','无人机',30),('car','四驱车',30),('locked','已锁定',28),('armed','控制已启用',28),('online','接收端在线',24),('offline','接收端未连接',24),('arming','正在确认解锁',24),('armhint','长按O解锁 · A急停',20),('exitcontrol','B切页 · X停止返回',20),('neutralhint','回中且油门低才能解锁',20),('throttle','左旋钮油门',24),('steering','转向 / 速度',24),('channels','遥控通道',24),('telemetry','接收端回传',24),('speedlimit','速度限制',24),('emergency','急停或失联已锁定',24),('rxlocked','接收端已锁定',24),('rxarmed','接收端已解锁',24),('nopending','功能待完善',24),('ratelimit','姿态幅度',24),('throttleoutput','油门采样 / 输出',24)]
labels += [('generic','NRF 设置',30),('rawstats','原始收发调试',26),('packet','数据包设置',26),('timing','重发与间隔',26),('txaddr','发送地址',26),('rxaddr','接收地址',26),('payload','发送内容',26),('presets','通信预设',26),('width','地址长度',24),('crc','硬件校验',24),('autoack','自动应答',24),('dynamic','动态长度',24),('rxlength','接收长度',24),('txlength','发送长度',24),('retrydelay','重发间隔',24),('retrycount','重发次数',24),('format','回显格式',24),('manualdata','手动原始数据',24),('sampledata','遥控通道数据',24),('genericnav','B切页 · X返回',24),('rawoperate','O启停 · A清零',24),('sourcehint','A数据源 · O启停',24),('presetapply','O应用 · 自动保存',24),('reference','江协参考程序',24),('standard','通用32字节',24),('oldaddress','原版遥控地址',24),('saved','设置已保存',24),('saving','设置保存中',24),('lastpacket','最近接收数据',24),('nosignal','未收到数据包',24),('presetwarn','应用后停止发送',24),('rawtx','发送 / 成功',24),('rawrx','接收 / 长度',24)]
labels += [('gadjust','上下选 · 左右调',24),('gdatahint','上下翻组 · 左右格式',22),('ghex','十六进制',24),('gtext','文本数据',24),('gbin','二进制',24)]
labels += [('rxhex','接收数据 · HEX',24),('rxtext','接收数据 · 文本',24),('rxbin','接收数据 · 二进制',24),('diagpreset','NDG1 专用测试',24),('diagreference','江协程序 · 4字节',24),('presetaddr','预设地址',24)]
labels += [('gstop','O停止 · B切页',24),('gstart','O发送 · A清零',24),('readonly','只读数据',24)]
out=['#pragma once','#include <Arduino.h>','namespace NrfUi {']
for n,t,z in labels:
 im=Image.new('1',(220,36));d=ImageDraw.Draw(im);f=ImageFont.truetype(str(root/'assets/fonts/NotoSansCJKsc-Regular.otf'),z);b=d.textbbox((0,0),t,font=f);assert b[2]-b[0]<=220
 d.text(((220-b[2]+b[0])/2-b[0],(36-b[3]+b[1])/2-b[1]),t,font=f,fill=1)
 # TFT drawBitmap rows must round up to 28 bytes per 220 pixels.
 vals=[]
 for y in range(36):
  for x in range(0,220,8):
   v=0
   for k in range(8):
    if x+k<220 and im.getpixel((x+k,y)):v|=128>>k
   vals.append(v)
 out.append('const uint8_t '+n+'[] PROGMEM={'+','.join(str(v) for v in vals)+'};')
out.append('}');(root/'src/NrfUiText.h').write_text('\n'.join(out)+'\n',encoding='utf-8')
# Save a reproducible generator.
