import numpy as np
import matplotlib.pyplot as plt
def process(sample,fname):
  val=[]
  vd=[]
  fout=open(fname,'r')
  in_data=fout.read()

  str=in_data.split('\n')
  for s in str:
      if(s):
	  d=s.split()
	  t=d[2].split(']');
	  mtime=int(float(t[0])*1000);
	  if(s.find(sample)!=-1):
	      val.append(mtime)
	      
  i=1
  for vt in val[1:]:
      vd.append(val[i]-val[i-1])
      i=i+1
  print vd      
  print fname
  print 'average=',np.mean(vd,axis=0),' dev=',np.std(vd, axis=0)

  plt.plot(vd)

  plt.show()
