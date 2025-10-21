CC = gcc

all: index_builder main

index_builder: index_builder.c MurmurHash2.c 
	$(CC) index_builder.c MurmurHash2.c -o index_builder

main: main.c worker_process.c ui_process.c MurmurHash2.c
	$(CC) main.c worker_process.c ui_process.c MurmurHash2.c -o main  

clean:
	rm -f index_builder main
