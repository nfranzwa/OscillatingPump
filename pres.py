from readard import readard
import serial
import serial.tools.list_ports
from flask import Flask
from datetime import datetime
import csv
arduino = serial.Serial(port='COM10',baudrate=9600)
app = Flask(__name__)
f = open('data.csv',"w+")
f.close()
def csvwriter(): 

	pressure_data = readard(arduino)
	if pressure_data:
		time = datetime.now().strftime('%H:%M:%S.%f')[:-5]
		pressure = float(pressure_data)	
		with open('data.csv','a') as fd:
			w = csv.writer(fd,lineterminator='\n')
			w.writerow([time,pressure])
			w.writerow
			fd.close()

if __name__ == "__main__": 
	while True:
		csvwriter()
