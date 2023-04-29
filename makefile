# Makefile for drone8

CC = gcc
#OBJCS = client.c
DRONECS = drone8.c dhelper.c linkedlist.c euclidean.c
#OBJCSS = server.c linkedlist.c

CFLAGS =  -g -Wall
# setup for system
LIBS = -lm

all: labcode tags headers

#all executables for lab to work
labcode: drone8

drone8: $(DRONECS)
	$(CC) $(CFLAGS) -o $@ $(DRONECS) $(LIBS) 

clean:
	rm -f drone8 *.vs tags headers
	clear

# I get these mixed up, so I have $make clean work as well
clear: clean
	
# this entry stays for C code labs - Systems 1
headers: *.c tags
	headers.sh	

#this entry stays for C code labs - Systems 1
tags: *.c
	ctags -R .