import serial
import time

s = serial.Serial('/dev/serial0', 115200, timeout=5)

print("Listening... reset STM32 now!")
data = s.read(100)
print('Received:', repr(data))

s.close()
