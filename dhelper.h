#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>

#define STDIN 0
#define MAXSEQNUMBER 100
#define MAXPARTNERS 100
#include "linkedlist.h"

int checkUserInput(int argc, char *argv[]);
int createSocket();
int findPartnerIndex(int portNumber);
int getMyLocation(FILE *configFile, int currentPort);
void handleACK(int sd, int portNumber, int myLocation);
void *handleFileOpening(char *fileName);
void handleLatestMessage(int sd, char *buffer, int portNumber, int myLocation, int rows, int columns);
void handleMoveMessage(int newLocation);
void initPartners(FILE *configFile, int currentPort);
void parseLine(char *line);
void reSend(int sd, int portNumber);
int receiveLine(int sd, int flags);
char *replace_str(char *str, char *orig, char *rep);
void sendACK(int sd, int portNumber, int myLocation);
int sendLine(int sd, char *line, struct sockaddr_in *server_address);
void sendMoveToPartner(int sd, char *finalLine, int currentPort, int seqNumber, int partnerIndex);
void sendToAllServers(int sd, char *buffer, int currentPort, int myLocation, int isResend);
void sendToPartners(int sd, char *finalLine, int currentPort, int seqNumber);
int verifyLocationNumber(char *loc);
int verifyPortNumber(char *pn);
void verifyServerIP(char *serverIP);