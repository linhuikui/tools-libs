.SUFFIXES : .x .o .c .s

TOOLCHAIN = arm-fsl-linux-gnueabi-
CC = $(TOOLCHAIN)gcc
AR = $(TOOLCHAIN)ar

BUSYBOX = 

INCLUDE = -I./ \
		  -I$(BUSYBOX) \
		  

LDFLAGS = -lc -lgcc 


CFLAGS := $(INCLUDE) -Wall -c -O2


#STRIP=$(TOOLCHAIN)strip
VPATH = %.c  $(BUSYBOX)

#Êä³ö¶Î¶¨Òå
TARGET = ulibc.a

SRC =$(wildcard *.c $(BUSYBOX)/*.c)

DIR=$(notdir $(SRC))
OBJ=$(patsubst %.c,%.o,$(DIR))

all:$(TARGET)

$(TARGET):$(OBJ)
	$(AR) crv $@ $^
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@


.PHONY: clean cleanobj
cleanobj:
	rm  -f $(OBJ)
clean:
	rm  -f $(OBJ)

