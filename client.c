#include "common.h"

//接收协议头并判断状态
//成功接收且状态正常返回0,否则返回-1
int recv_protocol_header(int sock,struct proto_head *head)
{
	ssize_t n = recv(sock,head,sizeof(struct proto_head),0);
	if (n != sizeof(struct proto_head))
	{
		perror("recv proto head failed");
		return -1;
	}

	head->filename[sizeof(head->filename) - 1] = '\0';
	//判断状态
	switch (head->status)
	{
		case 400:
			printf("400 Bad Request\n");
			return -1;
		case 404:
			printf("404 Not Found\n");
			return -1;
		case 100:
			printf("100 Continue\n");
			return 0;
		default:
			printf("未知错误\n");
			return -1;
	}
}

//接收文件
//成功返回0,失败返回-1
int recv_file_content(int sock, const char* save_path,off_t file_size)
{
	int fd = open(save_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
	{
		perror("open file failed");
		return -1;
	}

	char buf[4096];
	off_t have_read = 0, read_size;
	int n;
	while (have_read < file_size)
	{
		read_size = (file_size - have_read) < sizeof(buf) ? (file_size - have_read) : sizeof(buf);
		n = recv(sock, buf, read_size, 0);
		if (n <= 0)
		{
			perror("recv failed");
			close(fd);
			return -1;
		}

		if (write(fd, buf, n) != n)
		{
			perror("write failed");
			close(fd);
			return -1;
		}
		
		have_read += n;
	}
	close(fd);
	return 0;
}

int main()
{
	//1. 创建socket
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock<0)
	{
		perror("socket failed");
		return -1;
	}

	//2.设置服务器地址
	struct sockaddr_in server;
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=inet_addr("127.0.0.1");
	server.sin_port=htons(8888);
	
	//3.连接
	if(connect(sock,(struct sockaddr*)&server,sizeof(server))<0)
	{
		perror("connect failed");
		close(sock);
		return -1;
	}
	printf("已连接到服务器\n");
	
	//4.发送
	const char* requested_filename = "large.bin";
	char msg[256];
	snprintf(msg, sizeof(msg), "GET %s", requested_filename); 
	if(send(sock,msg,strlen(msg),0)<0)
	{
		perror("send failed");
                close(sock);
                return -1;
        }
	printf("发送：%s\n",msg);

	//5.回复
     	  //接收协议头,判断状态
     	struct proto_head head;
     	if (recv_protocol_header(sock, &head) < 0) {
         	close(sock);
         	return -1;
     	}
	  //判断文件名
	if (strcmp(requested_filename, head.filename) != 0)
	{
		printf("接收到的文件与请求不符\n");
		close(sock);
		return -1;
	}
	
	char save_path[128];
	snprintf(save_path, sizeof(save_path), "downloaded_%lu.txt", pthread_self());

     	  // 接收文件
     	if (recv_file_content(sock, save_path, head.file_size) == -1)
	{
		printf("文件接收失败\n");
		close(sock);
		return -1;
	}
	else
	{
		printf("文件接收成功\n");
	}

	  //接收传输结束状态
	char buf[256];
	ssize_t n  = recv(sock, buf, sizeof(buf), 0);
	if (n <= 0)
	{
		perror("recv failed");
		close(sock);
		return -1;
	}
	if (strncmp(buf, "101", 3) != 0)
	{
		printf("未收到文件传输结束状态\n");
		close(sock);
		return -1;
	}
	else
		printf("%s",buf);

	//6.关闭
	close(sock);

	return 0;
}
