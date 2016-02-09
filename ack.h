#include <stdint.h>

typedef struct ack_struct{
  uint16_t number;
} ack_t;

#define ACK_SIZE sizeof(ack_t)

void printACK(ack_t ack);
