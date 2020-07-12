#include <ESP8266WebServer.h>
#include <flash_hal.h>
#include <FS.h>
#include "StreamString.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include "Http.h"
#include "Module.h"
#include "Rtc.h"

ESP8266WebServer *Http::server;
bool Http::isBegin = false;
String Http::updaterError;

void Http::handleRoot()
{
    if (captivePortal())
    {
        return;
    }
    if (!checkAuth())
    {
        return;
    }

    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->send(200, F("text/html"), "");
    server->sendContent_P(
        PSTR("<!DOCTYPE html><html lang='zh-cn'><head><meta charset='utf-8'/><meta name='viewport'content='width=device-width, initial-scale=1, user-scalable=no'/><title>"));
    server->sendContent(module ? module->getModuleCNName() : F("修复模式"));
    server->sendContent_P(
        PSTR("</title><style type='text/css'>body{font-family:-apple-system,BlinkMacSystemFont,'Microsoft YaHei',sans-serif;font-size:16px;color:#333;line-height:1.75}#body{margin:0 auto;width:80%;max-width:600px}@media screen and (max-width:900px){#body{width:98%}}#nav{text-align:center}#tab>div{display:none}#nav button{background:#eee;border:1px solid #ddd;padding:.7em 1em;cursor:pointer;z-index:1;margin-left:-1px;outline:0}#nav .active{background:#fff}table.gridtable{color:#333;border-width:1px;border-color:#ddd;border-collapse:collapse;margin:auto;margin-top:15px;width:100%}table.gridtable th{border-width:1.5px;padding:8px;border-style:solid;border-color:#ddd;background-color:#f5f5f5}table.gridtable td{border-width:1px;padding:8px;border-style:solid;border-color:#ddd;background-color:#fff}input,select{border:1px solid #ccc;padding:7px 0;border-radius:3px;padding-left:5px;-webkit-box-shadow:inset 0 1px 1px rgba(0,0,0,.075);box-shadow:inset 0 1px 1px rgba(0,0,0,.075);-webkit-transition:border-color ease-in-out .15s,-webkit-box-shadow ease-in-out .15s;-o-transition:border-color ease-in-out .15s,box-shadow ease-in-out .15s;transition:border-color ease-in-out .15s,box-shadow ease-in-out .15s}input:focus,select:focus{border-color:#66afe9;outline:0;-webkit-box-shadow:inset 0 1px 1px rgba(0,0,0,.075),0 0 8px rgba(102,175,233,.6);box-shadow:inset 0 1px 1px rgba(0,0,0,.075),0 0 8px rgba(102,175,233,.6)}#tab button{color:#fff;border-width:0;border-radius:3px;cursor:pointer;outline:0;font-size:17px;line-height:2.4rem;width:100%}#tab button[disabled]{cursor:not-allowed;filter:alpha(opacity=65);-webkit-box-shadow:none;box-shadow:none;opacity:.65}.btn-info{background-color:#5bc0de;border-color:#46b8da}.btn-info:hover{background-color:#31b0d5;border-color:#269abc}.btn-success{background-color:#5cb85c;border-color:#4cae4c}.btn-success:hover{background-color:#449d44;border-color:#398439}.btn-danger{background-color:#d9534f;border-color:#d43f3a}.btn-danger:hover{background-color:#c9302c;border-color:#ac2925}.alert{width:80%;padding:15px;border:1px solid transparent;border-radius:4px;position:fixed;top:10px;left:10%;z-index:999999;display:none}label.bui-radios-label input{position:absolute;opacity:0;visibility:hidden}label.bui-radios-label .bui-radios{display:inline-block;position:relative;width:13px;height:13px;background:#fff;border:1px solid #979797;border-radius:50%;vertical-align:-2px}label.bui-radios-label input:checked+.bui-radios:after{position:absolute;content:'';width:7px;height:7px;background-color:#fff;border-radius:50%;top:3px;left:3px}label.bui-radios-label input:checked+.bui-radios{background:#00b066;border:1px solid #00b066}label.bui-radios-label input:disabled+.bui-radios{background-color:#e8e8e8;border:solid 1px #979797}label.bui-radios-label input:disabled:checked+.bui-radios:after{background-color:#c1c1c1}label.bui-radios-label .bui-radios{-webkit-transition:background-color ease-out .3s;transition:background-color ease-out .3s}input[type='range']{width:80%;height:10px;border:0;background-color:#f0f0f0;border-radius:5px;position:relative;-webkit-appearance:none!important;outline:0}input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:20px;height:20px;border-radius:50%;background:#f40}.file{position:relative;display:inline-block;background:#d0eeff;border:1px solid #99d3f5;border-radius:4px;padding:4px 12px;overflow:hidden;color:#1e88c7;text-decoration:none;text-indent:0;line-height:20px}.file input{position:absolute;font-size:100px;right:0;top:0;opacity:0}.file:hover{background:#aadffd;border-color:#78c3f3;color:#004974;text-decoration:none}</style>"
             "<script type='text/javascript'>"
             "var logIndex=0;var defIntervalTime=3000;var intervalTime=defIntervalTime;var lt;function id(d){return document.getElementById(d)}function tab(v){var divs=id('tab').childNodes;var btns=id('nav').getElementsByTagName('button');for(var i=0;i<divs.length;i++){divs[i].style.display=divs[i]==id('tab'+v)?'block':'none';btns[i].setAttribute('class',(i+1==v?'active':''))}intervalTime=v==5?1000:defIntervalTime}function serialize(form){var field,s='';if(typeof form=='object'&&form.nodeName=='FORM'){for(var i=0;i<form.elements.length;i++){field=form.elements[i];if(field.name&&!field.disabled&&field.type!='file'&&field.type!='reset'&&field.type!='submit'&&field.type!='button'){if((field.type!='checkbox'&&field.type!='radio')||field.checked){s+=field.name+'='+encodeURIComponent(field.value)+'&'}}}}if(s.length>1){s=s.substring(0,s.length-1)}return s}function ajax(){var ajaxData={type:(arguments[0].type||'GET').toUpperCase(),url:arguments[0].url||'',data:arguments[0].data||null,success:arguments[0].success||function(){},error:arguments[0].error||function(){}};var xhr=window.XMLHttpRequest?new XMLHttpRequest():new ActiveXObject('Microsoft.XMLHTTP');xhr.responseType='json';xhr.open(ajaxData.type,ajaxData.url);if(ajaxData.type=='POST'){xhr.setRequestHeader('Content-Type','application/x-www-form-urlencoded; charset=utf-8');xhr.send(ajaxData.data)}else{xhr.send()}xhr.onreadystatechange=function(){if(xhr.readyState==4){if(xhr.status==200){ajaxData.success(xhr.response)}else{ajaxData.error()}if(ajaxData.url=='/get_status'){lt=setTimeout(get_status,intervalTime)}}}}function toast(msg,duration,isok){var m=id('alert');m.innerHTML=msg;m.style.cssText=isok?'color: #3c763d;background-color: #dff0d8;border-color: #d6e9c6;':'color: #a94442; background-color: #f2dede; border-color: #ebccd1;';m.style.display='block';setTimeout(function(){var d=0.5;m.style.webkitTransition='-webkit-transform '+d+'s ease-in, opacity '+d+'s ease-in';m.style.opacity='0';setTimeout(function(){m.style.display='none'},d*1000)},duration)}function postform(the){ajaxPost(the.getAttribute('action'),serialize(the));return false}function getRadioValue(radioName){var radios=document.getElementsByName(radioName);for(var i=0;i<radios.length;i++){var radio=radios.item(i);if(radio.checked){return radio.value}}return undefined}function setRadioValue(radioName,value){var radios=document.getElementsByName(radioName);for(var i=0;i<radios.length;i++){var radio=radios.item(i);if(radio.value==value){radio.checked=true;return}}}function ajaxPost(url,data,callback){ajax({type:'POST',url:url,dataType:'json',data:data,success:function(data){if(typeof(callback)=='function'){if(callback(data)===true){return}}if(data.msg){toast(data.msg,data.code?3000:5000,data.code)}if(data.data){setData(data.data)}},error:function(){toast('<strong>Oh snap!</strong> 请求出错！',5000,false)}})}function get_status(){clearTimeout(lt);ajaxPost('/get_status','i='+logIndex)}window.addEventListener('load',get_status);"
             "function setData(data){for(var key in data){if(typeof(setDataSub)=='function'){var result=setDataSub(data,key);if(result){continue}}var v=data[key];if(key=='discovery'){id('discovery').innerHTML=v==1?'已启动':'未启动';id('discovery_btn').setAttribute('class',v==1?'btn-danger':'btn-info');id('discovery_btn').innerHTML=v==1?'关闭MQTT自动发现':'打开MQTT自动发现'}else if(key=='logindex'){logIndex=v}else if(key=='log'){if(v){id('log').value+=v;id('log').scrollTop=99999}}else if(key=='ip'){if(v&&v!=window.location.hostname){toast('连接WiFi成功，IP地址：'+v,5000,1);window.setTimeout('location.href=\\'http://'+v+'\\'',5000)}}else{if(id(key)){id(key).innerHTML=v}else{console.log(key)}}}}"
             "</script></head><body><div id='body'><div id='alert' class='alert'></div><h1 style='text-align:center'>"));
    server->sendContent(module ? module->getModuleCNName() : F("修复模式"));
    server->sendContent_P(
        PSTR("</h1><div id='nav'>"
             "<button onclick='tab(1)'class='active'>状态</button>"
             "<button onclick='tab(2)'>联网</button>"
             "<button onclick='tab(3)'>控制</button>"
             "<button onclick='tab(4)'>关于</button>"
#ifdef WEB_LOG_SIZE
             "<button onclick='tab(5)'>日志</button>"
#endif
             "</div>"));

    // TAB 1 Start
    uint8_t mode = WiFi.getMode();
    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<div id='tab'>"
                    "<div id='tab1' style='display: block;'>"
                    "<table class='gridtable'><thead><tr><th colspan='2'>WiFi状态</th></tr></thead><tbody>"
                    "<tr><td>主机名</td><td>%s</td></tr>"
                    "<tr><td>WiFi模式</td><td>%s</td></tr>"
                    "<tr><td>SSID</td><td>%s</td></tr>"),
               UID,
               (mode == WIFI_STA ? PSTR("STA") : (mode == WIFI_AP ? PSTR("AP") : PSTR("AP STA"))),
               WiFi.SSID().c_str());
    server->sendContent_P(tmpData);

    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<tr><td>RSSI</td><td>%ddBm</td></tr>"
                    "<tr><td>开机时间</td><td id='uptime'>%s</td></tr>"
                    "<tr><td>空闲内存</td><td><span id='free_mem'>%d</span> kB</td></tr>"
                    "<tr><td>IP地址</td><td>%s</td></tr>"
                    "<tr><td>DHCP</td><td>%s</td></tr>"
                    "</tbody></table>"
                    "</div>"),
               WiFi.RSSI(), Rtc::msToHumanString(millis()).c_str(), ESP.getFreeHeap() / 1024,
               WiFi.localIP().toString().c_str(), (globalConfig.wifi.is_static ? PSTR("静态IP") : PSTR("DHCP")));
    server->sendContent_P(tmpData);
    // TAB 1 End

    // TAB 2 Start
    server->sendContent_P(
        PSTR("<div id='tab2'>"
             "<form method='post' action='/wifi' onsubmit='postform(this);return false'>"
             "<table class='gridtable'><thead><tr><th>WiFi名称</th><th>信号</th></tr></thead><tbody>"
             "<tr id='clusss'><td>WiFi名称</td><td><input type='text' id='wifi_ssid' name='wifi_ssid' placeholder='WiFi名称'></td></tr>"
             "<tr><td>WiFi密码</td><td><input type='text' name='wifi_password' placeholder='WiFi密码'></td></tr>"
             "<tr><td colspan='2'><button type='submit' class='btn-info'>连接WiFi</button></td></tr>"
             "<tr><td colspan='2'><button type='button' class='btn-danger' onclick='scanWifi()'>搜索WiFi</button></td></tr>"
             "</tbody></table></form>"
             "<script type='text/javascript'>function clickwifi(t){id('wifi_ssid').value=t.value}function scanWifi(){ajaxPost('scan_wifi','',function(data){if(data.code==1){if(data.data.list.length==0){scanWifi();return;}var trs=document.getElementsByClassName('addwifi');for(var i=trs.length-1;i>=0;i--){trs[i].remove()}for(var a in data.data.list){var w=data.data.list[a];var tr=document.createElement(\"tr\");var td=document.createElement(\"td\");tr.setAttribute('class','addwifi');td.innerHTML=\"<label class='bui-radios-label'><input type='radio' name='wifi' onclick='clickwifi(this)' value='\"+w.name+\"'/><i class='bui-radios'></i> \"+w.name+(w.type==7?' [开放]':'')+\"</label>\";tr.appendChild(td);td=document.createElement(\"td\");td.innerHTML=w.rssi+'dBm '+w.quality+'%';tr.appendChild(td);var oldEle=id('clusss');oldEle.parentNode.insertBefore(tr,oldEle)}}else{toast(data.msg,data.code?3000:5000,data.code)}})}</script>"));

    server->sendContent_P(
        PSTR("<form method='post' action='/dhcp' onsubmit='postform(this);return false'>"
             "<table class='gridtable'><thead><tr><th colspan='2'>WIFI高级设置</th></tr></thead><tbody>"
             "<tr><td>DHCP</td><td>"
             "<label class='bui-radios-label'><input type='radio' name='dhcp' value='1' onchange='dhcponchange(this)'/><i class='bui-radios'></i> DHCP</label>&nbsp;&nbsp;&nbsp;&nbsp;"
             "<label class='bui-radios-label'><input type='radio' name='dhcp' value='2' onchange='dhcponchange(this)'/><i class='bui-radios'></i> 静态IP</label>"
             "</td></tr>"));

    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<tr class='dhcp_hide'><td>静态IP</td><td><input type='text' name='static_ip' placeholder='静态IP' value='%s'></td></tr>"
                    "<tr class='dhcp_hide'><td>子网掩码</td><td><input type='text' name='static_netmask' placeholder='子网掩码' value='%s'></td></tr>"
                    "<tr class='dhcp_hide'><td>网关</td><td><input type='text' name='static_gateway' placeholder='网关' value='%s'></td></tr>"),
               globalConfig.wifi.ip, globalConfig.wifi.sn, globalConfig.wifi.gw);
    server->sendContent_P(tmpData);

    server->sendContent_P(
        PSTR("<tr><td colspan='2'><button type='submit' class='btn-info'>保存</button></td></tr>"
             "</tbody></table></form>"
             "<script type='text/javascript'>function dhcponchange(the){var v=getRadioValue('dhcp');var dom=document.getElementsByClassName('dhcp_hide');for(var i=0;i<dom.length;i++){dom[i].style.display=v==2?'':'none'}}</script>"));

#ifndef DISABLE_MQTT
    server->sendContent_P(
        PSTR("<form method='post' action='/mqtt' onsubmit='postform(this);return false'>"
             "<table class='gridtable'><thead><tr><th colspan='2'>MQTT设置</th></tr></thead><tbody>"));

    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<tr><td>地址</td><td><input type='text' name='mqtt_server' placeholder='服务器地址' value='%s'></td></tr>"
                    "<tr><td>端口</td><td><input type='number' min='0' max='65535' name='mqtt_port' required value='%d'>&nbsp;&nbsp;&nbsp;&nbsp;0为不启动mqtt</td></tr>"),
               globalConfig.mqtt.server, globalConfig.mqtt.port);
    server->sendContent_P(tmpData);

    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<tr><td>用户名</td><td><input type='text' name='mqtt_username' placeholder='用户名' value='%s'></td></tr>"
                    "<tr><td>密码</td><td><input type='password' name='mqtt_password' placeholder='密码' value='%s'></td></tr>"),
               globalConfig.mqtt.user, globalConfig.mqtt.pass);
    server->sendContent_P(tmpData);

    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<tr><td>主题</td><td><input type='text' name='mqtt_topic' placeholder='主题' value='%s' style='min-width:90%'></td></tr>"
                    "<tr><td>心跳上报间隔</td><td><input type='number' min='0' max='3600' name='interval' required value='%d'>&nbsp;秒&nbsp;&nbsp;0为不上报</td></tr>"),
               globalConfig.mqtt.topic, globalConfig.mqtt.interval);
    server->sendContent_P(tmpData);

    server->sendContent_P(
        PSTR("<tr><td>retain</td><td>"
             "<label class='bui-radios-label'><input type='radio' name='retain' value='0'/><i class='bui-radios'></i> 关闭</label>&nbsp;&nbsp;&nbsp;&nbsp;"
             "<label class='bui-radios-label'><input type='radio' name='retain' value='1'/><i class='bui-radios'></i> 开启</label><br>除非你知道它是干嘛的。"
             "</td></tr>"));

    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<tr><td>状态</td><td id='mqttconnected'>%s</td></tr>"
                    "<tr><td colspan='2'><button type='submit' class='btn-info'>保存</button></td></tr>"
                    "</tbody></table></form>"),
               Mqtt::mqttClient.connected() ? PSTR("已连接") : PSTR("未连接"));
    server->sendContent_P(tmpData);

#ifndef DISABLE_MQTT_DISCOVERY
    server->sendContent_P(
        PSTR("<form method='post' action='/discovery' onsubmit='postform(this);return false'>"
             "<table class='gridtable'><thead><tr><th colspan='2'>MQTT自动发现</th></tr></thead><tbody>"));

    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<tr><td>自发现状态</td><td id='discovery'>%s</td></tr>"
                    "<tr><td>自发现前缀</td><td><input type='text' name='discovery_prefix' placeholder='自发现前缀' required value='%s'></td></tr>"),
               globalConfig.mqtt.discovery ? PSTR("已启动") : PSTR("未启动"), globalConfig.mqtt.discovery_prefix);
    server->sendContent_P(tmpData);

    server->sendContent_P(
        PSTR("<tr><td colspan='2'><button type='submit' class='btn-info' id='discovery_btn'>打开MQTT自动发现</button></td></tr>"
             "</tbody></table></form>"));
#endif
#endif

    server->sendContent_P(PSTR("</div>"));
    // TAB 2 End

    // TAB 3 Start
    server->sendContent_P(PSTR("<div id='tab3'>"));

    if (module)
    {
        module->httpHtml(server);
    }

    server->sendContent_P(
        PSTR("<form method='post' action='/module_setting' onsubmit='postform(this);return false'>"
             "<table class='gridtable'><thead><tr><th colspan='2'>模块设置</th></tr></thead><tbody>"));

    snprintf_P(tmpData, sizeof(tmpData), PSTR("<tr><td>主机名</td><td><input type='text' name='uid' value='%s'>&nbsp;具有唯一性，留空默认</td></tr>"), UID);
    server->sendContent_P(tmpData);

    server->sendContent_P(
        PSTR("<tr><td>日志输出</td><td>"
             "<label class='bui-radios-label'><input type='checkbox' name='log_serial' value='1'/><i class='bui-radios' style='border-radius:20%'></i> Serial</label>&nbsp;&nbsp;&nbsp;&nbsp;"
             "<label class='bui-radios-label'><input type='checkbox' name='log_serial1' value='1'/><i class='bui-radios' style='border-radius:20%'></i> Serial1</label>&nbsp;&nbsp;&nbsp;&nbsp;"
#ifdef USE_SYSLOG
             "<label class='bui-radios-label'><input type='checkbox' name='log_syslog' value='1'/><i class='bui-radios' style='border-radius:20%'></i> syslog</label>&nbsp;&nbsp;&nbsp;&nbsp;"
#endif
#ifdef WEB_LOG_SIZE
             "<label class='bui-radios-label'><input type='checkbox' name='log_web' value='1'/><i class='bui-radios' style='border-radius:20%'></i> web</label>&nbsp;&nbsp;&nbsp;&nbsp;"
#endif
             "</td></tr>"));

#ifdef USE_SYSLOG
    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<tr><td>syslog服务器</td><td>"
                    "<input type='text' name='log_syslog_host' style='width:150px' value='%s'> : "
                    "<input type='number' name='log_syslog_port' value='%d' min='0' max='65000' style='width:50px'>"
                    "</td></tr>"),
               globalConfig.debug.server, globalConfig.debug.port);
    server->sendContent_P(tmpData);
#endif

    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<tr><td>NTP服务器</td><td>"
                    "<input type='text' name='ntp' style='width:150px' value='%s'> 建议在获取时间失败时才填写"
                    "</td></tr>"),
               globalConfig.wifi.ntp);
    server->sendContent_P(tmpData);

    server->sendContent_P(
        PSTR("<tr><td colspan='2'><button type='submit' class='btn-info'>设置</button></td></tr>"
             "</tbody></table></form>"
             "<div>"
             "<button type='button' class='btn-danger' style='margin-top: 10px' onclick=\"javascript:if(confirm('确定要重启模块？')){ajaxPost('/operate', 'd=1');}\">重启模块</button>"
             "<button type='button' class='btn-danger' style='margin-top: 10px' onclick=\"javascript:if(confirm('确定要重置模块？')){ajaxPost('/operate', 'd=2');}\">重置模块</button>"
             "</div>"
             "</div>"));
    // TAB 3 End

    // TAB 4 Start
    server->sendContent_P(
        PSTR("<div id='tab4'>"
             "<table class='gridtable'><thead><tr><th colspan='2'>硬件参数</th></tr></thead><tbody>"));

    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<tr><td>ESP芯片ID</td><td>%d</td></tr>"
                    "<tr><td>Flash芯片 ID</td><td>%d</td></tr>"
                    "<tr><td>Flash大小</td><td>%d kB</td></tr>"
                    "<tr><td>固件Flash大小</td><td>%d kB</td></tr>"
                    "<tr><td>固件大小</td><td>%d kB</td></tr>"),
               ESP.getChipId(), ESP.getFlashChipId(), ESP.getFlashChipRealSize() / 1024, ESP.getFlashChipSize() / 1024, ESP.getSketchSize() / 1024);
    server->sendContent_P(tmpData);

    uint8_t mac[6];
    wifi_get_macaddr(STATION_IF, mac);
    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<tr><td>空闲程序空间</td><td>%d kB</td></tr>"
                    "<tr><td>内核和SDK版本</td><td>" ARDUINO_ESP8266_RELEASE "%s</td></tr>"
                    "<tr><td>重启原因</td><td>%s</td></tr>"
                    "<tr><td>MAC地址</td><td>%02X:%02X:%02X:%02X:%02X:%02X</td></tr>"
                    "</tbody></table>"),
               ESP.getFreeSketchSpace() / 1024, ESP.getSdkVersion(), ESP.getResetReason().c_str(), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    server->sendContent_P(tmpData);

    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("<table class='gridtable'><thead><tr><th colspan='2'>固件升级</th></tr></thead><tbody>"
                    "<tr><td>当前版本</td><td>v%s</td></tr>"
                    "<tr><td>编译时间</td><td>%s</td></tr>"),
               module ? module->getModuleVersion().c_str() : PSTR("0"), Rtc::GetBuildDateAndTime().c_str());
    server->sendContent_P(tmpData);

    server->sendContent_P(
        PSTR("<form method='POST' action='/update' enctype='multipart/form-data'>"
             "<tr><td colspan='2'><a class='file'><input type='file' name='update'>选择文件</a></td></tr>"
             "<tr><td colspan='2'><button type='submit' class='btn-info'>升级</button><br>"
             "</form>"
             "<tr><td colspan='2' style='text-align:center'>OTA更新</td></tr>"
             "<form method='POST' action='/ota' onsubmit='postform(this);return false'>"
             "<tr><td>OTA地址</td><td><input type='text' name='ota_url' placeholder='OTA地址' value='" OTA_URL "' style='width:98%'></td></tr>"
             "<tr><td colspan='2'><button type='submit' class='btn-success' onclick=\"return confirm('确定要OTA更新？')\">OTA更新</button></td></tr>"
             "</form>"
             "</tbody></table>"
             "</div>"));
    // TAB 4 End

#ifdef WEB_LOG_SIZE
    // TAB 5 Start
    server->sendContent_P(
        PSTR("<div id='tab5'>"
             "<div style='display:inline-block;color:#000000;min-width:340px;position:absolute;left:1%;margin-top:20px;width:99%'>"
             "<textarea readonly id='log' cols='340' wrap='off' style='resize:none;width:98%;height:600px;padding:5px;overflow:auto;background:#ffffff;color:#000000;'></textarea>"
             "</div></div>"));
// TAB 5 End
#endif

    server->sendContent_P(
        PSTR("</div><div style='text-align:center;margin-top:20px'>开发者：<a href='https://github.com/qlwz' target='_blank' style='color:#333;text-decoration:none'>情留メ蚊子</a>&nbsp;&nbsp;&nbsp;<a href='https://bbs.iobroker.cn' target='_blank' style='color:#333;text-decoration:none'>来和大神一起玩智能家居</a></div><div></body></html>"));

    // TAB 2
    snprintf_P(tmpData, sizeof(tmpData), PSTR("<script type='text/javascript'>%ssetRadioValue('dhcp', '%d');dhcponchange(null);"),
               WiFi.isConnected() ? PSTR("") : PSTR("scanWifi();"), globalConfig.wifi.is_static ? 2 : 1);
    server->sendContent_P(tmpData);

#ifndef DISABLE_MQTT
    snprintf_P(tmpData, sizeof(tmpData), PSTR("setRadioValue('retain', '%d');"), globalConfig.mqtt.retain ? 1 : 0);
    server->sendContent_P(tmpData);

#ifndef DISABLE_MQTT_DISCOVERY
    if (globalConfig.mqtt.discovery)
    {
        server->sendContent_P(PSTR("id('discovery_btn').setAttribute('class', 'btn-danger');id('discovery_btn').innerHTML='关闭MQTT自动发现';"));
    }
#endif
#endif
    // TAB 3
    if ((1 & globalConfig.debug.type) == 1)
    {
        server->sendContent_P(PSTR("setRadioValue('log_serial', '1');"));
    }
#ifdef USE_SYSLOG
    if ((2 & globalConfig.debug.type) == 2)
    {
        server->sendContent_P(PSTR("setRadioValue('log_syslog', '1');"));
    }
#endif
#ifdef WEB_LOG_SIZE
    if ((4 & globalConfig.debug.type) == 4)
    {
        server->sendContent_P(PSTR("setRadioValue('log_web', '1');"));
    }
#endif
    if ((8 & globalConfig.debug.type) == 8)
    {
        server->sendContent_P(PSTR("setRadioValue('log_serial1', '1');"));
    }
    server->sendContent_P(PSTR("</script>"));
}

#ifndef DISABLE_MQTT
void Http::handleMqtt()
{
    if (!checkAuth())
    {
        return;
    }
    String topic = server->arg(F("mqtt_topic"));
    if (topic.length() == 0)
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":0,\"msg\":\"MQTT主题不能为空\"}"));
        return;
    }
    if (topic.indexOf("%prefix%/") == 0)
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":0,\"msg\":\"MQTT主题必须包含【%prefix%/】\"}"));
        return;
    }
    strcpy(globalConfig.mqtt.server, server->arg(F("mqtt_server")).c_str());
    globalConfig.mqtt.port = server->arg(F("mqtt_port")).toInt();
    globalConfig.mqtt.retain = server->arg(F("retain")) == F("1");
    strcpy(globalConfig.mqtt.user, server->arg(F("mqtt_username")).c_str());
    strcpy(globalConfig.mqtt.pass, server->arg(F("mqtt_password")).c_str());
    strcpy(globalConfig.mqtt.topic, topic.c_str());
    globalConfig.mqtt.interval = server->arg(F("interval")).toInt();
    Config::saveConfig();

    if (Mqtt::mqttClient.connected())
    {
        Mqtt::mqttClient.disconnect();
    }

    if (Mqtt::mqttConnect())
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"设置MQTT服务器成功，已连接。\",\"data\":{\"mqttconnected\":\"已连接\"}}"));
    }
    else
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"设置MQTT服务器成功，未连接。\",\"data\":{\"mqttconnected\":\"未连接\"}}"));
    }
}

#ifndef DISABLE_MQTT_DISCOVERY
void Http::handleDiscovery()
{
    if (!checkAuth())
    {
        return;
    }
    strcpy(globalConfig.mqtt.discovery_prefix, server->arg(F("discovery_prefix")).c_str());
    globalConfig.mqtt.discovery = !globalConfig.mqtt.discovery;
    Config::saveConfig();

    if (module)
    {
        module->mqttDiscovery(globalConfig.mqtt.discovery);
    }
    if (globalConfig.mqtt.discovery)
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"已经打开MQTT自发现。\",\"data\":{\"discovery\":1}}"));
    }
    else
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"已经关闭MQTT自发现。\",\"data\":{\"discovery\":0}}"));
    }
}
#endif
#endif

void Http::handledhcp()
{
    if (!checkAuth())
    {
        return;
    }
    String ip = server->arg(F("static_ip"));
    String netmask = server->arg(F("static_netmask"));
    String gateway = server->arg(F("static_gateway"));
    if (!Wifi::isIp(ip))
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":0,\"msg\":\"IP地址错误\"}"));
        return;
    }
    if (!Wifi::isIp(netmask))
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":0,\"msg\":\"掩码地址错误\"}"));
        return;
    }
    if (!Wifi::isIp(gateway))
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":0,\"msg\":\"网关地址错误\"}"));
        return;
    }

    IPAddress static_ip;
    IPAddress static_sn;
    IPAddress static_gw;
    static_ip.fromString(ip);
    static_sn.fromString(netmask);
    static_gw.fromString(gateway);

    if (!(static_ip.isV4() && static_sn.isV4() && (!static_gw.isSet() || static_gw.isV4())))
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":0,\"msg\":\"IP地址或者网关错误\"}"));
        return;
    }

    if ((static_ip.v4() & static_sn.v4()) != (static_gw.v4() & static_sn.v4()))
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":0,\"msg\":\"网段错误\"}"));
        return;
    }

    bool old = globalConfig.wifi.is_static;
    globalConfig.wifi.is_static = server->arg(F("dhcp")).equals(F("2"));
    strcpy(globalConfig.wifi.ip, ip.c_str());
    strcpy(globalConfig.wifi.sn, netmask.c_str());
    strcpy(globalConfig.wifi.gw, gateway.c_str());
    Config::saveConfig();

    if (old != globalConfig.wifi.is_static)
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"设置DHCP信息成功，重启后生效\"}"));
    }
    else
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"设置DHCP信息成功\"}"));
    }
}

void Http::handleScanWifi()
{
    if (!checkAuth())
    {
        return;
    }
    int n = WiFi.scanNetworks();
    if (n == 0)
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"\",\"data\":{\"list\":[]}}"));
        //server->send_P(200, PSTR("text/html"), PSTR("{\"code\":0,\"msg\":\"找不到网络，请重新试试。\"}"));
        return;
    }

    //sort networks
    int indices[n];
    for (int i = 0; i < n; i++)
    {
        indices[i] = i;
    }

    // RSSI排序
    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i]))
            {
                std::swap(indices[i], indices[j]);
            }
        }
    }

    // 删除重复项（必须对RSSI进行排序）
    String cssid;
    for (int i = 0; i < n; i++)
    {
        if (indices[i] == -1)
            continue;
        cssid = WiFi.SSID(indices[i]);
        for (int j = i + 1; j < n; j++)
        {
            if (cssid == WiFi.SSID(indices[j]))
            {
                indices[j] = -1; // set dup aps to index -1
            }
        }
    }

    int _minimumQuality = -1;
    int quality;
    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"\",\"data\":{\"list\":["));
    for (int i = 0; i < n; i++)
    {
        if (indices[i] == -1)
            continue; // skip dups
        int RSSI = WiFi.RSSI(indices[i]);
        if (RSSI <= -100)
        {
            quality = 0;
        }
        else if (RSSI >= -50)
        {
            quality = 100;
        }
        else
        {
            quality = 2 * (RSSI + 100);
        }
        if (_minimumQuality == -1 || _minimumQuality < quality)
        {
            if (i != 0)
            {
                server->sendContent_P(PSTR(","));
            }
            snprintf_P(tmpData, sizeof(tmpData), PSTR("{\"name\":\"%s\",\"rssi\":%d,\"quality\":%d,\"type\":%d}"), WiFi.SSID(indices[i]).c_str(), RSSI, quality, indices[i]);
            server->sendContent_P(tmpData);
        }
    }

    server->sendContent_P(PSTR("]}}"));
}

void Http::handleWifi()
{
    if (!checkAuth())
    {
        return;
    }
    String wifi = server->arg(F("wifi_ssid"));
    if (wifi == "")
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":0,\"msg\":\"WiFi名称不能为空。\"}"));
        return;
    }
    String password = server->arg(F("wifi_password"));

    if (WiFi.getMode() == WIFI_STA)
    {
        strcpy(globalConfig.wifi.ssid, wifi.c_str());
        strcpy(globalConfig.wifi.pass, password.c_str());
        Config::saveConfig();
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"设置WiFi信息成功，重启模块（手动）使用新的Wifi信息连接。\"}"));
    }
    else
    {
        Wifi::tryConnect(wifi, password);
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"尝试将ESP连接到网络。 如果失败，请重新连接到AP再试一次。\"}"));
    }
}

void Http::handleOperate()
{
    if (!checkAuth())
    {
        return;
    }
    String d = server->arg(F("d")); // 1：重启模块 2：重置模块
    if (d == F("1"))
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"设备正在重启 . . .\"}"));
    }
    else if (d == F("2"))
    {
        Config::resetConfig();
        Config::saveConfig();
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"正在重置模块 . . . 设备将会重启。\"}"));
    }
    delay(200);

    Led::blinkLED(400, 4);
    ESP.restart();
}

void Http::handleOTA()
{
    if (!checkAuth())
    {
        return;
    }
    strcpy(globalConfig.http.ota_url, server->arg(F("ota_url")).c_str());
    server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"如果成功后设备会重启 . . . \"}"));
    Http::OTA(String(globalConfig.http.ota_url));
}

void Http::handleNotFound()
{
    if (captivePortal())
    {
        return;
    }
    server->sendHeader(PSTR("Cache-Control"), PSTR("no-cache, no-store, must-revalidate"));
    server->sendHeader(PSTR("Pragma"), PSTR("no-cache"));
    server->sendHeader(PSTR("Expires"), PSTR("-1"));
    server->setContentLength(CONTENT_LENGTH_UNKNOWN);

    snprintf_P(tmpData, sizeof(tmpData), PSTR("File Not Found\n\nURI: %s\nMethod: %s\nArguments: %d\n"),
               server->uri().c_str(), server->method() == HTTP_GET ? PSTR("GET") : PSTR("POST"), server->args());
    server->send_P(404, PSTR("text/plain"), tmpData);
    for (uint8_t i = 0; i < server->args(); i++)
    {
        snprintf_P(tmpData, sizeof(tmpData), PSTR(" %s: %s\n"), server->argName(i).c_str(), server->arg(i).c_str());
        server->sendContent_P(tmpData);
    }
}

void Http::handleGetStatus()
{
    if (!checkAuth())
    {
        return;
    }

    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"\",\"data\":{"));

    snprintf_P(tmpData, sizeof(tmpData), PSTR("\"uptime\":\"%s\",\"free_mem\":%d"), Rtc::msToHumanString(millis()).c_str(), ESP.getFreeHeap() / 1024);
    server->sendContent_P(tmpData);

#ifndef DISABLE_MQTT
    if (Mqtt::mqttClient.connected())
    {
        server->sendContent_P(PSTR(",\"mqttconnected\":\"已连接\""));
    }
    else
    {
        server->sendContent_P(PSTR(",\"mqttconnected\":\"未连接\""));
    }

#ifndef DISABLE_MQTT_DISCOVERY
    snprintf_P(tmpData, sizeof(tmpData), PSTR(",\"discovery\":%d"), globalConfig.mqtt.discovery ? 1 : 0);
    server->sendContent_P(tmpData);
#endif
#endif

    if (Wifi::configPortalStart == 0 && WiFi.isConnected())
    {
        snprintf_P(tmpData, sizeof(tmpData), PSTR(",\"ip\":\"%s\""), WiFi.localIP().toString().c_str());
        server->sendContent_P(tmpData);
    }

    if (module)
    {
        String tmp = module->httpGetStatus(server);
        if (tmp.length() > 0)
        {
            server->sendContent_P(PSTR(","));
            server->sendContent(tmp);
        }
    }

#ifdef WEB_LOG_SIZE
    bool cflg = true;
    uint8_t counter = 0;
    if (server->hasArg(F("i")))
    {
        counter = server->arg(F("i")).toInt();
    }
    snprintf_P(tmpData, sizeof(tmpData), PSTR(",\"logindex\":%d,\"log\":\""), Debug::webLogIndex);
    server->sendContent_P(tmpData);
    if (counter != Debug::webLogIndex)
    {
        if (!counter)
        {
            counter = Debug::webLogIndex;
            cflg = false;
        }
        do
        {
            char *tmp;
            uint16_t len;
            Debug::GetLog(counter, &tmp, &len);
            if (len)
            {
                if (cflg)
                {
                    server->sendContent_P(PSTR("\\n"));
                }

                size_t j = 0;
                for (size_t i = 0; i < len - 1; i++)
                {
                    char each = tmp[i];
                    if (each == '\\' || each == '"')
                    {
                        tmpData[j++] = '\\';
                        tmpData[j++] = each;
                    }
                    else if (each == '\b')
                    {
                        tmpData[j++] = '\\';
                        tmpData[j++] = 'b';
                    }
                    else if (each == '\f')
                    {
                        tmpData[j++] = '\\';
                        tmpData[j++] = 'f';
                    }
                    else if (each == '\n')
                    {
                        tmpData[j++] = '\\';
                        tmpData[j++] = 'n';
                    }
                    else if (each == '\r')
                    {
                        tmpData[j++] = '\\';
                        tmpData[j++] = 'r';
                    }
                    else if (each == '\t')
                    {
                        tmpData[j++] = '\\';
                        tmpData[j++] = 't';
                    }
                    else
                    {
                        tmpData[j++] = each;
                    }
                }
                tmpData[j++] = '\0';

                server->sendContent_P(tmpData);
                cflg = true;
            }
            counter++;
            if (!counter)
            {
                counter++;
            } // Skip log index 0 as it is not allowed
        } while (counter != Debug::webLogIndex);
    }
    server->sendContent_P(PSTR("\"}}"));
#else
    server->sendContent_P(PSTR("}}"));
#endif
}

void Http::handleUpdate()
{
    // handler for the /update form POST (once file upload finishes)
    server->on(
        F("/update"), HTTP_POST, [&]() {
        if (!checkAuth())
        {
            return;
        } 
        if (Update.hasError())
        {
            Debug::AddError(PSTR("Update error: %s"), Http::updaterError.c_str());
            server->send(200, F("text/html"), String(F("Update error: ")) + Http::updaterError);
        }
        else
        {
            Config::saveConfig();
            server->client().setNoDelay(true);
            server->send_P(200, PSTR("text/html"), PSTR("<meta charset='utf-8'/><meta http-equiv=\"refresh\" content=\"15;URL=/\">升级成功！正在重启 . . ."));
            delay(100);
            server->client().stop();
            ESP.restart();
        } }, [&]() {
                HTTPUpload &upload = server->upload();
                if (upload.status == UPLOAD_FILE_START)
                {
                    Http::updaterError = String();
                    if (globalConfig.http.user[0] != 0 && globalConfig.http.pass[0] != 0 && server->client().localIP().toString() != "192.168.4.1" && !server->authenticate(globalConfig.http.user, globalConfig.http.pass))
                    {
                        Debug::AddInfo(PSTR("Unauthenticated Update"));
                        return;
                    }
                    WiFiUDP::stopAll();
                    Debug::AddInfo(PSTR("Update: %s"), upload.filename.c_str());
                    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                    if (!Update.begin(maxSketchSpace, U_FLASH))//start with max available size
                    { 
                        StreamString str;
                        Update.printError(str);
                        Http::updaterError = str.c_str();
                    }
                }
                else if (upload.status == UPLOAD_FILE_WRITE && !Http::updaterError.length())
                {
                    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                    {
                        StreamString str;
                        Update.printError(str);
                        Http::updaterError = str.c_str();
                    }
                }
                else if (upload.status == UPLOAD_FILE_END && !Http::updaterError.length())
                {
                    if (Update.end(true))
                    { 
                        Debug::AddInfo(PSTR("Update Success: %u   Rebooting..."), upload.totalSize);
                    }
                    else
                    {
                        StreamString str;
                        Update.printError(str);
                        Http::updaterError = str.c_str();
                    }
                }
                else if (upload.status == UPLOAD_FILE_ABORTED)
                {
                    Update.end();
                    Debug::AddInfo(PSTR("Update was aborted"));
                }
                delay(0); });
}

void Http::begin()
{
    if (isBegin)
    {
        return;
    }
    isBegin = true;
    server = new ESP8266WebServer();

    server->on(F("/"), handleRoot);
#ifndef DISABLE_MQTT
    server->on(F("/mqtt"), handleMqtt);
#ifndef DISABLE_MQTT_DISCOVERY
    server->on(F("/discovery"), handleDiscovery);
#endif
#endif
    server->on(F("/dhcp"), handledhcp);
    server->on(F("/scan_wifi"), handleScanWifi);
    server->on(F("/wifi"), handleWifi);
    server->on(F("/operate"), handleOperate);
    server->on(F("/module_setting"), handleModuleSetting);
    server->on(F("/ota"), handleOTA);
    server->on(F("/get_status"), handleGetStatus);
    server->onNotFound(handleNotFound);
    handleUpdate();

    if (module)
    {
        module->httpAdd(server);
    }
    server->begin(globalConfig.http.port);
    Debug::AddInfo(PSTR("HTTP server started port: %d"), globalConfig.http.port);
}

void Http::stop()
{
    if (!isBegin)
    {
        return;
    }
    server->stop();
    Debug::AddInfo(PSTR("HTTP server stoped"));
}

void Http::loop()
{
    if (isBegin)
    {
        server->handleClient();
    }
}

bool Http::captivePortal()
{
    if (!Wifi::isIp(server->hostHeader()))
    {
        //Debug::AddInfo(PSTR("Request redirected to captive portal"));
        server->sendHeader(F("Location"), String(F("http://")) + server->client().localIP().toString(), true);
        server->send(302, F("text/plain"), ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
        server->client().stop();                // Stop is needed because we sent no content length
        return true;
    }
    return false;
}

void Http::handleModuleSetting()
{
    if (!checkAuth())
    {
        return;
    }

    int t = 0;
    if (server->arg(F("log_serial")).equals(F("1")))
    {
        t = t | 1;
    }
    if (server->arg(F("log_serial1")).equals(F("1")))
    {
        t = t | 8;
    }
#ifdef WEB_LOG_SIZE
    if (server->arg(F("log_web")).equals(F("1")))
    {
        t = t | 4;
    }
#endif

#ifdef USE_SYSLOG
    if (server->arg(F("log_syslog")).equals(F("1")))
    {
        t = t | 2;
        String log_syslog_host = server->arg(F("log_syslog_host"));
        String log_syslog_port = server->arg(F("log_syslog_port"));
        if (log_syslog_host.length() == 0)
        {
            server->send_P(200, PSTR("text/html"), PSTR("{\"code\":0,\"msg\":\"syslog服务器不能为空\"}"));
            return;
        }
        strcpy(globalConfig.debug.server, log_syslog_host.c_str());
        globalConfig.debug.port = log_syslog_port.toInt();
        WiFi.hostByName(globalConfig.debug.server, Debug::ip);
    }
#endif

    globalConfig.debug.type = t;
    if ((8 & globalConfig.debug.type) == 8)
    {
        Serial1.begin(115200);
    }

    String ntp = server->arg(F("ntp"));
    if (strcmp(globalConfig.wifi.ntp, ntp.c_str()) != 0)
    {
        strcpy(globalConfig.wifi.ntp, ntp.c_str());
        Rtc::init();
    }

    String uid = server->arg(F("uid"));
    strcpy(globalConfig.uid, uid.c_str());
    Config::saveConfig();
    if (uid.length() == 0 || strcmp(globalConfig.uid, UID) != 0)
    {
#ifndef DISABLE_MQTT
#ifndef DISABLE_MQTT_DISCOVERY
        if (globalConfig.mqtt.discovery && module)
        {
            module->mqttDiscovery(false);
        }
#endif
#endif
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"修改了重要配置 . . . 正在重启中。\"}"));
        Led::blinkLED(400, 4);
        ESP.restart();
    }
    else
    {
        server->send_P(200, PSTR("text/html"), PSTR("{\"code\":1,\"msg\":\"已经修改成功\"}"));
    }
}

bool Http::checkAuth()
{
    if (globalConfig.http.user[0] != 0 && globalConfig.http.pass[0] != 0 && server->client().localIP().toString() != F("192.168.4.1"))
    {
        if (!server->authenticate(globalConfig.http.user, globalConfig.http.pass))
        {
            server->requestAuthentication();
            return false;
        }
    }
    return true;
}

void Http::OTA(String url)
{
    if (url.indexOf(F("%04d")) != -1)
    {
        url.replace(F("%04d"), String(ESP.getChipId() & 0x1fff));
    }
    else if (url.indexOf(F("%d")) != -1)
    {
        url.replace(F("%d"), String(ESP.getChipId()));
    }
    url.replace(F("%hostname%"), UID);
    url.replace(F("%module%"), module ? module->getModuleName() : F(""));

    Config::saveConfig();
    Debug::AddInfo(PSTR("OTA Url: %s"), url.c_str());
    Led::blinkLED(200, 5);
    WiFiClient OTAclient;
    if (ESPhttpUpdate.update(OTAclient, url, (module ? module->getModuleVersion() : F(""))) == HTTP_UPDATE_FAILED)
    {
        Debug::AddError(PSTR("HTTP_UPDATE_FAILD Error (%d): %s"), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    }
}
