import socket
from PIL import Image
import msvcrt
import time
import threading



def kbfunc(): 
   x = msvcrt.kbhit()
   if x: 
      ret = ord(msvcrt.getch()) 
   else: 
      ret = 0 
   return ret
#------------------------------------------------------------------
class cCamera(threading.Thread):
    x = 0
    dy = 0
    sock = 0
    cameraName = 0
    cameraIP = 0
    thread_stop = False
    tp_flag = False

    index = 0
    outDatapath = 0
    outPicpath = 0
    
    def __init__(self,cameraName,cameraIP):
        threading.Thread.__init__(self)

        self.thread_stop = False
        
        self.dx = 600
        self.dy = 480
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect(('192.168.64.'+ cameraIP, 2002))
        self.cameraName = cameraName
        self.cameraIP = cameraIP
        
    def run(self): #Override run() methord, com to camera
        print("ECS to stop,  t to takepic")
        while not self.thread_stop:
            if not self.tp_flag:
                str1='a'
                str2='\x02'
                send_str = "".join([str1,str2])
                self.sock.send(send_str)
            else:
                self.takePic(self.index,self.outDatapath,self.outPicpath)
                self.tp_flag = False 
            time.sleep(0.01)
                
    def takePic(self,index,outDatapath,outPicpath):
        self.sock.send("TC")
        #-------------------------
        str1=chr(0)
        str2='\x02'
        send_str = "".join([str1,str2])
        self.sock.send(send_str)
        data_rec=self.sock.recv(self.dx*self.dy+28)
        data_change=[]
        for x in data_rec:
                data_change.append( "%i"%(ord(x)) )
        datastr = " ".join(data_change)
        filename="Data.txt"
        filename=self.cameraName + filename
        filename = outDatapath + filename
        f = open(filename,'wb')
        f.write(datastr)
        f.close()
    
        send_str = 'en'
        self.sock.send(send_str)

        #-------------------------
        #convert to bmp  
        self.convertToImg(self.cameraName,outDatapath,outPicpath)    
        
    def close(self):
        self.thread_stop = True
        self.sock.close()

    def convertToImg(self,name,outDatapath,outPicpath):
        try:                                      
            fobj=open(outDatapath+name+"Data.txt",'r')            
        except IOError:                           
            print '*** file open error:'
        else:                                     
            imvalStr = fobj.readline() 
            fobj.close
        imval = map(float, imvalStr.split(' '))
        for data in imval:
            data = data/255.0
        im = Image.new('L', [600,480]); 
        k = 0
        for j in range(0,480):
            for i in range(0,600):
                im.putpixel((i,j),imval[k] )
                k = k+1
        im.save(outPicpath+name+".bmp","BMP")
        

	
