#include <stdio.h>
#include <string.h>
#include "packet.h"
#include "ccitt16.h"

void printPacket(packet_t packet){
  printf("SN\tData\tCRC\n");
  printf("%d\t", packet.number);
  printf("%c%c\t", packet.data[0], packet.data[1]);
  printf("%02X%02X\n\n", packet.crc[0], packet.crc[1]);
}

packet_t createPacket(uint16_t SN, char* data, int dataIndex){
  packet_t pkt;

  // Set packet Sequence Number
  pkt.number = SN;

  // Set packet Data
  pkt.data[0] = data[dataIndex];
  pkt.data[1] = data[dataIndex + 1];

  // Initialize packet CRC
  pkt.crc[0] = 0;
  pkt.crc[1] = 0;

  // Generate CRC for the packet
  uint16_t crc = calculate_CCITT16((char*) &pkt, PACKET_SIZE-4, GENERATE_CRC);
  pkt.crc[0] = (uint8_t)(crc >> 8);
  pkt.crc[1] = (uint8_t)(crc & 0xFF);
  
  // Post-CRC generated packet
  // printPacket(pkt);

  // Set null terminator
  pkt.end = '\0';

  return pkt;
}
