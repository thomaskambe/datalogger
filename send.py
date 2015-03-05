#!/usr/bin/env python

from socket import *
import sys
import time
import fileinput

s = socket(AF_INET,SOCK_DGRAM)
host =sys.argv[1]
port = 5810
buf =1024
addr = (host,port)

file_name=sys.argv[2]

#s.sendto(file_name,addr)

f=open(file_name,"rb")
for line in fileinput.input(file_name):
    s.sendto(line, addr)
    time.sleep(1)

#data = f.read(buf)
#while (data):
#    if(s.sendto(data,addr)):
#        print "sending ..."
#        data = f.read(buf)
s.close()
f.close()
