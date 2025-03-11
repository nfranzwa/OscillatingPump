#!/usr/bin/env python3
import requests
from datetime import datetime
from datetime import date
import csv
import os
url_record = "http://192.168.1.85/record"
url_read = "http://192.168.1.85/readings"
firsthit = 0
headers = requests.utils.default_headers()
headers.update(
     {
          'User-Agent':'Me',
     }
)
print("recording")
print("Press Control + C or exit Terminal to stop recording")
print("The files will be placed in the folder titled data")

counter = 0

while True:

    response_record = requests.get(url_record,headers=headers)
    recording = response_record.text
    val1 = datetime.now().strftime("%H:%M:%S.%f")[:-4]
    vallis = [val1]

    if recording == "1":
    
        response_read = requests.get(url_read,headers=headers)
        val2 = response_read.text
        vallis[1:] = val2.split(",")
        if firsthit == 0:
            curdate = date.today().strftime("%Y_%m_%d")
            curhrmin = datetime.now().strftime("%H_%M")
            if not os.path.exists("data"):
                os.makedirs("data")
            filename = "data/data"+curdate+"_"+curhrmin+".csv"
            with open(filename, "w", newline="") as csvfile:
                    fieldnames = ['Time', 'Pressure', 'Oscillation Frequency', 'Sustain Time']
                    writer1 = csv.DictWriter(csvfile, fieldnames=fieldnames)
                    writer1.writeheader()
                    writer = csv.writer(csvfile)
                    writer.writerows([vallis])
            firsthit = 1
        else:
            with open(filename, "a", newline="") as csvfile:
                    writer = csv.writer(csvfile)
                    writer.writerows([vallis])
            firsthit = 1
    else:
        firsthit = 0

