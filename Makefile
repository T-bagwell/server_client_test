CC=gcc
CFLAGS= -Wall -g -I.

OBJS_CLIENT	+= \
	   socket_client.o

OBJS_SERVER	+= \
	   socket_server.o
		
TARGET_CLIENT=test_socket_client
TARGET_SERVER=test_socket_server

all:$(OBJS_CLIENT) $(OBJS_SERVER)
	$(CC) $(CFLAGS) -o $(TARGET_CLIENT) $(OBJS_CLIENT)
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) $(OBJS_SERVER)

clean:
	rm -rf $(TARGET_CLIENT) $(OBJS_CLIENT) $(TARGET_SERVER) $(OBJS_SERVER)
