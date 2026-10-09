from pathlib import Path
from PIL import Image,ImageFont,ImageDraw
root=Path(__file__).resolve().parents[1]
labels=[('title','NRF 调试',30),('present','模块已检测',28),('absent','未检测到模块',28),('stats','收发统计',24),('settings','射频参数',24),('data','收发数据',24),('channel','频道',24),('rate','速率',24),('power','功率',24),('role','本机角色',24),('period','发送间隔',24),('stopped','测试已停止',24),('running','测试发送中',24),('tx','发送 / 应答',24),('rx','有效 / 异常接收',24),('success','应答成功率',24),('noise','信道能量检测',24),('hit','检测到强信号',24),('quiet','未检测到强信号',24),('nav','B切页 · X退出',20),('operate','O启停 · A清零统计',20),('adjust','上下选项 · 左右调整',20),('peer','对端需相同射频参数',20),('ack','应答不代表执行动作',20),('last','最近接收序号',24),('local','本机采样',24),('remote','对端采样',24),('none','尚未收到测试包',24)]
labels += [('drone','无人机',30),('car','四驱车',30),('locked','已锁定',28),('armed','控制已启用',28),('online','接收端在线',24),('offline','接收端未连接',24),('arming','正在确认解锁',24),('armhint','长按O解锁 · A急停',20),('exitcontrol','B+X同时长按退出',22),('neutralhint','回中且油门低才能解锁',20),('throttle','左旋钮油门',24),('steering','转向 / 速度',24),('channels','遥控通道',24),('telemetry','接收端回传',24),('speedlimit','速度限制',24),('emergency','急停或失联已锁定',24),('rxlocked','接收端已锁定',24),('rxarmed','接收端已解锁',24),('nopending','功能待完善',24),('ratelimit','姿态幅度',24),('throttleoutput','油门采样 / 输出',24)]
labels += [('generic','NRF 设置',30),('rawstats','原始收发调试',26),('packet','数据包设置',26),('timing','重发与间隔',26),('txaddr','发送地址',26),('rxaddr','接收地址',26),('payload','发送内容',26),('presets','通信预设',26),('width','地址长度',24),('crc','硬件校验',24),('autoack','自动应答',24),('dynamic','动态长度',24),('rxlength','接收长度',24),('txlength','发送长度',24),('retrydelay','重发间隔',24),('retrycount','重发次数',24),('format','回显格式',24),('manualdata','手动原始数据',24),('sampledata','遥控通道数据',24),('genericnav','B切页 · X返回',24),('rawoperate','O启停 · A清零',24),('sourcehint','A数据源 · O启停',24),('presetapply','O应用 · 自动保存',24),('reference','江协参考程序',24),('standard','通用32字节',24),('oldaddress','原版遥控地址',24),('saved','设置已保存',24),('saving','设置保存中',24),('lastpacket','最近接收数据',24),('nosignal','未收到数据包',24),('presetwarn','应用后停止发送',24),('rawtx','发送 / 成功',24),('rawrx','接收 / 长度',24)]
labels += [('gadjust','上下选 · 左右调',24),('gdatahint','上下翻组 · 左右格式',22),('ghex','十六进制',24),('gtext','文本数据',24),('gbin','二进制',24)]
labels += [('rxhex','接收数据 · HEX',24),('rxtext','接收数据 · 文本',24),('rxbin','接收数据 · 二进制',24),('diagpreset','NDG1 专用测试',24),('diagreference','江协程序 · 4字节',24),('presetaddr','预设地址',24)]
labels += [('gstop','O停止 · B切页',24),('gstart','O发送 · A清零',24),('readonly','只读数据',24)]
labels += [('custom','自定义协议',26),('customdrone','无人机设置',28),('customcar','四驱车设置',28),('profilebank','保存 / 管理预设',24),('mapping','数据字节映射',26),('customhint','O进入自定义设置',22),('bankhint','上下槽位 · 左右操作',20),('confirmhint','O确认 · B切页',22),('savedok','预设保存成功',24),('loadedok','预设已应用',24),('deletedok','预设已删除',24),('deleteconfirm','再按O确认删除',24),('emptyprofile','此预设为空',24),('failed','操作失败',24),('applied','设置已应用',24),('saveslot','保存到此槽位',24),('loadslot','应用为默认预设',24),('delslot','删除此预设',24),('draft','参数已修改 · 发送停止',20),('banknav','O确认 · B切页 · X返回',20),('customnote','自定义需对端配套协议',20)]
labels += [('joydir','摇杆方向设置',26),('forwardaxis','前后控制摇杆',24),('turnaxis','左右转向摇杆',24),('forwardrev','前后方向',24),('turnrev','左右方向',24),('leftvertical','左摇杆 · 前后',24),('rightvertical','右摇杆 · 前后',24),('lefthorizontal','左摇杆 · 左右',24),('righthorizontal','右摇杆 · 左右',24),('normaldir','正常方向',24),('reversedir','反转方向',24),('presethint','A打开我的预设',22),('directionhint','上下选 · 左右调整',22),('bankoperations','上下选 左右调 O确认',22),('bankcontrol','A控制 B+X长按退出',22),('vehiclenav','B切页 B+X长按退出',22),('joyhint','前后用纵轴 · 转弯用横轴',18)]
for i in range(1,6):
 labels += [(f'slot{i}saved',f'预设{i} · 已保存',24),(f'slot{i}empty',f'预设{i} · 空',24)]
labels += [('controlnav','长按B切页 B+X退出',22),('settingnav','B下一页 · X返回',22),('menunav','O确认 · X返回',22),('menuoperate','上下选择 · O确认',22),('stopfirst','控制中 · 请先停止',24),('customstart','O开始 · A停止',24),('customstop','O停止 · A停止',24),('applyhint','O应用 · X返回',24),('sourceconfirm','O切换数据来源',24),('hub','控制设置',26),('defaultmode','默认控制模式',26),('standardmode','标准NRC1协议',24),('custommode','当前自定义协议',24),('defaultsaved','已设为默认配置',24),('radiomenu','射频参数',24),('packetmenu','通信数据',24),('contentmenu','发送内容',24),('joymenu','摇杆与映射',24),('bankoperate','上下选 左右调 O确认',22),('bankback','X返回设置菜单',24),('editing','仅停止后可修改',24),('draftsaved','草稿已保存 · O应用',22),('hubback','X返回遥控界面',24),('sourceData','数据来源',24)]
labels += [('controlrunning','遥控发送中',24),('controlstopped','遥控已停止',24),('stopwaiting','等待停止确认',24)]
for i in range(1,6):labels += [(f'using{i}',f'默认 · 预设{i}',24)]
labels += [('saveentry','A保存预设 · O应用',22),('saveguide','O保存当前全部设置',22),('applyguide','O应用并设为默认',22),('deleteguide','O删除 · 再次确认',22)]
for i in range(1,6):labels += [(f'savedslot{i}',f'已保存到预设{i}',24)]
labels += [('bptitle','蓝牙协议遥控',30),('bphub','协议控制中心',26),('bpcontrol','进入遥控界面',24),('bpconnect','搜索连接设备',24),('bpparams','协议参数设置',24),('bpmapping','自定义字节映射',24),('bpbuiltin','内置车型协议',24),('bpapp','C30D · APP方向',24),('bpsteer','C30D · APP转向',24),('bpros','C30D · ROS速度',24),('bpcustom','自定义字节协议',24),('bplinkfirst','请先搜索连接设备',24),('bpcenter','请将控制摇杆回中',24),('bpconnected','已连接',28),('bpdisconnected','未连接',28),('bpdisconnect','设备已请求断开',24),('bpdisconnectkey','O断开 · A重新搜索',22),('bpscanning','正在搜索',28),('bpconnecting','正在连接',28),('bpnodevices','未发现BLE设备',24),('bpleonly','仅支持BLE串口模块',22),('bpscankeys','O连接 · A重新搜索',22),('bpxback','X返回协议菜单',24),('bphubback','X返回蓝牙中心',24),('bpoverwrite','再按O覆盖此预设',24),('bppresetnote','O应用 · 保持停止',24),('bpconstants','自定义常量字节',24),('bpchecksum','末字节校验',24),('bpdead','方向触发死区',24),('bpspeed','最大前进速度',24),('bpangular','最大转向速度',24),('bpfixed','内置协议已固定',24),('bpchoosecustom','请先选自定义协议',22)]
labels += [('bpstopbytes','自定义停止指令',24)]
labels += [('bpunused','此协议不使用此项',24),('bpunusedvalue','不适用',24),('bpfixedvalue','协议固定',24)]
labels += [('bpknobminimum','限速调至25%以内',22),('bpc30d','轮趣 C30D',28),('bpspeedknob','速度限速旋钮',24),('bpturnknob','转向限速旋钮',24),('bpwire','接口通信方式',24),('bpknoboff','固定参数上限',24),('bpknobleft','左侧旋钮',24),('bpknobright','右侧旋钮',24),('bplimitnow','当前限速',24),('bptargetspeed','目标前进速度',24),('bpgroupona','第1/3页 · A保存',22),('bpgroupduo','第2/3页 · A保存',22),('bpgroupthree','第3/3页 · A保存',22),('bpparamnav','O应用 B方向 X返回',21)]
labels += [('bpfeedbackstop','回传超时已停止',24),('bpinputstop','采样超时已停止',24),('bpwritestop','通信异常已停止',24),('bpprotocolstop','协议接口不匹配',24),('bpstopunconfirmed','停车未确认',24),('bpcutpower','请关闭小车电源',22)]
assert len(labels)>100 and len({n for n,_,_ in labels})==len(labels)
out=['#pragma once' ,'#include <Arduino.h>','namespace NrfUi {']
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
