#include <stdint.h>

typedef struct packet_struct{
  uint16_t number;
  char data[2];
  uint8_t crc[2];
  char end;
} packet_t;

#define PACKET_SIZE sizeof(packet_t)

// Prints packet information to the console
void printPacket(packet_t packet);

// Generates a packet with the SN provided and 2 bytes of 
// data starting at dataIndex with CRC
packet_t createPacket(uint16_t SN, char* data, int dataIndex);
