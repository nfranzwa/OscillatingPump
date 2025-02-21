import serial
from datetime import datetime
import csv
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

filename = "csvdata3.csv"
datalis = []
timelis = []

with open(filename, "w", newline="") as csvfile:
    fieldnames = ['Time', 'Pressure']
    writer1 = csv.DictWriter(csvfile, fieldnames=fieldnames)
    writer1.writeheader()

def readserial(com, baud):
    ser = serial.Serial(com, baud, timeout=0.1)
    
    fig, ax = plt.subplots(figsize=(8, 6))
    line, = ax.plot([], [], marker='o', linestyle='-')
    def update(frame):
        data = ser.readline().decode().strip()
        if data:
            if data[0].isdigit() == True: 
                val1 = datetime.now().strftime("%H:%M:%S.%f")[:-4]
                timelis.append(datetime.strptime(val1, "%H:%M:%S.%f"))  # Convert to datetime object
                datalis.append(float(data))


                if len(datalis) > 100:
                    del datalis[0]
                    del timelis[0]
                if timelis and datalis:  # Ensure both lists are populated before plotting
                        ax.plot(timelis, datalis, marker='o', linestyle='-')
                ax.clear()
                ax.plot(timelis, datalis, marker='o', linestyle='-')
                ax.set_xlabel("Time")
                ax.set_ylabel("Pressure")
                ax.set_title("Plot of Last 20 Values")
                ax.grid()
                fig.autofmt_xdate()
                vallis = [val1]
                val2 = data
                vallis[1:] = val2.split(",")
                with open(filename, "a", newline="") as csvfile:
                    writer = csv.writer(csvfile)
                    writer.writerows([vallis])
                
    ani = FuncAnimation(fig, update)
    plt.show()

if __name__ == "__main__":
    readserial('COM12', 115200)
