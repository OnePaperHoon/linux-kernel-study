import serial
import time

s = serial.Serial('/dev/serial0', 115200, timeout=3)

print("Waiting for data (reset STM32 now)...")
data = s.read(100)
print('Startup msg:', repr(data))

print("Sending Hello...")
s.write(b'Hello STM32!\r\n')
data = s.read(100)
print('Echo:', repr(data))

s.close()
