import subprocess

def main():
   success = 0
   fail = False
   with open("log.txt","w") as f:
      while (not fail):
         ret = subprocess.call("Release/lame_pthread.exe C:\\Temp\\testwav -n64", stdout=f, stderr=f)
         if ret!=0:
            fail = True
            print "Error in run %d!" % (success+1)
         else:
            success += 1
            print "%d runs successful." % success

if __name__=='__main__':
   main()