#!/usr/bin/python           
 
import socket               # Import socket module
s = socket.socket()         # Create a socket object
host = '192.168.0.141'	    # Get local machine name
port = 12347                # Reserve a port for your service.
 
import serial
import time
 
DEVICE = '/dev/ttyACM0' # the arduino serial interface (use dmesg when connecting)
BAUD = 9600
ser = serial.Serial(DEVICE, BAUD)

 
 
s.bind((host, port))        # Bind to the port
print 'Server started!'
while True:
    print 'Waiting for client'
    s.listen(5)                 # Now wait for client connection.
    c, addr = s.accept()     # Establish connection with client.
    print 'Got connection from', addr
    while True:
        msg = c.recv(1) # get a byte from the TCP connection
        if msg == "":
            print 'Connection closed'
            c.close() 
            break
        print 'Got "%s", writing to serial' % msg
	if '$' in msg:
		millis = int(round(time.time() * 1000))
		print 'Time: ' + str(millis)
        ser.write(msg)  # write the byte to the serial interface
