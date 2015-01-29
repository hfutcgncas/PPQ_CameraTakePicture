import cCameraIf as cCam
import msvcrt
import time
import os
import shutil

def kbfunc(): 
   x = msvcrt.kbhit()
   if x: 
      ret = ord(msvcrt.getch()) 
   else: 
      ret = 0 
   return ret

def prepareFilePath(rootdir,isClean):
   if not os.path.isdir(rootdir):
      os.makedirs(rootdir)
   if isClean:
      filelist = []
      filelist = os.listdir(rootdir)
      for f in filelist:
         filepath = os.path.join(rootdir,f)
         if os.path.isfile(filepath):
            os.remove(filepath)
            print(filepath + " removed")
         elif os.path.isdir(filepath):
            shutil.rmtree(filepath,True)
            print("dir "+ filepath + " removed")
   

def main(cameraIPList):
   
   #clean dir-----------------------------------
   prepareFilePath("./pic",True)
   prepareFilePath("./data",True)
   #-----------------------------------------------
   cameraList   = []
   for cameraIP in cameraIPList:
      cameraList.append(cCam.cCamera(cameraIP[0],cameraIP[1]))
   for camera in cameraList:
      camera.start()

   index = 0
   while True:
       ret = kbfunc()
       if ret == 27:#Esc :stop
           for camera in cameraList:
               camera.close()
           exit()
       elif ret == 116:#t   :take pic
           print("take Pic")
           for camera in cameraList:
              outDatapath = "./data/"+str(index)+"/"
              outPicpath = "./pic/"+str(index)+"/"
              prepareFilePath(outDatapath,False)
              prepareFilePath(outPicpath,False)

              camera.index = index
              camera.outDatapath = outDatapath
              camera.outPicpath = outPicpath
              
              camera.tp_flag = True
              #camera.takePic(index,outDatapath,outPicpath)
           index = index + 1
       time.sleep(0.1)

if __name__ == '__main__':
   cameraIPList = [('pic88','88'),('pic89','89'),('pic98','98'),('pic99','99')]
   main(cameraIPList)
    

