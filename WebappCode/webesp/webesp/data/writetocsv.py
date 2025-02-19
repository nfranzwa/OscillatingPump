import urllib.request
from datetime import datetime
import csv

firsthit = 0

while True:
    request_url1 = urllib.request.urlopen("http://192.168.1.85/record")
    request_url2 = urllib.request.urlopen("http://192.168.1.85/readings")
    val1 = datetime.now().strftime("%H:%M:%S.%f")[:-4]
    vallis = [val1]
    val2 = request_url2.read().decode("utf-8")
    vallis[1:] = val2.split(",")
    if request_url1.read().decode("utf-8") == "1":
        if firsthit == 0:
            with open("data.csv", "w", newline="") as csvfile:
                    fieldnames = ['Time', 'Pressure', 'Oscillation Frequency', 'Sustain Time']
                    writer1 = csv.DictWriter(csvfile, fieldnames=fieldnames)
                    writer1.writeheader()
                    writer = csv.writer(csvfile)
                    writer.writerows([vallis])
            firsthit = 1
        else:
            with open("data.csv", "a", newline="") as csvfile:
                    writer = csv.writer(csvfile)
                    writer.writerows([vallis])
            firsthit = 1
    else:
        firsthit = 0
