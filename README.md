# ESP DC1
**斐讯DC1智能排插个人固件.**

## WHY
众所周知的原因，斐讯服务器已经不能正常访问，插座的APP控制已经无法正常实现，需要有另外的方式实现插座的控制。

已有的方法为内网劫持实现，具体可参考[这里](https://bbs.hassbian.com/thread-5637-1-1.html)。

这次要实现的是通过一个自定义的固件，来完整实现DC1联网控制。

> ### 作者声明
>
> 注意: 本项目主要目的为作者本人自己学习及使用DC1插线板而开发，本着开源精神及造福网友而开源，仅个人开发，可能无法做到完整的测试，所以不承担他人使用本项目照成的所有后果。
>
> **严禁他人将本项目用户用于任何商业活动。个人在非盈利情况下可以自己使用，严禁收费代刷等任何盈利服务、**
> 
> 有需要请联系作者：qlwz@qq.com


## 特性

本固件使用斐讯DC1插线板硬件为基础,实现以下功能:

- [x] 4个USB充电
- [x] 按键控制所有插口通断
- [x] 控制每个接口独立开关
- [x] OTA在线升级
- [x] WEB配置页面
- [x] MQTT服务器连接控制
- [x] 通过MQTT连入Home Assistant
- [x] 电压/电流/功率/视在功率/无功功率/功率因数/用电量统计(不做任何精度保证)
- [x] 过载保护


## 拆机接线及烧录固件相关

见[固件烧录](固件烧录.md)

烧录固件完成后,即可开始使用

## 总开关控制USB

1、抖音 搜索 DC1 按照李老师的硬件改造

2、WEB页面 分开关联动 设置为不联动

## 如何配网

1、第一次使用自动进入配网模式

2、以后通过长按【总开关】进入配网模式

## 如何编译
Visual Studio Code + PlatformIO ID 开发  [安装](https://www.jianshu.com/p/c36f8be8c87f)

## 已支持接入的开源智能家居平台
以下排序随机，不分优劣。合适自己的就好。

### 1、Home Assistant
Home Assistant 是一款基于 Python 的智能家居开源系统，支持众多品牌的智能家居设备，可以轻松实现设备的语音控制、自动化等。
- [官方网站](https://www.home-assistant.io/)
- [国内论坛](https://bbs.hassbian.com/)

#### 接入方法
WEB页面开启**MQTT自动发现**  

### 2、ioBroker
ioBroker是基于nodejs的物联网的集成平台，为物联网设备提供核心服务、系统管理和统一操作方式。
- [官方网站](http://www.iobroker.net)
- [中文资料可以参考这里](https://doc.iobroker.cn/#/_zh-cn/)
- [国内论坛](https://bbs.iobroker.cn)
#### 接入方法
ioBroker相关接入问题可以加QQ群776817275咨询

### 3、其他支持mqtt的平台
理论上来说，只要是支持MQTT的平台都可以实现接入。

#### 接入方法
添加对应的topic

# 固件截图

![image](https://github.com/qlwz/esp_dc1/blob/master/file/images/tab1.png)
![image](https://github.com/qlwz/esp_dc1/blob/master/file/images/tab2.png)
![image](https://github.com/qlwz/esp_dc1/blob/master/file/images/tab3.png)
![image](https://github.com/qlwz/esp_dc1/blob/master/file/images/tab4.png)

## 致谢
以下排名不分先后，为随机。
- killadm：导出原始固件，提供WiFi芯片对比图，主控制板WiFi模块、U7移除后的PCB照片，U7逻辑分析数据采集
- 老妖：U7驱动编写，U7逻辑分析
- 实验幼儿园小二班扛把子：测试引脚走向
- Heller、巴山耗子：初期资料整理
- 风中的summer：提供清晰的电路板照片、拆机过程照片

感谢各位使用本方法的玩家，欢迎加入QQ群776817275

## 免责申明
以上纯属个人爱好，因为使用上述方法造成的任何问题，不承担任何责任。

部分图片来源于网络，如果涉及版权，请通知删除。