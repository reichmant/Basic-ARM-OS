#Makefile for Phase 1 of JaeOS#

#Written by Mikey Goldweber and updated by Jake Wagner 
#Last Updated: 5-07-17#

SUPDIR = /usr/include/uarm

DEFS = ../h/const.h ../h/types.h ../e/asl.e ../e/pcb.e $(SUPDIR)/libuarm.h Makefile

CFLAGS =  -mcpu=arm7tdmi -c
LDCOREFLAGS =  -T $(SUPDIR)/ldscripts/elf32ltsarm.h.uarmcore.x

CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld

#main target
all: kernel.core.uarm

kernel.core.uarm: p1test.o asl.o pcb.o
	$(LD) $(LDCOREFLAGS) -o kernel.core.uarm p1test.o asl.o pcb.o $(SUPDIR)/crtso.o $(SUPDIR)/libuarm.o

p1test.o: p1test.c $(DEFS)
	$(CC) $(CFLAGS) p1test.c
 
asl.o: asl.c $(DEFS)
	$(CC) $(CFLAGS) asl.c

pcb.o: pcb.c $(DEFS)
	$(CC) $(CFLAGS) pcb.c



clean:
	rm -f *.o *.uarm kernel


distclean: clean
	-rm kernel.*.uarm
