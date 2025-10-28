CC = gcc

all: index_builder server_main client_main

index_builder: index_builder.c MurmurHash2.c definitions.h
	$(CC) index_builder.c MurmurHash2.c -o index_builder

server_main: server_main.c worker_process.c MurmurHash2.c definitions.h
	$(CC) server_main.c worker_process.c MurmurHash2.c -o server_main

client_main: client_main.c ui_process.c definitions.h
	$(CC) client_main.c ui_process.c -o client_main

clean:
	rm -f index_builder server_main client_main