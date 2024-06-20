CC = gcc
CFLAGS = -Wall -Wextra -lpthread

SRC = main.c source.c
HDR = header.h
OBJ = $(SRC:.c=.o)
TARGET = iperf_adi

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ 

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)

