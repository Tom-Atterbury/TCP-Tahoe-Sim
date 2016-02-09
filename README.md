# TCP Tahoe Sim

To compile Sender:
  $gcc -o Sender Sender.c ccitt16.o AddCongestion.o packet.o ack.o -lm
  
To compile Receiver:
  $gcc -o Receiver Receiver.c ccitt16.o packet.o ack.o
  
To Run:
  1) Start receiver using the following command
    $./Receiver
  2) Start the sender with a BER of 0.0001 use the following command
    $./Sender 0.0001
  3) Wait for program to stop
  4) RTT and congestion window data are saved in the file "RTT data.csv"
  
If no BER is specified, Sender runs with a default BER = 0