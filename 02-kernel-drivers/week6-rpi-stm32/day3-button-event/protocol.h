/*
 * protocol.h - Communication protocol
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <linux/types.h>

// Protocol Makers
#define PROTO_START 0x02
#define PROTO_END 0x03

/* Cmmands */
#define CMD_LED_ON 0x01
#define CMD_LED_OFF 0x02
#define CMD_LED_TOGGLE 0x03
#define CMD_LED_STATUS 0x04

/* NEW: Button Event */
#define CMD_BTN_PRESSED 0x10
#define CMD_BTN_RELEASED 0x11
#define CMD_BTN_CLICK 0x12 // Short press
#define CMD_BTN_HOLD 0x13  // Long Press

#define CMD_ACK 0xAA
#define CMD_NACK 0xBB

/* Packet Structure */
struct uart_packet {
  u8 start;
  u8 cmd;
  u8 len;
  u8 data[16];
  u8 checksum;
  u8 end;
} __attribute__((packed));

/* Button Event Data */
struct button_event {
  u32 timestamp;
  u8 state;
} __attribute__((packed));

/*Calculate checksum */
static inline u8 calc_checksum(struct uart_packet *pkt) {
  u8 sum = 0;
  int i;

  sum += pkt->cmd;
  sum += pkt->len;

  for (int i = 0; i < pkt->len; i++) {
    sum += pkt->data[i];
  }

  return sum & 0xFF;
}
#endif
