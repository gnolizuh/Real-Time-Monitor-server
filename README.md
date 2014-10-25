#Real-Time Monitor server
==========================

## 功能简介

负责媒体流和控制流转发, 并大量使用c++11特性

### 高性能

使用libevent提供的高性能IO, 内部实现了多线程

### 跨平台

使用pjsip封装的LINUX/WINDOWS基本类型和各种函数, 轻松实现跨平台

## 参考、使用的开源项目
* [Libevent](https://github.com/nmathewson/Libevent) ([BSD License](https://github.com/nmathewson/Libevent/blob/master/LICENSE))
* [Pjsip](http://www.pjsip.org/) ([GPL v2 License](http://www.pjsip.org/licensing.htm))
