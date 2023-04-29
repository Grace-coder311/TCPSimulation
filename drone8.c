// Grace Workman Lab8
#include "dhelper.h"
int MY_LOCATION;

// gdb --args drone8 41001
int main(int argc, char *argv[])
{
    FILE *configFile; // file ptrs
    int sd, rc, portNumber;
    const int TIMEOUT_LENGTH = 20; //todo - this should be 20s
    struct sockaddr_in server_address;
    char buffer[1000];
    char *ptr = NULL, configFileName[] = "config.file";
    char *serverIP = "127.0.0.1"; // this should not change
    int flags = 0;
    int rows = ROWS, columns = COLUMNS; // user will input these values at the beginning, but I added a default
    fd_set socketFDS;                   // the socket descriptor set - for select
    int maxSD;                          // tells the OS how many sockets are set - for select
    struct timeval timeout;

    initLLA();

    sd = createSocket();
    portNumber = checkUserInput(argc, argv);

    signal(SIGPIPE, SIG_IGN); // ignore the sigpipe error

    configFile = (FILE *)handleFileOpening(configFileName); // open the config file

    initPartners(configFile, portNumber); // drone would like to know your location

    MY_LOCATION = getMyLocation(configFile, portNumber);
    if (MY_LOCATION < 0)
    {
        perror("could not find location for the current portnumber");
        exit(1);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(portNumber);
    server_address.sin_addr.s_addr = inet_addr(serverIP);

    rc = bind(sd, (struct sockaddr *)&server_address, sizeof(server_address));
    if (rc < 0)
    {
        perror("bind");
        exit(1);
    }

    printf("rows:%d and cols:%d\n", rows, columns);

    for (;;)
    {
        memset(buffer, 0, 1000);
        // do the select() stuff here
        FD_ZERO(&socketFDS);       // NEW
        FD_SET(sd, &socketFDS);    // NEW - sets the bit for the initial sd socket
        FD_SET(STDIN, &socketFDS); // NEW tell it you will look at STDIN too
        if (STDIN > sd)            // figure out what the max sd is. biggest number
            maxSD = STDIN;
        else
            maxSD = sd;
        timeout.tv_sec = TIMEOUT_LENGTH;
        timeout.tv_usec = 0;
        rc = select(maxSD + 1, &socketFDS, NULL, NULL, &timeout); // NEW block until something arrives
        if(rc == 0)
        { //had a timeout!
            reSend(sd, portNumber);
            printf ("timeout!\n");
            continue;
        }

        if (FD_ISSET(STDIN, &socketFDS))
        { // means i received something from the keyboard.
            ptr = NULL;
            memset(buffer, '\0', 1000);
            ptr = fgets(buffer, sizeof(buffer), stdin);
            if (ptr == NULL)
            {
                perror("error getting from keyboard");
                exit(1);
            }
            buffer[strlen(buffer) - 1] = 0; // get rid of \n that is there, cuz i don't want it

            sendToAllServers(sd, strdup(buffer), portNumber, MY_LOCATION, false);
        }
        if (FD_ISSET(sd, &socketFDS))
        { // got something from the network
            if(receiveLine(sd, flags))
            {
                handleLatestMessage(sd, strdup(buffer), portNumber, MY_LOCATION, rows, columns);
            }
        }
    }

    // Thnks fr th Mmrs
    freeAll(); // frees all ll's
    close(sd);
    fclose(configFile);

    return 0;
}
