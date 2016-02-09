// crc_demo.c
// Compile: gcc -o crc_demo crc_demo.c ccitt16.o 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ccitt16.h"
#include "packet.h"
#include "AddCongestion.h"

/*
typedef struct {
    uint16_t number;
    uint8_t data[2];
    uint8_t crc[2];
    char end;
} packet_t;


long M = 2147483647;
 
void AddCongestion(char *data, double p)
{
    char c, *pointer = data;
    int i;
    while (*pointer != '\0') {
   	 c = 0x01;
   	 for ( i = 0; i < 8; i++) {
   		 if ((double)random()/M <= p)
   			 *pointer ^= c;
   		 c <<= 1;
   	 }
   	 pointer++;
    }
}
*/
int main(int argc, char *argv[]) {
    
    packet_t* packet;

    packet->number = 0;
    packet->data[0] = 'a';
    packet->data[1] = 'b';
    
    uint16_t crc = calculate_CCITT16((uint8_t *) packet, 4, GENERATE_CRC);
    packet->crc[0] = (uint8_t)(crc >> 8);
    packet->crc[1] = (uint8_t)(crc & 0xFF);

    packet->end = '\0';

    printf("Packet before introducing error: %d %c%c %02x%02x\n", packet->number, packet->data[0], packet->data[1], packet->crc[0], packet->crc[1]);

    double ber = 1.0;
    AddCongestion((char *) packet, ber);    
    packet->data[0] = 'x';
    printf("Packet after  introducing error: %d %c%c %02x%02x\n", packet->number, packet->data[0], packet->data[1], packet->crc[0], packet->crc[1]);

    
    if (!calculate_CCITT16((char *) packet, 6, CHECK_CRC))
   	 printf("No error!\n");
    else
   	 printf("Error!\n");    

}
