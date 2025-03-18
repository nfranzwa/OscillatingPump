UCSD MAE 156 Capstone Project

The provided code was developed using Arduino IDE and the Platform.io extension on VS Code.
While it is possible to flash/deploy our software to an ESP32 through other means, we heavily recommend the two for ease of installing dependencies and following subsequent instructions.

# Setup
In order to flash the GPIO ESP or Web ESP, first make a local copy of the most recent version of this repository by clicking `<Code>` and either:

 - Copying and running the command in terminal (requires github cli installation or git)
 - Opening through Github Desktop (requires installing the Github Desktop app)
 - Downloading and extracting the code from a .zip file

The next steps assume you have a local copy of the provided code.

# Installing Libraries

## GPIO ESP

Click the Platform IO icon on the left edge of VS Code, then navigate to `Quick Access > PIO Home > Projects & Configuration`.

![image](https://github.com/user-attachments/assets/866cb615-2042-4cda-ac27-e3dfc7ca1644)

Select `Add Existing`.

![image](https://github.com/user-attachments/assets/4e44e633-8489-4eae-9c97-9edeb3221ab2)

Navigate to the directory `drive/path/to/repository/OscillatingPump/ESP32_CTRL` and select `Open "ESP32_CTRL`.

![image](https://github.com/user-attachments/assets/e20c40d3-a5b7-4fa2-be15-65cb3920270f)

Shortly, Platform IO should be setting up the project according to the `platform.ini` file in the selected folder, most importantly installing libraries that are used in the project.

## Web ESP

Open the Arduino sketch named `webesp.ino` located in `/WebappCode/webesp/webesp/`.

Navigate to the library manager.

![image](https://github.com/user-attachments/assets/e63eb2b8-ef66-48fd-8fc9-b952e9c8ebfb)

Search for and install the following libraries:
 - Arduino_JSON by Arduino
 - Async TCP by ESP32Async
 - ESP Async WebServer by ESP32ASync
 - EspSoftwareSerial by Dirk Kaar, Peter Lerup

In the top menu, Navigate to `Sketch > Include Library`, and include the following:
 - WiFi
 - LittleFS

![image](https://github.com/user-attachments/assets/48c6c2f4-89c3-4810-a910-0f3b61e1e3e2)

The next step is to install ESP32 LittleFS uploader to the Arduino IDE.

Go to [[https://github.com/earlephilhower/arduino-littlefs-upload/releases]] and download the latest version of `arduino-littlefs-upload-x.x.x.vsix`.

Windows: 
1. In File Explorer, navigate to `C:\Users\<username>\.arduinoIDE\`
2. Create a folder called `plugins` if it doesn't already exist.
3. Move the downloaded `.vsix` file into this folder.

Mac OS: 
1. In Finder, navigate to `~/.arduinoIDE`
2. Create a new folder called `plugins`
3. Move the downloaded `.vsix` file into this folder.

Restart the Arduino IDE, and make sure the plug-in was successfully installed by opening the command palette with `Ctrl/Cmd + Shift + P` and ensuring `Upload LittleFS to Pico/ESP8266/ESP32` is an available command.

# Uploading/Deploying code to the ESP32's

With the environments and libraries set up, one can compile and deploy the code through the following procedures.

## GPIO ESP

On the bottom bar, ensure that the Platform IO project is ESP32_CTRL and you are deploying to the right COM port (leave as `Auto` unless multiple ESP's are connected to same computer).

![image](https://github.com/user-attachments/assets/f1948b3a-c561-4254-bbf7-d5509642d9c6)

The code will be compiled and deployed to the ESP32 when pressing the `→` button.
 - one can also compile the code without deploying by pressing the `✓` button.

A new terminal should open within VS Code, displaying compile messages. When the terminal says `Connecting...`
Press and hold the `Boot` button on the ESP until the terminal shows that code is being written to the ESP32.

## Web ESP
First, Go to `Sketch > Show Sketch Folder` and ensure that the `data/` folder from our repository is there. This contains files that are to be uploaded to the ESP32 filesystem.

Open the command palette and run `Upload LittleFS to Pico/ESP8266/ESP32`.

After getting a `Completed upload` message, compile and deploy the sketch to the ESP32 by pressing the `→` button.
 - one can also compile the code without deploying by pressing the `✓` button.

When the terminal says `Connecting...`
Press and hold the `Boot` button on the ESP until the terminal shows that code is being written to the ESP32.

# Troubleshooting code
If the ESP's are consistently not behaving as expected, we provide debugging statements through Serial Monitors, which can be accessed through the Command Palette of both VS Code and Arduino IDE.

Issues to check may include whether an ESP is:
 - constantly restarting (stuck in a boot loop)
 - unable to communicate through serial with the other ESP
 - some particular features are not behaving as expected
 - error codes being thrown by an ESP

The GPIO ESP has an ongoing task that reads the Serial port between the computer and ESP, providing the user with the following inputs and features:

| User Input      | Feature                                                            |
| --------------- | ------------------------------------------------------------------ |
| `debug`         | toggles debugging statements for Serial communication between ESPs |
| `cal debug`     | toggles debugging statements for during calibration                |
| `wave debug`    | toggles debugging statements for wave generation                   |
| `show pm`       | prints out the current pressure map vector                         |
| `m:#.## M:#.##` | manually overwrite the target min and max pressure                 |
| `c:#`           | manually overwrite the calibration state                           |
