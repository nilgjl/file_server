#include "common.h"
const int MAX_CLIENTS = 100;

int main()
{
	//1.创建socket
	int server_fd = socket(AF_INET,SOCK_STREAM,0);
	if(server_fd<0)
	{
		perror("socket failed");
		return -1;
	}
	//2.绑定地址
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(8888);
	if(bind(server_fd,(struct sockaddr*)&address,sizeof(address))<0)
	{
		perror("bind failed");
		close(server_fd);
		return -1;
	}

	//3.监听
	if(listen(server_fd,3)<0)
	{
		perror("listen failed");
                close(server_fd);
                return -1;
        }
	printf("服务器启动，监听端口8888\n");
	printf("等待客户端连接...\n");
	
	pthread_t tid;
	int ret;

	while(1)
	{
		//接受连接
        	socklen_t addr_len=sizeof(address);
        	int client_fd = accept(server_fd,(struct sockaddr*)&address,&addr_len);
        	if(client_fd < 0)
        	{
                	fprintf(stderr, "accept failed:%s\n", strerror(errno));
                	close(server_fd);
                	return -1;
        	}
        	printf("客户端已连接\n");

		//创建线程
		ret = pthread_create(&tid, NULL, handle_client,(void *)(uintptr_t)client_fd);
		if (ret != 0)
		{
			fprintf(stderr,"pthread create failed:%s\n",strerror(ret));
		}

		//分离线程
		ret = pthread_detach(tid);
		if (ret != 0)
		{
			fprintf(stderr,"pthread detach failed:%s\n",strerror(ret));
		}
	}

	close(server_fd);
	return 0;
}
