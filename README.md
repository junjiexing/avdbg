avdbg
=====

仿OD的调试器,现已实现单步步入（F7）、单步步过（F8）、运行（F9），忽略异常的单步步入（Shift+F7）、单步步过（Shift+F8）、运行（Shift+F9），断点（F2），汇编功能（space），十六进制内存查看，堆栈查看等基本调试功能。


欢迎有兴趣的童鞋一起来进行开发，大家一起努力打造一个开源的OD，并超越OD。

编译方法：
 
 环境：VS2012、Intel C++ Composer XE 2013
 
 步骤：

	1.git更新源码。由于界面库Xtreme toolkit pro 15.3.1是商业软件，不能开源，所以我把这个界面库作为子模块放到了私有仓库，大家可以自己想办法解决这个界面库的源码问题，解决不了的可以加群讨论。
       
	2.如果问我要了私有仓库的密码，首先更新子模块，然后编译3rdparty\Scintilla\Scintilla.sln,然后编译Xtreme toolkit pro，都完成后打开KillDBG.sln进行编译。如果自己下载了XTP的源码，则需要修改KillDBG项目的链接属性，将里面的ToolkitPro.lib改为自己编译的XTP的库文件
	   
	3.下载boost库http://www.boost.org/并解压编译和配置（关于编译和配置boost的方法我就不赘述了，网上很多了）
	
编译好的：http://pan.baidu.com/s/1gQFLQ
       

讨论群：
  
  DdvpDbg  127285697  一个开源的OD的VT插件DdvpDbg的讨论群。
  
  avplayer.org  3597082  avplayer开源社区的群，主要讨论C++、Boost，社区有播放器、QQ聊天机器人、苹果移动设备的开源库等多个开源项目
  
  水群  149462056  我为这个调试器建的一个群，不过人很少，欢迎大家来扯淡。
  
