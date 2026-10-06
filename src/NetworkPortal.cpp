#include "NetworkPortal.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "ControlPacket.h"
#include <atomic>

namespace {
WebServer server(80); DNSServer dns;
std::atomic<int> command{0}; std::atomic<bool> ap{false};
std::atomic<bool> bootConnect{true};bool stationAllowed=false;
bool started=false;String savedSsid,savedPassword;uint32_t connectingAt=0;
bool scanning=false;uint32_t scanAt=0;String networks="{\"ready\":false,\"networks\":[]}";
KVS telemetry;uint32_t sampledAt=0;portMUX_TYPE lock=portMUX_INITIALIZER_UNLOCKED;
const char page[] PROGMEM=R"HTML(<!doctype html><html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>AeroPad WiFi 管理</title>
<style>body{font:16px system-ui;background:#eef3f8;color:#183049;max-width:760px;margin:auto;padding:20px}section{background:white;border-radius:16px;padding:20px;margin:16px 0}h1{font-size:26px}button,input,select{font:inherit;padding:12px;border-radius:9px;border:1px solid #bbcbd9;box-sizing:border-box}input,select{width:100%;margin:6px 0}button{background:#146ad1;color:white;cursor:pointer;margin:6px}pre{white-space:pre-wrap;overflow-wrap:anywhere}.hint{color:#596e81}#msg{color:#146ad1}</style>
<h1>AeroPad · WiFi 管理</h1><p class="hint">热点地址 192.168.4.1。配置路由器后，热点仍保留，便于查看连接结果。</p>
<section><h2>连接家庭 WiFi</h2><button onclick="scan()">搜索附近 WiFi</button><select id="net" onchange="ssid.value=this.value"><option>选择网络或手动填写</option></select>
<form id="config"><label>WiFi 名称<input id="ssid" maxlength="32" required></label><label>WiFi 密码<input id="password" type="password" maxlength="64" autocomplete="new-password"></label><button>保存并连接</button></form><p id="msg"></p><button onclick="forget()">清除已保存的路由器</button><p class="hint">仅支持 2.4 GHz。密码保存在设备中，网页不会回传密码。清除配置需要确认。</p></section>
<section><h2>开机连接设置</h2><label><input id="boot" type="checkbox" style="width:auto">开机自动连接已保存的 WiFi</label><button onclick="saveBoot()">保存开机设置</button><p class="hint">关闭后保留名称和密码，下次开机不连接。当前连接保持不变。</p></section>
<section><h2>网络与设备状态</h2><pre id="status">正在读取…</pre></section><section><h2>实时控件数据</h2><pre id="keys"></pre><p class="hint">数据来自设备当前采样；返回其他页面后仍可查看。此页面只读，不发送遥控指令。</p></section>
<script>
const $=id=>document.getElementById(id);let scanPending=false;
let bootLoaded=false;async function saveBoot(){try{let r=await request('/api/autoconnect',{method:'POST',body:new URLSearchParams({enabled:$('boot').checked?'1':'0'})});$('msg').textContent=await r.text();bootLoaded=false;}catch(e){$('msg').textContent='保存失败，请重试'}}
async function bootInfo(){try{if(!bootLoaded){const d=await(await request('/api/autoconnect')).json();$('boot').checked=d.enabled;bootLoaded=true;}}catch(e){}setTimeout(bootInfo,2000)}bootInfo();
async function request(url,options={}){return fetch(url,{...options,cache:'no-store',signal:AbortSignal.timeout(3000)})}
async function scan(){if(scanPending)return;try{let r=await request('/api/scan',{method:'POST'});if(!r.ok)throw Error(await r.text());scanPending=true;$('msg').textContent='正在搜索，请稍候';checkScan();}catch(e){$('msg').textContent=e.message}}
async function checkScan(){try{let s=await(await request('/api/networks')).json();if(s.ready){const previous=$('ssid').value;$('net').replaceChildren(new Option('选择网络或手动填写',''));s.networks.forEach(n=>$('net').add(new Option(`${n.ssid} (${n.rssi} dBm)`,n.ssid)));$('net').value=previous;scanPending=false;$('msg').textContent='搜索完成';return;}if(s.failed){scanPending=false;$('msg').textContent='搜索失败，请稍后重试';return;}}catch(e){}setTimeout(checkScan,500)}
$('config').onsubmit=async e=>{e.preventDefault();let r=await fetch('/api/connect',{method:'POST',body:new URLSearchParams({ssid:$('ssid').value,password:$('password').value})});$('msg').textContent=await r.text()};async function forget(){if(confirm('清除已保存的路由器 WiFi 配置？')){$('msg').textContent=await(await fetch('/api/forget',{method:'POST'})).text()}}
async function tick(){try{let d=await(await request('/api/status')).json();$('status').textContent=`路由器：${d.connected?'已连接':d.connecting?'正在连接':d.failed?'连接失败，请检查密码':'未连接'}\nWiFi：${d.ssid||'未配置'}\n路由器 IP：${d.ip}\n信号：${d.rssi} dBm\n热点客户端：${d.clients}\n运行：${d.uptime} 秒\n可用内存：${d.heap} 字节\nPSRAM：${d.psram} 字节\nMAC：${d.mac}`;}catch(e){$('status').textContent='网络暂时忙，正在自动重试'}setTimeout(tick,2000)}tick();
async function live(){let delay=50;try{if(!document.hidden){let d=await(await request('/api/live')).json();$('keys').textContent=`左摇杆 X ${d.lx} / Y ${d.ly}\n右摇杆 X ${d.rx} / Y ${d.ry}\n旋钮 左 ${d.kl} / 右 ${d.kr}\n按键位图 ${d.buttons}\n倾角 X ${d.ax} / Y ${d.ay}\n距采样 ${d.age} ms`;}else delay=1000;}catch(e){delay=500;}setTimeout(live,delay)}live();
</script></html>)HTML";
String quote(const String& s){String r="\"";for(size_t i=0;i<s.length();i++){unsigned char c=s[i];if(c=='"'||c=='\\'){r+='\\';r+=char(c);}else if(c<32){char b[7];snprintf(b,7,"\\u%04x",c);r+=b;}else r+=char(c);}return r+'"';}
void startAp(){WiFi.mode(WIFI_AP_STA);WiFi.setSleep(true);ap=WiFi.softAP("AeroPad-Setup","12345678",WiFi.status()==WL_CONNECTED?WiFi.channel():6,false,4);if(ap)dns.start(53,"*",WiFi.softAPIP());}
void join(){if(savedSsid.length()){stationAllowed=true;WiFi.setAutoReconnect(true);WiFi.begin(savedSsid.c_str(),savedPassword.c_str());connectingAt=millis();}}
void saveBoot(bool enabled){Preferences p;p.begin("aerowifi",false);p.putBool("boot",enabled);p.end();bootConnect=enabled;}
void worker(void*){
    Preferences p;p.begin("aerowifi",true);savedSsid=p.getString("ssid","");savedPassword=p.getString("password","");bootConnect=p.getBool("boot",true);p.end();
    // ESP32-S3 WiFi/BLE coexistence requires modem sleep; disabling it aborts
    // in coex_core_enable when the BLE controller starts.
    WiFi.persistent(false);WiFi.mode(WIFI_STA);WiFi.setSleep(true);WiFi.setAutoReconnect(false);WiFi.disconnect();if(bootConnect)join();
    server.on("/api/autoconnect",HTTP_GET,[]{server.send(200,"application/json",bootConnect?"{\"enabled\":true}":"{\"enabled\":false}");});
    server.on("/api/autoconnect",HTTP_POST,[]{String v=server.arg("enabled");if(v!="0"&&v!="1"){server.send(400,"text/plain","invalid");return;}saveBoot(v=="1");server.send(200,"text/plain; charset=utf-8",bootConnect?"已保存：开机自动连接 WiFi":"已保存：开机不连接 WiFi");});
    server.on("/api/live",HTTP_GET,[]{KVS k;uint32_t age;portENTER_CRITICAL(&lock);k=telemetry;age=millis()-sampledAt;portEXIT_CRITICAL(&lock);char j[220];snprintf(j,sizeof(j),"{\"lx\":%d,\"ly\":%d,\"rx\":%d,\"ry\":%d,\"kl\":%d,\"kr\":%d,\"ax\":%d,\"ay\":%d,\"buttons\":%lu,\"age\":%lu}",k.LX,k.LY,k.RX,k.RY,k.L_knob,k.R_knob,k.angleX,k.angleY,(unsigned long)ControlPacket::buttons(k),(unsigned long)age);server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",j);});
    server.on("/",HTTP_GET,[]{server.send_P(200,"text/html; charset=utf-8",page);});
    server.on("/api/status",HTTP_GET,[]{KVS k;uint32_t age;portENTER_CRITICAL(&lock);k=telemetry;age=millis()-sampledAt;portEXIT_CRITICAL(&lock);
        bool connected=WiFi.status()==WL_CONNECTED;String j="{\"connected\":"+String(connected?"true":"false")+",\"connecting\":"+String(!connected&&connectingAt&&millis()-connectingAt<20000?"true":"false")+",\"failed\":"+String(!connected&&connectingAt&&millis()-connectingAt>=20000?"true":"false");
        j+=",\"ssid\":"+quote(savedSsid)+",\"ip\":"+quote(WiFi.localIP().toString())+",\"mac\":"+quote(WiFi.macAddress());
        j+=",\"rssi\":"+String(connected?WiFi.RSSI():0)+",\"clients\":"+String(WiFi.softAPgetStationNum())+",\"uptime\":"+String(millis()/1000)+",\"heap\":"+String(ESP.getFreeHeap())+",\"psram\":"+String(ESP.getFreePsram());
        j+=",\"lx\":"+String(k.LX)+",\"ly\":"+String(k.LY)+",\"rx\":"+String(k.RX)+",\"ry\":"+String(k.RY)+",\"kl\":"+String(k.L_knob)+",\"kr\":"+String(k.R_knob)+",\"ax\":"+String(k.angleX)+",\"ay\":"+String(k.angleY)+",\"buttons\":"+String(ControlPacket::buttons(k))+",\"age\":"+String(age)+"}";server.send(200,"application/json",j);});
    server.on("/api/connect",HTTP_POST,[]{String ssid=server.arg("ssid"),pass=server.arg("password");if(ssid.isEmpty()||ssid.length()>32||(pass.length()&&pass.length()<8)||pass.length()>64){server.send(400,"text/plain; charset=utf-8","名称或密码长度无效");return;}savedSsid=ssid;savedPassword=pass;Preferences p;p.begin("aerowifi",false);p.putString("ssid",ssid);p.putString("password",pass);p.end();join();server.send(200,"text/plain; charset=utf-8","已保存，正在连接；请查看下方状态");});
    server.on("/api/forget",HTTP_POST,[]{Preferences p;p.begin("aerowifi",false);p.clear();p.end();savedSsid="";savedPassword="";connectingAt=0;WiFi.disconnect();server.send(200,"text/plain; charset=utf-8","已清除路由器配置，热点仍可使用");});
    server.on("/api/scan",HTTP_POST,[]{if(scanning){server.send(202,"text/plain","scanning");return;}if(scanAt&&millis()-scanAt<5000){server.send(429,"text/plain; charset=utf-8","请等待5秒再搜索");return;}if(WiFi.status()!=WL_CONNECTED&&connectingAt&&millis()-connectingAt<20000){server.send(409,"text/plain; charset=utf-8","正在连接路由器，请稍后搜索");return;}WiFi.setAutoReconnect(false);WiFi.scanDelete();int result=WiFi.scanNetworks(true,false,true,120);scanning=result==WIFI_SCAN_RUNNING;scanAt=millis();networks="{\"ready\":false,\"networks\":[]}";if(!scanning){WiFi.setAutoReconnect(true);networks="{\"ready\":false,\"failed\":true,\"networks\":[]}";server.send(503,"text/plain; charset=utf-8","搜索启动失败，请重试");return;}server.send(202,"text/plain","scanning");});
    server.on("/api/networks",HTTP_GET,[]{server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",networks);});
    server.onNotFound([]{server.sendHeader("Location","http://192.168.4.1/");server.send(302,"text/plain","");});server.begin();
    for(;;){int c=command.exchange(0);if(c==2||c==3)saveBoot(c==3);if(c==1){if(ap){dns.stop();WiFi.softAPdisconnect(true);ap=false;WiFi.mode(WIFI_STA);}else startAp();}
        if(scanning){int n=WiFi.scanComplete();if(n>=0||n==WIFI_SCAN_FAILED||millis()-scanAt>12000){networks=n>=0?"{\"ready\":true,\"networks\":[":"{\"ready\":false,\"failed\":true,\"networks\":[";for(int i=0;i<min(n,32);i++){if(i)networks+=',';networks+="{\"ssid\":"+quote(WiFi.SSID(i))+",\"rssi\":"+String(WiFi.RSSI(i))+"}";}networks+="]}";WiFi.scanDelete();scanning=false;WiFi.setAutoReconnect(true);}}
        if(ap)dns.processNextRequest();server.handleClient();vTaskDelay(pdMS_TO_TICKS(2));}
}
}
void WIFI::begin(){if(started)return;started=true;xTaskCreate(worker,"wifi_portal",8192,nullptr,1,nullptr);}
void WIFI::toggleHotspot(){begin();command=1;}
bool WIFI::hotspot(){return ap.load();}
bool WIFI::autoConnect(){return bootConnect.load();}
void WIFI::setAutoConnect(bool enabled){begin();command=enabled?3:2;}
void WIFI::publish(const KVS& data){portENTER_CRITICAL(&lock);telemetry=data;sampledAt=millis();portEXIT_CRITICAL(&lock);}
