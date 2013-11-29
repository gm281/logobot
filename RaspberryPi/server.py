#!/usr/bin/python

import socket
import serial
import select
import time

s = socket.socket()         # Create a socket object
port = 12347                # Reserve a port for your service.

DEVICE = '/dev/ttyACM0' # the arduino serial interface (use dmesg when connecting)
BAUD = 9600
ser = serial.Serial(DEVICE, BAUD)
ser.nonblocking()

s.bind(('', port))        # Bind to the port
print 'Server started!'
while True:
    print 'Waiting for client'
    s.listen(5)                 # Now wait for client connection.
    c, addr = s.accept()     # Establish connection with client.
    print 'Got connection from', addr
    readins = [c, ser]
    serialOut = ""
    while True:
        try:
            inputready,outputready,exceptready = select.select(readins, [], [])
        except:
            break
        for fd in inputready:
            if fd == ser:
                serialOut += ser.read()
                if '\n' in serialOut:
                    print 'Serial stuff: ' + serialOut
                    serialOut = ""
            if fd == c:
                msg = c.recv(1024)
                if msg == "":
                    print 'Connection closed'
                    c.close()
                    break
                print 'Got "%s", writing to serial' % msg
                ser.write(msg)  # write the byte to the serial interface
                if '$' in msg:
                    millis = int(round(time.time() * 1000))
                    print 'Time: ' + str(millis)
