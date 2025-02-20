import serial
from datetime import datetime
import csv
filename = "csvdata2.csv"

with open(filename, "w", newline="") as csvfile:
    fieldnames = ['Time', 'Pressure']
    writer1 = csv.DictWriter(csvfile, fieldnames=fieldnames)
    writer1.writeheader()
def readserial(com,baud):
    ser = serial.Serial(com,baud,timeout=0.1)
    while True:
        data=ser.readline().decode().strip()
        if data:
            val1 = datetime.now().strftime("%H:%M:%S.%f")[:-4]
            vallis = [val1]
            val2 = data
            vallis[1:] = val2.split(",")
            with open(filename, "a", newline="") as csvfile:
                    writer = csv.writer(csvfile)
                    writer.writerows([vallis])


if __name__ == "__main__":
     readserial('COM12',115200)