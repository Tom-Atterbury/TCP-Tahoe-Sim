#include "packet.h"
#include "ack.h"
#include "ccitt16.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
 
#define SN_OFFSET 1000

struct sockaddr_in receiverAddr, senderAddr;

// Default receiver information
char* LOCAL_HOST = "127.0.0.1";
int PORT = 8005;
 
void main(int argc, char** argv) {
  int receiverSocketDescriptor, senderSocketDescriptor;
  int len = sizeof(senderAddr);
  int* received = malloc(1024 * sizeof(int));
  char* readBuffer = malloc(sizeof(packet_t));
  packet_t packet;
 
  // Create a socket
  if( (receiverSocketDescriptor = socket(PF_INET, SOCK_STREAM, 0)) < 0){
    perror("socket() error");
    exit(1);
  }
  
  // Set IP Address and port of socket
  receiverAddr.sin_family = PF_INET;
  receiverAddr.sin_port = htons(PORT);
  receiverAddr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
 
  // Bind receiver address to socket
  if( bind(receiverSocketDescriptor, (struct sockaddr*) &receiverAddr, sizeof(receiverAddr)) < 0) {
    perror("bind() error");
    exit(1);
  }

  // Listen for incomming connections on receiver socket, with 10 maximum connections
  listen(receiverSocketDescriptor, 10);
  printf("Receiver ready. Waiting for Senders.\n");

    // Accept a sender connection
    senderSocketDescriptor = accept(receiverSocketDescriptor, (struct sockaddr*) &senderAddr, &len);
    if(senderSocketDescriptor < 0) {
      perror("accept() error");
      exit(1);
    }
  printf("Sender connected, reading data...\n");
  int packetCount = 0;
  while(1){
    // Receive packet
    read(senderSocketDescriptor, &packet, PACKET_SIZE);
    //printPacket(packet);

    // Check packet CRC
    if (!calculate_CCITT16((char *) &packet, PACKET_SIZE-2, CHECK_CRC)) {
      // CRC is good
      // "Buffer" packet
      received[packet.number - SN_OFFSET] = 1;
      received[packet.number - SN_OFFSET + 1] = 1;

      int SN = 0;
      // SN = next largest missing packet number
      while(received[SN]) SN++;

      // Delay ~1 sec
      sleep(0.25);

      // Send ACK(SN)
      ack_t ack;
      ack.number = SN + SN_OFFSET;
      printACK(ack);
      write(senderSocketDescriptor, (char *) &ack, ACK_SIZE);

      packetCount++;
      /*
      if(packet.number >= 2022){
        // Close the sender socket
        close(senderSocketDescriptor);      
        break;
      }
      */
    } else {
      // Do nothing.
   	  printf("SN %d: Error!\n", packet.number);
    }
  }

  // Close the receiver socket
  close (receiverSocketDescriptor);
}
