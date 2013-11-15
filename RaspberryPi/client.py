#!/usr/bin/env python

import time
import socket               # Import socket module
import sys
import os, fcntl, tty, termios

def getch():
    fd = sys.stdin.fileno()
    oldterm = termios.tcgetattr(fd)
    newattr = termios.tcgetattr(fd)
    newattr[3] = newattr[3] & ~termios.ICANON & ~termios.ECHO
    try:
        termios.tcsetattr(fd, termios.TCSANOW, newattr)
        oldflags = fcntl.fcntl(fd, fcntl.F_GETFL)
        return sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, oldterm)

s = socket.socket()         # Create a socket object
host = '192.168.0.141' #socket.gethostname() # Get local machine name
port = 12349                # Reserve a port for your service.

print 'Connecting to ', host, port
s.connect((host, port))

print 'Waiting for input'
while True:
    byte = getch()
    print 'Got input "%c" sending off' % byte
    s.send(byte)
