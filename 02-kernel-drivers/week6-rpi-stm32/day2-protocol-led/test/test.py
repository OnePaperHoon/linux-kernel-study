#!/usr/bin/env python3
"""
Day2 - UART Protocol LED Control Test
STM32 NUCLEO-F446RE에 프로토콜 패킷을 전송하여 LED 제어
"""

import serial
import time
import sys

PORT = '/dev/cu.usbmodem1403'
BAUD = 115200

# 명령어 정의
CMD_LED_ON     = 0x01
CMD_LED_OFF    = 0x02
CMD_LED_TOGGLE = 0x03
CMD_ACK        = 0xAA

PACKET_START   = 0x02
PACKET_END     = 0x03

def send_cmd(ser, cmd):
    """프로토콜 패킷 전송 (DATA 없는 경우)"""
    checksum = cmd ^ 0x00  # CMD ^ LEN(0)
    packet = bytes([PACKET_START, cmd, 0x00, checksum, PACKET_END])
    ser.write(packet)
    print(f"  TX: {packet.hex(' ')}")
    time.sleep(0.1)
    if ser.in_waiting:
        ack = ser.read(ser.in_waiting)
        print(f"  RX: {ack.hex(' ')}")

def main():
    print(f"[*] Connecting to {PORT} @ {BAUD}...")
    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
    except Exception as e:
        print(f"[!] 연결 실패: {e}")
        print("    - screen/minicom 세션 종료했는지 확인 (killall screen)")
        print("    - USB 케이블 뽑았다 다시 꽂기")
        sys.exit(1)

    time.sleep(2)  # STM32 리셋 대기

    # 시작 메시지 읽기
    if ser.in_waiting:
        startup = ser.read(ser.in_waiting)
        print(f"[*] STM32: {startup.decode(errors='replace')}")

    print("\n[1] LED ON")
    send_cmd(ser, CMD_LED_ON)
    time.sleep(0.1)

    print("[2] LED OFF")
    send_cmd(ser, CMD_LED_OFF)
    time.sleep(0.1)

    print("[3] LED TOGGLE")
    send_cmd(ser, CMD_LED_TOGGLE)
    time.sleep(0.1)

    print("[4] LED TOGGLE")
    send_cmd(ser, CMD_LED_TOGGLE)
    time.sleep(0.1)

    print("\n[*] 테스트 완료!")
    ser.close()

if __name__ == '__main__':
    main()
