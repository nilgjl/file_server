#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

//协议头
struct proto_head{
	int status;
	char filename[256];
	off_t file_size;
	//off_t offset;
};

void* handle_client(void*);
