#include "dhelper.h"

typedef struct PartnersHost
{
  char ipAddress[25];
  int portNumber;
  int seqNumber; // current one for that host
  int location;
  bool hasAcked[MAXSEQNUMBER];
  bool sentAck[MAXSEQNUMBER];
} partnersHost;

static int maxHosts = 0;
static partnersHost hostInfo[MAXPARTNERS];

extern node *head; // this is so sad, but I need to be able to access this linked list to be able to ack back!
extern node **llarray;
extern int ll_index;
extern int MY_LOCATION; //need to accesss and update drone location!

int findPartnerIndex(int portNumber)
{
  int i = 0;
  while (i < maxHosts)
  {
    if (hostInfo[i].portNumber == portNumber)
      return i;
    ++i;
  }

  return -1;
}

void initPartners(FILE *configFile, int currentPort)
{
  char serverIP[20]; // provided by the user on the command line */
  int portNumber, i = 0;
  int location;
  char *line = NULL; // line in a file
  size_t len = 0;    // holds the current line in the file
  char *token;

  fseek(configFile, 0, SEEK_SET);

  while (i < MAX_SERVERS && getline(&line, &len, configFile) != -1)
  {
    // read config file line by line parse the line to get the portNumber and serverIP
    token = strtok(line, " "); //  get IP address
    strcpy(serverIP, token);
    verifyServerIP(token);

    token = strtok(NULL, " ");                    // get port #
    portNumber = verifyPortNumber(strdup(token)); // verifies and returns integer

    if (currentPort == portNumber)
      continue; // don't wanna send to itself

    // get location without the newline
    token = strtok(NULL, "\n");
    location = verifyLocationNumber(strdup(token)); // verifies and returns integer

    // add all those values to the array of partners
    strcpy(hostInfo[i].ipAddress, serverIP);
    hostInfo[i].portNumber = portNumber;
    hostInfo[i].seqNumber = 1;
    hostInfo[i].location = location;

    i++;
  }

  maxHosts = i; // how many were collected in the config

  if (line)
    free(line);
}

int getMyLocation(FILE *configFile, int currentPort)
{
  int currentLocation, portNumber;
  char *line = NULL; // line in a file
  size_t len = 0;    // holds the current line in the file
  char *token;

  fseek(configFile, 0, SEEK_SET); // reset fp to beginning
  while (getline(&line, &len, configFile) != -1)
  {
    token = strtok(line, " ");                             // get IP address, won't verify
    token = strtok(NULL, " ");                             // get port #
    portNumber = verifyPortNumber(strdup(token));          // verifies and returns integer
    token = strtok(NULL, "\n");                            // get location without the newline
    currentLocation = verifyLocationNumber(strdup(token)); // verifies and returns integer

    if (portNumber == currentPort)
      return currentLocation;
  }
  if (line)
    free(line);

  return -1; // means couldn't find that port number
}

int verifyLocationNumber(char *loc)
{
  int location = 0; // provided by the user on the command line
  int i;            /* loop variable */

  /* check that the port number is a number..... */
  for (i = 0; i < strlen(loc); i++)
  {
    if (!isdigit(loc[i]))
    {
      printf("The location isn't a number! - %s\n", loc);
      exit(1);
    }
  }
  location = strtol(loc, NULL, 10); /* many ways to do this */
  /* exit if a location too big or too small  */
  if ((location > 50) || (location < 0))
  {
    printf("you entered an invalid socket number\n");
    exit(1);
  }

  return location;
}

void handleLatestMessage(int sd, char *buffer, int portNumber, int myLocation, int rows, int columns)
{
  int toPort, msgTTL, msgLoc, isPrintable, isForwardable, isWithinRange, isMoveMessage;

  /*
  TODO - use global MY_LOCATION now instead of passing
      Do not forward to those to a node already in the send-path 
      (gonna have to make method in linkedlist to parse that string with commas into array)
  */
  // invalid ll should be dealt with!
  if (isCurrentListValid())
  { 
    updateSendPath(portNumber);
    toPort = getIntValue(head, "toPort");
    msgTTL = getIntValue(head, "TTL");
    msgLoc = getIntValue(head, "location");

    isWithinRange = isInRange(msgLoc, myLocation, rows, columns);
    isMoveMessage = (toPort == portNumber) && hasKey(head, "move");
    isPrintable = (toPort == portNumber) && isWithinRange && msgTTL >= 0;
    isForwardable = (toPort != portNumber) && isWithinRange && msgTTL > 0;
    
    setIsForwardMsg(0);

    if(isMoveMessage)
    {
      printList(head);
      handleMoveMessage(getIntValue(head, "move"));
      freeList(&head); //delete move message cuz we don't need to send it anywhere
      reSend(sd, portNumber);
    }
    else if(isPrintable)
    {
      printList(head);
      handleACK(sd, portNumber, myLocation);  
    }
    else if (isForwardable)
    {
      //TODO - optimize -> get message that is in range and NOT for you (TTL IS pos.), then check send-path. 
      prepareToForward(myLocation, msgTTL);
      sendToAllServers(sd, buffer, portNumber, myLocation, true);
    }
  }

  // next ll can now be received cuz this one's been handled
  if(head) 
    createNewList();
}

void handleMoveMessage(int newLocation)
{
  int oldLocation = MY_LOCATION;
  MY_LOCATION = newLocation;
  printf("Old Location: %d, New Location: %d\n", oldLocation, MY_LOCATION);
}

void handleACK(int sd, int portNumber, int myLocation)
{
  int msgSeqNum = getIntValue(head, "seqNumber");
  int originalSender = getIntValue(head, "fromPort");
  int i = findPartnerIndex(originalSender);

  if (hasPair(head, "type", "ACK"))
  {
    hostInfo[i].hasAcked[msgSeqNum - 1] = true;
  }
  else
  {
    if (hostInfo[i].sentAck[msgSeqNum - 1])
    {
      printf("DUPLICATE!!! seqNumber:%d fromPort (originalSender):%d\n", msgSeqNum, originalSender);
    }
    sendACK(sd, portNumber, myLocation);
    hostInfo[i].sentAck[msgSeqNum - 1] = true;
  }
}

void sendACK(int sd, int portNumber, int myLocation)
{
  char receiver[25], originalSender[25];
  char resetTTL[25], loc[25], seqNumber[25];
  char finalLine[1000];

  snprintf(receiver, 25, "%d", portNumber);
  strcpy(originalSender, getStringValue(head, "fromPort"));
  snprintf(resetTTL, 25, "%d", TTL);
  snprintf(loc, 25, "%d", myLocation);
  strcpy(seqNumber, getStringValue(head, "seqNumber"));

  updateValue(head, "fromPort", (receiver + 0));
  updateValue(head, "toPort", (originalSender + 0));
  updateValue(head, "send-path", (receiver + 0));
  updateValue(head, "TTL", (resetTTL + 0));
  updateValue(head, "location", (loc + 0)); // TODO - should I add a seperate myLocation and location pair to differentiate the two?
  addNode("type", "ACK");

  setIsForwardMsg(1);
  strcpy(finalLine, getForwardedLL(head));
  printf("sending to peers: '%s'\n", finalLine);

  sendToPartners(sd, strdup(finalLine), portNumber, getIntValue(head, "seqNumber"));
}

void sendToAllServers(int sd, char *buffer, int currentPort, int myLocation, int isResend)
{
  char finalLine[300];
  void *forwardedMsg;
  bool isAckMessage = head != NULL && hasPair(head, "type", "ACK"); // an ack is a type that will have no
  bool isFreshMessage = !isResend && !isAckMessage; //a whole new message going out into the bright, new world!
  bool isFreshMoveMessage = false;
  int seqNumber = -1;
  int partnerIndex;
  int toPortNumber;

  if (isResend)
  {
    forwardedMsg = getForwardedLL(head);
    if (forwardedMsg != NULL)
      strcpy(finalLine, (char *)forwardedMsg);
    else
      return;
  }
  else
  { /* ACK msg or fresh message from command line - first time being sent anywhere so it needs to add these to prime the pump */
    snprintf(finalLine, 300, "fromPort:%d TTL:%d location:%d send-path:%d seqNumber:^ ", currentPort,
             TTL, myLocation, currentPort);
    strcat(finalLine, buffer);
    
    if (isFreshMessage)
    {
      parseLine(strdup(finalLine));
      toPortNumber = getIntValue(head, "toPort");
      if(toPortNumber == currentPort)
      {
        perror("don't send urself a message dummy");
        exit(1);
      }
      partnerIndex = findPartnerIndex(toPortNumber);
      seqNumber = hostInfo[partnerIndex].seqNumber;
      hostInfo[partnerIndex].seqNumber += 1; // should only increment when fresh message is sent to final destination partner
      isFreshMoveMessage = hasKey(head, "move");
    }
      
  }

  if(seqNumber < 0) //ACK msg and basic resend message will not change the seqNumber for the partner
    seqNumber = getIntValue(head, "seqNumber");

  if(isFreshMoveMessage)
    sendMoveToPartner(sd, (finalLine + 0), currentPort, seqNumber, partnerIndex);
  else
    sendToPartners(sd, (finalLine + 0), currentPort, seqNumber);
}

/* resend all currently saved messages and decrement the TTL of the message*/
void reSend(int sd, int portNumber)
{
  int i;
  int seqNumber;
  char buffer[1000];

  for (i = 0; i < ll_index; i++)
  {
    if ((llarray)[i] == NULL)
      continue;
    memset(buffer, '\0', 1000);
    setIsForwardMsg(1);
    strcpy(buffer, getForwardedLL((llarray)[i]));
    #ifdef DEBUG
      printf("resend: %s\n", buffer);
    #endif
    seqNumber = getIntValue((llarray)[i], "seqNumber");
    sendToPartners(sd, (buffer + 0), portNumber, seqNumber);
    decrementTTL(&llarray[i]); //frees list once it hits 0!
  }
}

void sendToPartners(int sd, char *finalLine, int currentPort, int seqNumber)
{
  int i = 0;
  char serverIP[20];
  struct sockaddr_in server_address;
  int portNumber;        // holds every possible portnumber that the msg will be sent to
  char seqNum[25];

  while (i < maxHosts)
  {
    strcpy(serverIP, hostInfo[i].ipAddress);
    portNumber = hostInfo[i].portNumber;

    if (currentPort == portNumber)
      continue; // don't wanna send to itself

    server_address.sin_family = AF_INET;                  /* use AF_INET addresses */
    server_address.sin_port = htons(portNumber);          /* convert port number */
    server_address.sin_addr.s_addr = inet_addr(serverIP); /* convert IP addr */

    snprintf(seqNum, 25, "%d", seqNumber);

    finalLine = replace_str(finalLine, "^", (seqNum + 0));

    sendLine(sd, strdup(finalLine), &server_address);

    i++;
  }

  if(head) 
    createNewList();
}

 /* sending change of location to designated partner ONLY*/
void sendMoveToPartner(int sd, char *finalLine, int currentPort, int seqNumber, int partnerIndex)
{
  char serverIP[20];
  struct sockaddr_in server_address;
  int portNumber;        // holds every possible portnumber that the msg will be sent to
  char seqNum[25];

  //updating partner's location before sending the move message
  hostInfo[partnerIndex].location = getIntValue(head, "move"); // ! not sure this will change anything regarding logic cuz this is never used

  strcpy(serverIP, hostInfo[partnerIndex].ipAddress);
  portNumber = hostInfo[partnerIndex].portNumber;

  if (currentPort == portNumber)
    return; // don't wanna send to itself

  server_address.sin_family = AF_INET;                  /* use AF_INET addresses */
  server_address.sin_port = htons(portNumber);          /* convert port number */
  server_address.sin_addr.s_addr = inet_addr(serverIP); /* convert IP addr */

  snprintf(seqNum, 25, "%d", seqNumber);

  finalLine = replace_str(finalLine, "^", (seqNum + 0));

  sendLine(sd, strdup(finalLine), &server_address);

  freeList(&head); //delete move message cuz it's been sent off
}

int sendLine(int sd, char *line, struct sockaddr_in *server_address)
{
  int rc;

  rc = sendto(sd, line, strlen(line), 0,
              (struct sockaddr *)server_address, sizeof(*server_address));

  /* Check the RC and figure out what to do if it is 'bad'     */
  if (rc < 0)
  {
    perror("sendto");
    exit(1);
  }

  return rc;
}

void *handleFileOpening(char *fileName)
{
  FILE *inputFile;
  if ((inputFile = fopen(fileName, "r")) == NULL)
  {
    printf("Error opening the input file '%s'\n", fileName);
    exit(1);
  }

  return inputFile;
}

void verifyServerIP(char *serverIP)
{
  struct sockaddr_in inaddr; /* structures for checking addresses */
  /* this code checks to see if the ip address is a valid ip address */
  /* meaning it is in dotted notation and has valid numbers          */
  if (!inet_pton(AF_INET, serverIP, &inaddr))
  {
    printf("error, bad ip address\n");
    exit(1); /* just leave if is incorrect */
  }
}

/* check to see if the right number of parameters was entered */
int verifyPortNumber(char *pn)
{
  int portNumber = 0; // provided by the user on the command line
  int i;              /* loop variable */

  /* check that the port number is a number..... */
  for (i = 0; i < strlen(pn); i++)
  {
    if (!isdigit(pn[i]))
    {
      printf("The Portnumber isn't a number! - '%s'\n", pn);
      printf("%c\n", pn[i]);
      exit(1);
    }
  }
  portNumber = strtol(pn, NULL, 10); /* many ways to do this */
  /* exit if a portnumber too big or too small  */
  if ((portNumber > 65535) || (portNumber < 0))
  {
    printf("you entered an invalid socket number\n");
    exit(1);
  }

  return portNumber;
}

int receiveLine(int sd, int flags)
{
  struct sockaddr_in from_address; /* address of sender */
  char bufferReceived[1000];       // used in recvfrom
  socklen_t fromLength;
  int rc;
  // bool isCompleteList = false;
  /* NOTE - you MUST MUST MUST give fromLength an initial value */
  fromLength = sizeof(struct sockaddr_in);

  memset(bufferReceived, 0, 1000); // zero out the buffers in C

  rc = recvfrom(sd, bufferReceived, 1000, flags,
                (struct sockaddr *)&from_address, &fromLength);

  /* check for any possible errors */
  if (rc < 0)
  {
    perror("recvfrom");
    printf("leaving, due to socket error on recvfrom\n");
    exit(1);
  }
  else if (rc > 0)
  {
    /* parse the line and add to linked list */
    parseLine(strdup(bufferReceived));

    if(isCurrentListDuplicate(head))
    {//we don't want duplicates, so delete them and no sending
      printf("we don't store or send duplicates!\n");
      rc = false;
      freeList(&head);
    }
  }
  // else case does nothing cuz nothing is received

  return rc;
}

/* parses a given line and adds it to the linked list*/
void parseLine(char *line)
{                      // kinda gross, refactor would be in order
  int i, isString = 0; // counter and boolean for when the value is a string with "" marks
  char key[50], value[50];
  char c, colon = ':', dquotes = '\"'; // current character
  int isKey = 1;                       // first non whitespace string should be a key

  memset(key, 0, 50);
  memset(value, 0, 50);

  for (i = 0; i < strlen(line); i++)
  {
    c = line[i]; // char stored to simplify

    // skip whitespace characters that aren't part of a string value
    if (isspace(c) && !isString)
    {
      isKey = 1;
      if (strlen(value) > 0 && strlen(key) > 0)
      {
        addNode(strdup(key), strdup(value));
        memset(key, 0, 50);
        memset(value, 0, 50);
      }
    }
    else if (c == dquotes && !isString) // first quotation mark
    {
      isString = 1;
      strncat(value, &c, 1);
    }
    else if (c == dquotes && isString) // last quotation mark
    {
      isString = 0;
      strncat(value, &c, 1);
      isKey = 1; // key will be next after whitespace
    }
    else if (isspace(c) && isString)
      strncat(value, &c, 1);
    else if (c == colon)
      isKey = 0; // going to the value
    else if (isKey)
      strncat(key, &c, 1);
    else if (!isKey)
      strncat(value, &c, 1);
    else
      printf("need to see what this case is at i: %d\n", i);
  }

  if (strlen(value) > 0 && strlen(key) > 0)
  {
    addNode(strdup(key), strdup(value));
    memset(key, 0, 50);
    memset(value, 0, 50);
  }
}

int createSocket()
{
  int sd = socket(AF_INET, SOCK_DGRAM, 0); /* create a socket */

  if (sd == -1)
  { /* means some kind of error occured */
    perror("socket");
    exit(1); /* just leave if wrong number entered */
  }
  return sd;
}

int checkUserInput(int argc, char *argv[])
{
  int i, portNumber;

  /* first, decide if we have the right number of parameters */
  if (argc < 2)
  {
    printf("usage is: drone3 <portnumber>\n");
    exit(1);
  }

  /* now fill in the address data structure we use to sendto the server */
  for (i = 0; i < strlen(argv[1]); i++)
  {
    if (!isdigit(argv[1][i]))
    {
      printf("%c\n", argv[1][i]);
      printf("The Portnumber isn't a number!\n");
      exit(1);
    }
  }

  portNumber = strtol(argv[1], NULL, 10); /* many ways to do this */

  if ((portNumber > 65535) || (portNumber < 0))
  {
    printf("you entered an invalid port number\n");
    exit(1);
  }

  return portNumber;
}

/* borrowed work to replace substring with another string and return the new string*/
char *replace_str(char *str, char *orig, char *rep)
{
  static char buffer[4096];
  char *p;

  if (!(p = strstr(str, orig))) // Is 'orig' even in 'str'?
    return str;

  strncpy(buffer, str, p - str); // Copy characters from 'str' start to 'orig' st$
  buffer[p - str] = '\0';

  sprintf(buffer + (p - str), "%s%s", rep, p + strlen(orig));

  return buffer;
}