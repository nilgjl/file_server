#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//协议头
struct proto_head{
	int status;
	char filename[64];
	off_t file_size;
	//off_t offset;
};
