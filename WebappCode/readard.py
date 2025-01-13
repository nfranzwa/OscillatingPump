def readard(arduino):
    data = arduino.readline()
    data = data.decode('utf-8')
    if data[0] == 'Z' or data[0] == 'C':
         return []
    else:
        lis = data.split(',')
        if len(lis) > 2:
            cmH20 = lis[2]
            print(cmH20)
            return cmH20

