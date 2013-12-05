#!/usr/bin/env python

import socket
import curses
import signal
import sys
from datetime import datetime, timedelta

def signal_handler(signal, frame):
    curses.endwin()
    sys.exit(0)

def register_signal_handler():
    signal.signal(signal.SIGINT, signal_handler)

def connect():
    global socket
    socket = socket.socket()
    host = '192.168.0.141'
    port = 12347
    socket.connect((host, port))

def send_command(key, same_direction):
    global current_speed
    global stdscr
    global socket

    if (not same_direction):
        current_speed = 50
    else:
        current_speed = current_speed + 10
        current_speed = min(current_speed, 100)
    command = ''
    if key == curses.KEY_UP:
        stdscr.addstr(0,0,"UP")
        command = 'm' + str(current_speed) + ','  + str(current_speed) + ',400$'
    elif key == curses.KEY_DOWN:
        stdscr.addstr(0,0,"DOWN")
        command = 'm-' + str(current_speed) + ',-'  + str(current_speed) + ',400$'
    elif key == curses.KEY_LEFT:
        stdscr.addstr(0,0,"LEFT")
        command = 'm' + str(current_speed) + ',-'  + str(current_speed) + ',400$'
    elif key == curses.KEY_RIGHT:
        stdscr.addstr(0,0,"RIGH")
        command = 'm-' + str(current_speed) + ','  + str(current_speed) + ',400$'
    stdscr.addstr(1,0,command)
    socket.send(command)

def setup_console():
    global stdscr
    stdscr = curses.initscr()
    curses.cbreak()
    stdscr.keypad(1)
    stdscr.refresh()

def read_loop():
    global stdscr
    count = 0
    key = ''
    last_key = ''
    last_command_time = datetime.now()
    last_evaluation_time = datetime.now()
    while key != ord('q'):
        key = stdscr.getch()
        stdscr.clear()
        if key != curses.KEY_UP and key != curses.KEY_DOWN and key != curses.KEY_RIGHT and key != curses.KEY_LEFT:
            stdscr.refresh()
            continue
        curr_time = datetime.now()
        if key != last_key or (curr_time - last_command_time) > timedelta(milliseconds = 100):
            send_command(key, (key == last_key) and ((curr_time - last_evaluation_time) < timedelta(milliseconds = 100)))
            last_command_time = curr_time
        last_evaluation_time = curr_time
        last_key = key
        stdscr.refresh()
        count = count + 1
    curses.endwin()

register_signal_handler()
connect()
setup_console()
read_loop()
