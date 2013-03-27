"""    internet_arduino_interface.py
 Purpose: Mid term demo
 Author: Tiago Pimentel
        t.pimentelms@gmail.com
         Joao Felipe
        joaofelipenp@gmail.com
 CSE 467 -- Embedded Computing Systems
 WUSTL, Spring 2013
 Date: Mar., 27, 2013

 Invariants:
    ROUGH_TIME > 10
    REALLY_ROUGH_TIME > 1
    LIGHT_POWER_TIME > 1
    START_TIME >= 2
 Description:
    Arduino's interface with internet will be replaced with an ethernet shield in next version of the project
    This provides the interface with internet

"""

import serial
import glob
import time
import sched
import httplib
import threading
import urllib, urllib2

ROUGH_TIME = 10
REALLY_ROUGH_TIME = 1
LIGHT_POWER_TIME = 1
START_TIME = 5

arduino_usb = glob.glob('/dev/ttyUSB*')[0]
arduino = serial.Serial(arduino_usb, 9600)

block_light_switch = False

# serial_write
# writes data to arduino serial port
# receives string
def serial_write(send):
    print send
    arduino.write(send.encode('ascii'))


# get_time
# get current timestamp
# returns string
def get_time():
    return str(int(time.time()))


# switch_lights
# sends post request to turn the lights on or off
# receives 0 or 1
def switch_lights(on_off):
    block_light_switch = True
    params = urllib.urlencode({
        'light_power[on]': str(on_off),
        'light_power[time]': str(int(time.time()*1000)),
    })
    url = "http://sleepingbeauty.herokuapp.com/light_power"
    req = urllib2.Request(url, params)
    try:
        response = urllib2.urlopen(req)
    except urllib2.URLError, e:
        pass
    block_light_switch = False


# rough_movement_thread
# checks if a rough movement occurred and sends to arduino
def rough_movement_thread():
    conn = httplib.HTTPConnection("sleepingbeauty.herokuapp.com")
    conn.request("GET", "/rough_movements/last_time.txt")
    r1 = conn.getresponse()
    if r1.status == 200:
        data1 = r1.read()
        serial_write("Data1"+str(data1)+"a")
    threading.Timer(ROUGH_TIME, rough_movement_thread).start()


# really_rough_movement_thread
# checks if a really rough movement occurred and sends to arduino
def really_rough_movement_thread():
    conn = httplib.HTTPConnection("sleepingbeauty.herokuapp.com")
    conn.request("GET", "/rough_movements/last_time.txt?really_rough=1")
    r1 = conn.getresponse()
    if r1.status == 200:
        data1 = r1.read()
        serial_write("Data3"+str(data1)+"a")
    threading.Timer(REALLY_ROUGH_TIME, really_rough_movement_thread).start()


# light_switch_thread
# checks if the light button was pressed and sends to arduino
def light_switch_thread():
    if block_light_switch:
        threading.Timer(LIGHT_POWER_TIME, light_switch_thread).start()
        return
    conn = httplib.HTTPConnection("sleepingbeauty.herokuapp.com")
    conn.request("GET", "/light_power/last.txt")
    r1 = conn.getresponse()
    if r1.status == 200:
        data1 = r1.read()
        serial_write("Data2"+str(data1))
    threading.Timer(LIGHT_POWER_TIME, light_switch_thread).start()


# arduino_listener
# Listen to arduino commands and realize a post request if the arduino says to turn the lights on of off
def arduino_listener():
    while True:
        #import ipdb; ipdb.set_trace()
        line = arduino.readline()
        print line

        if 'DATA 0' in line:
            switch_lights(0)
        elif 'DATA 1' in line:
            switch_lights(1)


if __name__ == '__main__':
    time.sleep(START_TIME)
    serial_write("Time"+get_time()+"a")
    rough_movement_thread()
    really_rough_movement_thread()
    light_switch_thread()
    arduino_listener()

