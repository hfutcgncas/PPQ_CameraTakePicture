用法：
1、打开ttermpro.exe,连上要用的摄像机，上传“.\摄像机配套程序\liutry.msf”

2、在ttermpro.exe中启动“liutry” 之后按任意键

3、选择配套的摄像机：  修改“.\script\__main__.py”文件中的倒数第二行
	cameraIPList = [('picXX','XX'),('picYY','YY'), ... ] 
	其中XX，YY为摄像机IP最后两位 ，
		例如若只用88的话，设为cameraIPList = [('pic88','88') ] 
			用88,89,98的话，设为cameraIPList = [('pic88','88')，('pic89','89')，('pic98','98') ] 

4、运行.\script\__main__.py   注意只能直接双击运行，不能再IDE里运行，因为在IDE中无法捕捉键盘中断

5、得到的结果在“.\script\pic文件夹内”文件夹内




涉及： python多线程
1，多线程
2，TCPIP
3，键盘监控