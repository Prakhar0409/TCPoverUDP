CC = gcc

all: receiver sender


receiver: receiver.c
	$(CC) -o receiver receiver.c

sender: sender.c
	$(CC) -o sender sender.c

clean:
	rm sender receiver
