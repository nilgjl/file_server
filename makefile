src = $(wildcard *.c)
obj = $(patsubst %.c, %.o, $(src))

all: server client

server: server.o handle_client.o
	gcc $^ -o $@ -Wall -g

client: client.o
	gcc $^ -o $@ -Wall -g

$(obj): %o: %c
	gcc -c $< -o $@ -Wall  

clean:
	rm -f $(obj) server client

.PHONY: all clean
