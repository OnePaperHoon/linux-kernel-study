#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// 패킷 프레임
#define PACKET_START	0x02
#define PACKET_END		0x03

// 명령어
#define CMD_LED_ON		0x01
#define CMD_LED_OFF		0x02
#define CMD_LED_TOGGLE	0x03
#define CMD_ACK			0xAA

// 패킷크기
#define MAX_DATA_LEN	8
#define PACKET_MIN_LEN	5	// START + CMD + LEN + CHECKSUM + END (NO DATA)

// 패킷 파싱 상태
typedef enum {
	STATE_WAIT_START,
	STATE_GET_CMD,
	STATE_GET_LEN,
	STATE_GET_DATA,
	STATE_GET_CHECKSUM,
	STATE_GET_END
} PacketState;

// 패킷 구조체
typedef struct {
	uint8_t cmd;
	uint8_t len;
	uint8_t data[MAX_DATA_LEN];
	uint8_t checksum;
} Packet;

// 체크섬 계산 : CMD ^ LEN ^ DATA[0] ^ ... ^ DATA[N-1]
static inline uint8_t calc_checksum(const Packet *pkt) {
	uint8_t cs = pkt->cmd ^ pkt->len;
	for (uint8_t i = 0; i < pkt->len; i++) {
		cs ^= pkt->data[i];
	}
	return cs;
}

#endif
