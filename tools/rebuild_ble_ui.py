from pathlib import Path
import json
from PIL import Image, ImageDraw, ImageFont
root = Path(__file__).resolve().parents[1]
(root/'logs').mkdir(exist_ok=True)
fontpath = str(root / 'assets/fonts/NotoSansCJKsc-Regular.otf')
assets = [('connected','已连接',28,(64,240,128)),('disconnected','未连接',28,(255,216,64)),('back','按 X 键返回主菜单',23,(220,228,240)),('title','蓝牙游戏手柄',32,(240,245,255))]
assets += [('cal','控件校准',30,(240,245,255)),('release','松开摇杆，按 O 校准',21,(220,228,240)),('saved','校准成功，已保存',23,(64,240,128)),('failed','请松开摇杆后重试',22,(255,216,64)),('dead','左右键调整中心死区',21,(220,228,240))]
assets += [('drone','无人机',30,(240,245,255)),('car','四驱车',30,(240,245,255)),('protocol','待配置接收端协议',23,(255,216,64))]
assets += [('m0', '主菜单', 28, (240, 245, 255)), ('m1', 'NRF遥控', 28, (240, 245, 255)), ('m2', '本机游戏', 28, (240, 245, 255)), ('m3', '网络信息', 28, (240, 245, 255)), ('m4', '系统设置', 28, (240, 245, 255)), ('m5', '按键测试', 28, (240, 245, 255)), ('m6', '陀螺仪立方体', 28, (240, 245, 255)), ('m7', '贪吃蛇', 28, (240, 245, 255)), ('m8', '打砖块', 28, (240, 245, 255)), ('m9', '飞机大战', 28, (240, 245, 255)), ('m10', '2048', 28, (240, 245, 255)), ('m11', '俄罗斯方块', 28, (240, 245, 255)), ('m12', '哔哩哔哩', 28, (240, 245, 255)), ('m13', '天气预报', 28, (240, 245, 255)), ('m14', '股票基金', 28, (240, 245, 255))]
assets += [('switch','左右键切换',22,(220,228,240)),('enter','O 键进入 · X 键返回',21,(220,228,240)),('exit','按 X 键返回上级',22,(220,228,240)),('pending','此功能尚未实现',23,(255,216,64))]
assets += [('rootenter','按 O 键进入',23,(220,228,240))]
assets += [('keysexit','同时按 B + X 返回',23,(220,228,240))]
assets += [(name,text,24,(220,228,240)) for name,text in [
('cauto','完整自动校准'),('cmanual','手动调整参数'),('cview','查看 ADC 数据'),('cstart','松开两个摇杆'),('ccenter','O 键采集中心'),('csweep','缓慢转动两个摇杆'),('cextreme','上下左右推到极限'),('cfinish','O 键检查采集范围'),('creview','检查后按 O 保存'),('crange','范围不足，请继续转动'),('cmin','最小值'),('cmid','中心值'),('cmax','最大值'),('craw','当前 ADC 采样'),('cunit','ADC 范围 0~4095'),('cdead','中心死区'),('caxis0','左摇杆 X 轴'),('caxis1','左摇杆 Y 轴'),('caxis2','右摇杆 X 轴'),('caxis3','右摇杆 Y 轴'),('cnext','左右键切换轴'),('cedit','上下选项 · 左右增减'),('ccancel','X 键取消 · O 键保存'),('cstep','每次调整 10 ADC'),('coutput','输出范围 -100~100'),('cconfirm','参数无效，检查范围'),('cdone','O 键返回校准菜单'),('cdeadhelp','死区内输出为零'),('cknob','旋钮 ADC 校准'),('caxis4','左侧旋钮'),('caxis5','右侧旋钮'),('ckturn','两个旋钮转到两端'),('ckmid','中点按行程自动计算')]]
assets += [(n,t,24,(220,228,240)) for n,t in [('kswitch','左右键切换旋钮'),('jstep','每次调整 10 采样值'),('jcal','两个摇杆'),('kcal','两个旋钮 ADC'),('guide','引导校准'),('inspect','查看已存数据'),('fine','手动微调'),('jdesc','校准松手中心与行程'),('kdesc','校准旋钮两端行程'),('s1','第 1 步 · 采集中心'),('s2','第 2 步 · 采集极限'),('s3','第 3 步 · 确认保存'),('k1','第 1 步 · 采集两端'),('k2','第 2 步 · 确认保存'),('sampling','实时采样值'),('axisselect','选择要调整的控件'),('finehint','上下选参数'),('adjusthint','左右键增减数值'),('menuhint','左右选择 · O 键进入')]]
assets = [(n,t,z,(255,216,64) if n in ('crange','cconfirm') else c) for n,t,z,c in assets]
assets += [(n,t,23,(220,228,240)) for n,t in [('autohelp','有效调整自动保存'),('autosaved','已自动保存，掉电保留'),('autoback','X 键返回 · O 键确认')]]
assets += [(n,t,23,(220,228,240)) for n,t in [('bleexit1','同时按 B + X'),('bleexit2','返回蓝牙菜单')]]
# Data colors are semantic and shared with the numeric renderer.
data_colors={'cmin':(80,200,255),'cmid':(80,235,140),'cmax':(255,170,80),'craw':(255,220,90),'sampling':(255,220,90),'cdead':(195,150,255)}
assets=[(n,t,z,data_colors.get(n,c)) for n,t,z,c in assets]
assets += [(n,t,22,(220,228,240)) for n,t in [('joyselect','摇杆 / 方向键选择'),('joyfield','上下选择参数'),('joyadjust','左右调整 · 摇杆/按键')]]
assets += [('cubelive','实时姿态',24,(80,235,140)),('cubepaused','已暂停',24,(255,216,64)),('cubeangles','横滚 X · 俯仰 Y · 航向 Z',20,(220,228,240)),('cubeyaw','航向为相对角度（度）',20,(220,228,240)),('cubecontrols','O 归零 · A 暂停/继续',21,(220,228,240))]
assets += [(n,t,21,(220,228,240)) for n,t in [('cubestill','请保持遥控器静止'),('cubecal','正在校准陀螺仪'),('cubeback','X 返回 · B 静置校准')]]
assets += [('cuberetry','请静置后按 B 校准',21,(255,216,64))]
assets += [(n,t,22,(220,228,240)) for n,t in [('blehub','蓝牙中心'),('module','蓝牙模块控制'),('sendsettings','发送设置'),('sendformat','输出格式'),('sendperiod','发送间隔'),('blebaud','BLE 无需串口波特率'),('uartremote','设备菜单可设置 UART'),('settingsnav','上下选择 · 左右调整'),('settingsback','X 返回 · 自动保存'),('searchwait','正在搜索附近设备'),('nodevices','未发现可连接设备'),('scancontrols','O 连接 · A 重新搜索'),('scanback','X 返回 · B 发送设置'),('rxoff','回显：关闭'),('rxfeedback','回显：HEX'),('rxbinary','回显：二进制'),('rxtext','回显：文本'),('rxjson','回显：JSON'),('rxnotprovided','模块未提供回显通道'),('moduleexit','B + X 返回设备菜单')]]
assets += [(n,t,23,(255,216,64)) for n,t in [('scanning','正在搜索'),('connecting','正在连接'),('nouart','设备不支持串口服务'),('connectfailed','连接失败，请重试')]]
assets += [(n,t,23,(220,228,240)) for n,t in [('uartsettings','模块串口设置'),('uartactual','已读取实际波特率'),('uartauto','自动读取模块参数'),('uartmanual','手动选择波特率'),('uartneedprotocol','需要模块支持参数协议'),('uartnav','上下选模式 · 左右选值'),('uartconfirm','O 读取或应用 · X 返回'),('startcontrol','开始遥控'),('disconnectdevice','断开设备连接')]]
assets += [('uartunsupported','模块不支持远程设置',22,(255,216,64)),('uartapplied','已应用并核对参数',22,(80,235,140)),('uartapplyfailed','设置失败，未确认生效',22,(255,216,64))]
assets += [('formatbinary','二进制 · 20 字节',23,(0,255,255)),('formatjson','JSON 文本',23,(0,255,255))]
assets += [('formathex','HEX 十六进制',24,(0,255,255)),('formattext','文本数据',24,(0,255,255))]
assets += [('preview','界面预览',24,(255,216,64))]
assets += [(n,t,24,(220,228,240)) for n,t in [('seriallocal','本机 USB 串口'),('serialremote','对方模块串口'),('usbmode','接收回显格式'),('usboff','关闭回显'),('usbbinary','原始二进制'),('usbhex','HEX 十六进制'),('usbtext','文本数据'),('usbjson','JSON 数据'),('usbinfo','USB无需设波特率'),('usbhelp','左右选择格式'),('remotehelp','需支持参数协议'),('serialpick','选择要设置的设备')]]
# Shared typography. Menu titles never shrink based on name length.
menu_titles={f'm{i}' for i in range(15)}|{'title','blehub','module','sendsettings','uartsettings','cal','drone','car','jcal','kcal','guide','inspect','fine','cauto','cmanual','cview','cknob','startcontrol','disconnectdevice','uartauto','uartmanual','menugroup'}
status_titles={'connected','disconnected','scanning','connecting','cubelive','cubepaused','cuberetry','saved','failed','crange','cconfirm','nouart','connectfailed','uartunsupported','uartapplied','uartapplyfailed'}
shorter={
 'release':'松开摇杆·O校准','jdesc':'校准中心与行程','settingsback':'X返回·自动保存','uartapplied':'已核对并生效','m10':'2048 游戏','back':'按 X 返回主菜单','dead':'左右调整中心死区','switch':'左右键选择','enter':'O进入 · X返回','rootenter':'O 键进入',
 'keysexit':'同时按 B+X 返回','crange':'范围不足请继续','cedit':'上下选·左右调','ccancel':'X取消 · O保存','cconfirm':'参数无效请检查',
 'ckmid':'中点按行程计算','jstep':'步长 10 采样值','s1':'第1步·采集中心','s2':'第2步·采集极限','s3':'第3步·确认保存','k1':'第1步·采集两端','k2':'第2步·确认保存',
 'menuhint':'方向选择·O进入','autosaved':'已保存，掉电保留','autoback':'X返回 · O确认','joyselect':'摇杆或方向键选择','joyadjust':'左右调整数值',
 'cubeangles':'三轴角度（度）','cubeyaw':'相对航向·单位度','cubecontrols':'O归零·A暂停/继续','cubeback':'X返回·B静置校准','cuberetry':'静置后按B校准',
 'blebaud':'BLE 无波特率','uartremote':'设备菜单设置UART','settingsnav':'上下选·左右调','scancontrols':'O连接·A重新搜索','scanback':'X返回·B模块设置',
 'rxfeedback':'回显预览（HEX）','rxnotprovided':'未提供回显通道','moduleexit':'B+X返回设备菜单','nouart':'无兼容串口服务','connectfailed':'连接失败请重试',
 'uartneedprotocol':'需支持参数协议','uartnav':'上下选·左右调','uartconfirm':'O确认·X返回','uartunsupported':'不支持远程设置','uartapplyfailed':'未确认设置生效',
 'uartauto':'自动读取','uartmanual':'手动设置','saved':'校准已保存','failed':'松开摇杆后重试','uartsettings':'串口设置'
}
assets += [('menugroup','功能菜单',30,(240,245,255))]
assets += [(n,t,24,(220,228,240)) for n,t in [('cube_roll','横滚'),('cube_pitch','俯仰'),('cube_yaw','航向')]]
assets=[(n,shorter.get(n,t),30 if n in menu_titles else 28 if n in status_titles else 24,c) for n,t,z,c in assets]
manifest={}
assets += [('bootwifi_on','开机自动连接：开',24,(64,240,128)),('bootwifi_off','开机自动连接：关',24,(255,216,64)),('wifibootkeys','O热点·A自动连接',24,(220,228,240))]
assets += [('wifimanage','WiFi 管理',30,(240,245,255)),('hotspoton','热点已开启',28,(64,240,128)),('hotspotoff','热点未开启',28,(255,216,64)),('wifipassword','热点密码',24,(220,228,240)),('routeron','WiFi 已连接',24,(64,240,128)),('routeroff','WiFi 未连接',24,(255,216,64)),('wifiguide','连接热点打开网页',24,(220,228,240)),('wifitoggle','O 开关热点',24,(220,228,240))]
out=['#pragma once','#include <Arduino.h>']
for name,text,size,color in assets:
    font=ImageFont.truetype(fontpath,size)
    width,height=(80,36) if name in {'cube_roll','cube_pitch','cube_yaw'} else (220,44)
    im=Image.new('RGB',(width,height)); d=ImageDraw.Draw(im)
    box=d.textbbox((0,0),text,font=font)
    if box[2]-box[0] > width-8:
        raise ValueError(f'{name}: text too wide at fixed {size}px: {text} ({box[2]-box[0]}px)')
    d.text(((width-(box[2]-box[0]))/2-box[0],(height-(box[3]-box[1]))/2-box[1]),text,font=font,fill=color)
    manifest[name]={'text':text,'font_size':size,'text_width':box[2]-box[0],'tile':[width,height],'menu_title':name in menu_titles}
    im.save(root/'logs'/f'ui_{name}.png')
    vals=[((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b in im.getdata()]
    out += [f'const uint16_t ui_{name}[] PROGMEM = {{']
    out += [','.join(f'0x{v:04x}' for v in vals[i:i+16])+',' for i in range(0,len(vals),16)]
    out += ['};',f'const uint16_t ui_{name}_w={width}, ui_{name}_h={height};']
# WiFi menu icon uses the same 200x200 canvas as existing menu icons.
im=Image.new('RGB',(200,200));d=ImageDraw.Draw(im)
for radius in (88,60,32):
    d.arc((100-radius,124-radius,100+radius,124+radius),215,325,fill=(0,220,255),width=13)
d.ellipse((89,116,111,138),fill=(240,245,255))
vals=[((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b in im.getdata()]
out+=['const uint16_t ui_wifiicon[] PROGMEM = {']
out += [','.join(f'0x{v:04x}' for v in vals[i:i+16])+',' for i in range(0,len(vals),16)]
out+=['};']
im.save(root/'logs/ui_wifiicon.png')
(root/'src/generated_ui_text.h').write_text('\n'.join(out))

(root/'logs/ui_typography.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')



