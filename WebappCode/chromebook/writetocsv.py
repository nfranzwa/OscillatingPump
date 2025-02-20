import requests
from datetime import datetime
import csv
import os
from win32com.client import Dispatch
url_record = "http://192.168.1.85/record"
url_read = "http://192.168.1.85/readings"
firsthit = 0
headers = requests.utils.default_headers()
headers.update(
     {
          'User-Agent':'Me',
     }
)
print("here")
counter = 0
def create_shortcut(folder_path,shortcut_name):
    if not os.path.exists(folder_path):
        print("error")
        return
    desktop = os.path.join(os.environ["USERPROFILE"],"Desktop")
    shortcut_path = os.path.join(desktop, f"{shortcut_name}.lnk")

    # Create the shortcut
    shell = Dispatch('WScript.Shell')
    shortcut = shell.CreateShortcut(shortcut_path)
    shortcut.TargetPath = folder_path  # Set target to the folder
    shortcut.WorkingDirectory = folder_path  # Set working directory
    shortcut.IconLocation = r"C:\Windows\System32\shell32.dll,3"  # Optional: Folder icon
    shortcut.Save()

    print(f"Shortcut successfully created: {shortcut_path}")

folder_to_shortcut = os.path.join(os.environ["USERPROFILE"],"Documents","156testingcsv\dist\writetocsv\data")

while True:

    response_record = requests.get(url_record,headers=headers)
    recording = response_record.text
    val1 = datetime.now().strftime("%H:%M:%S.%f")[:-4]
    vallis = [val1]
    val1 = datetime.now().strftime("%H:%M:%S.%f")[:-4]
    vallis = [val1]
    if recording == "1":
        response_read = requests.get(url_read,headers=headers)
        val2 = response_read.text
        vallis[1:] = val2.split(",")
        if firsthit == 0:
            counter +=1
            counterstr = str(counter)
            if not os.path.exists("data"):
                os.makedirs("data")
                create_shortcut(folder_to_shortcut,"Data")
            filename = "data/data"+counterstr+".csv"
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

