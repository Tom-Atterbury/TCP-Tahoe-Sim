#include "packet.h"
#include "ack.h"
#include "ccitt16.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>

char* LOCAL_HOST = "127.0.0.1";
char* RTT_OUTPUT_FILE = "RTT data.csv";
int PORT = 8005;
int MAX_DATA_BYTES = 2048;
uint16_t SN_OFFSET = 1000;
float BER = 0.0;

uint16_t fastReTransmitt(int SD, uint16_t SN, char* data, int** dataState){
  int success = 0;
  packet_t packet;
  ack_t ack;
  while(!success){
    // Create packet to send
    packet = createPacket(SN, data, SN - SN_OFFSET);

    // Congest packet
    AddCongestion((char *) &packet, BER);

    // Send packet
    printf("Sending packet: %d\n", SN);
    write(SD, (char *) &packet, PACKET_SIZE);

    // Set data state of data sent
    dataState[0][SN] = 1;


    while(1){
      // Wait for ACK or RTO
      int bytesRead = 0;
      while (bytesRead == 0) bytesRead = read(SD, &ack, ACK_SIZE);
      if( bytesRead < 0) {
        int errsv = errno;

        // Timeout detection
        if(errsv = EWOULDBLOCK) {
          // Timeout: Retransmit SN
          printf("Timeout\n");
          break;
        }
      } else if (bytesRead > 0) {
        printACK(ack);

        // Succesful retransmit
        if(SN < ack.number) {
          success = 1;
          break;
        }
      }
    }
  }

  return ack.number;
}


int main(int argc, char* argv[]){
  
  // Get BER from command line, if provided
  if(argc > 1){
    sscanf(argv[1], "%f", &BER);
  }

  // Socket
  int socketDescriptor;
  struct sockaddr_in receiverAddress;
  char* receiverIPAddress = LOCAL_HOST; // Use localhost

  // Data
  char* data = malloc(MAX_DATA_BYTES * sizeof(char));
  int** dataState = malloc(2 * sizeof(int *));

  // Packet
  packet_t packet;
  ack_t ack;

  // Congestion Control
  uint16_t SN = SN_OFFSET;
  uint16_t SN_next = SN;
  int ssthresh = 16;
  float cwnd = 1;
  int swnd = 0;
  int wndIndex = 0;
  int RTT = 0;
  int done = 0;

  // Timeout
  int RTO = 0;
  struct timeval tv;
  tv.tv_sec = 3;
  tv.tv_usec = 0;
  
  // Read in "input.txt" data to array
  FILE *fp = fopen("input.txt" , "r");       
  int numDataBytes = strlen(fgets(data, MAX_DATA_BYTES, fp));
  fclose(fp);

  // Open file to write RTT data to
  fp = fopen(RTT_OUTPUT_FILE, "w");
  fprintf(fp, "RTT, CWD\n");

  // Allocate space for state of data bytes
  dataState[0] = malloc(numDataBytes * sizeof(int)); // byte Sent? (y/n)
  dataState[1] = malloc(numDataBytes * sizeof(int)); // byte ACK count

  // Initialize space
  for(wndIndex = 0; wndIndex < numDataBytes; wndIndex++){
    dataState[0][wndIndex] = 0;
    dataState[1][wndIndex] = 0;
  }

  // Create a socket
  if ( (socketDescriptor = socket(PF_INET, SOCK_STREAM, 0)) < 0 ) {
    perror("socket() error");
    exit(-1);
  }

  // Setup socket timeout options
  if(setsockopt(socketDescriptor, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(struct timeval)) < 0) {
    perror("setsockopt() error");
    exit(-1);
  }
  
  // Set the IP address and port of the socket
  receiverAddress.sin_family = PF_INET;
  receiverAddress.sin_port = htons(PORT);
  receiverAddress.sin_addr.s_addr = inet_addr(receiverIPAddress);

  // Connect to the receiver
  printf("Connecting to receiver...");
  if ( connect(socketDescriptor, (struct sockaddr*) &receiverAddress, sizeof(receiverAddress)) < 0 ) {
    perror("connect() error");
    exit(1);
  }
  printf("...Done\n");
  
  //printf("Sending data...\n");
  while(!done) { // All data has not been sent
    
    // Send cwnd burst, or all remaining packets
    SN = SN_next;
    swnd = (int) cwnd < SN - SN_OFFSET + numDataBytes ? (int) cwnd : SN - SN_OFFSET + numDataBytes;

    RTO = 0;
    printf("RTT: %d\tSN: %d\tCwnd: %f\tSwnd: %d\tssthresh: %d\n",RTT,SN,cwnd,swnd,ssthresh);

    // Print RTT and cwnd to file
    fprintf(fp, "%d, %f\n", RTT, cwnd);

    // Send swnd packets of data
    for(wndIndex = 0; wndIndex < swnd; wndIndex++) {
      // Create a packet then send if it has not been sent
      int startByte = SN - SN_OFFSET + (wndIndex * 2);

      if(!dataState[0][startByte]){
        // Create packet to send
        packet = createPacket(startByte + SN_OFFSET, data, startByte);

        // Congest packet
        AddCongestion((char *) &packet, BER);

        // Send packet
        //printPacket(packet);
        printf("Sending packet: %d\n", startByte + SN_OFFSET);
        write(socketDescriptor, (char *) &packet, PACKET_SIZE);

        // Set data state of data sent
        dataState[0][startByte] = 1;
      }
    }
    
    int loop = 1;
    for(wndIndex = 0; wndIndex < swnd; wndIndex++){
      if(!loop) break;

      // Wait for ACK or RTO
      int bytesRead = 0;
      while (bytesRead == 0) bytesRead = read(socketDescriptor, &ack, ACK_SIZE);
      if( bytesRead < 0) {
        int errsv = errno;

        // Timeout detection
        if(errsv = EWOULDBLOCK) {
          // Timeout: Retransmit SN, update ssthresh and cwnd
          // Fast Retransmit
          printf("Timeout\n");
          SN_next = fastReTransmitt(socketDescriptor, SN_next, data, dataState);

          // Reset data state of data sent
          dataState[0][SN_next - SN_OFFSET] = 0;
          dataState[1][SN_next - SN_OFFSET] = 0;

          // Update ssthresh and cwnd
          ssthresh = floorf(cwnd) / 2;
          cwnd = 1;

          // Exit to next transmission cycle
          loop = 0;
          break;
        }
      } else if (bytesRead > 0) {
        printACK(ack);

        // Set SN_next for next transmission cycle
        SN_next = ack.number;
        
        // Stop if we've received an ACK for all packets
        if(SN_next >= numDataBytes + SN_OFFSET - 1){
          done = 1;
          break;
        }

        // Count the ACK
        dataState[1][SN_next - SN_OFFSET]++;

        // Update cwnd and ssthresh at each ACK
        if(dataState[1][SN_next - SN_OFFSET] > 3) {
          // 4 Dup ACKs: Retransmit ACK(SN), update ssthresh and cwnd
          // Fast Retransmit
          printf("3 Dup ACKs\n");
          SN_next = fastReTransmitt(socketDescriptor, SN_next, data, dataState);

          // Reset data state of data sent
          dataState[0][SN_next - SN_OFFSET] = 0;
          dataState[1][SN_next - SN_OFFSET] = 0;

          // Update ssthresh and cwnd
          ssthresh = floorf(cwnd) / 2;
          cwnd = 1;

          // Exit to next Transmission cycle
          loop = 0;
          break;
        } else if(dataState[1][SN_next - SN_OFFSET] == 1) {
          // Non-dup ACK
          // Update cwnd according to ssthresh
          if(cwnd < ssthresh) {
              // Slow start
              cwnd += 1;
          } else {
              // Congestion Avoidance
              cwnd += 1 / floorf(cwnd);
          }
        }
      }
    }
    

    // Start next round
    RTT++;
  }

  // Close the socket
  close(socketDescriptor);

  // Close the output file
  fclose(fp);

  // Free up allocated space
  free(dataState);
  free(data);
  return 0;
}
