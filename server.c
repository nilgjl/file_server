#include "common.h"

// 检查文件名是否合法（不包含路径穿越风险）
// 返回 0 表示合法，-1 表示非法
int is_valid_filename(const char *filename) {
    // 空文件名非法
    if (filename == NULL || *filename == '\0') {
        return -1;
    }
    // 禁止包含上级目录
    if (strstr(filename, "..") != NULL) {
        return -1;
    }
    // 禁止包含路径分隔符
    if (strchr(filename, '/') != NULL) {
        return -1;
    }
    return 0; // 合法
}

// 获取文件大小
// 成功返回文件大小，失败返回 -1
off_t get_file_size(const char *filename) {
	struct stat st;
	if (stat(filename, &st) == -1) {
		return -1;
	}
	return st.st_size;
}

// 构造并发送协议头
// 成功返回 0，协议头发送失败返回-2
int send_protocol_header(int cfd, const char *nfilename, const char *filename,int status) {
	struct proto_head head;
	//判断文件是否能打开
	int fd = open(nfilename,O_RDONLY);
        if (fd == -1)
		status = 404;
	else
		close(fd);
	//构造协议头
	head.status = status;
	strncpy(head.filename, filename, sizeof(head.filename) - 1);
	head.filename[sizeof(head.filename) - 1] = '\0'; 

	head.file_size = get_file_size(nfilename);//获取文件长度

	// 发送头部
	if (send(cfd, &head, sizeof(head), 0) < 0) {
		perror("send head failed");
		return -2;
	}

	return 0;
}

// 发送文件数据
// 成功返回 0，文件信息获取失败返回-1，发送失败返回-3
int send_file_data(int cfd, const char *filename)
{
	char buffer[1024];
	ssize_t n;
	int fd = open(filename,O_RDONLY);
	if (fd == -1){
		perror("open failed");
		return -1;
	}
	while ((n = read(fd,buffer,sizeof(buffer)))>0)
	{
		if (send(cfd,buffer,n,0) == -1)
		{
			perror("send failed");
			close(fd);
			return -3;
		}
	}
	close(fd);
	return 0;
}


// 向客户端发送文件
// 成功返回 0，文件信息获取失败返回-1,协议头发送失败返回-2,文件发送失败返回-3
int send_file_to_client(int cfd, const char *nfilename,const char *filename,int status)
{
	int ret;
	//发送头部
	if ((ret= (send_protocol_header(cfd, nfilename, filename, status))) != 0)
        	return ret;
	//发送数据
	if ((ret= (send_file_data(cfd, nfilename))) != 0)
                return ret;

	return 0;
}

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

	//4.接受连接
	socklen_t addr_len=sizeof(address);
	int client_fd = accept(server_fd,(struct sockaddr*)&address,&addr_len);
	if(client_fd < 0)
	{
		perror("accept failed");
                close(server_fd);
                return -1;
        }
	printf("客户端已连接\n");

	//5.接收消息
	char buffer[1024]={0};
	ssize_t n = read(client_fd,buffer,sizeof(buffer)-1);
	if(n<0)
	{
		perror("read failed");
		close(client_fd);
                close(server_fd);
                return -1;
        }
	printf("收到消息：%s\n",buffer);

	//判断命令是否支持
	int res;
	char filename[256]={0};
	if(strncmp(buffer,"GET ",3)==0)
	{
		sscanf(buffer,"GET %255s",filename);
		res=1;
	}
	else 
		res=0;

	//6.发送回复
	int status = 100;	
	//命令不支持
	if(res==0)
	{
		status = 400;
		printf("客户端发送了不支持的命令：%s\n",buffer);
	}
	//命令支持
	else
	{	
		// 检查文件名是否合法
		int rc=is_valid_filename(filename);
		if(rc==-1)//不合法
		{
			status = 400;
                	printf("客户端请求文件不合法：%s\n",buffer);
		}
		else//合法
		{
			//为文件添加路径
                	char nfilename[264];
                	snprintf(nfilename, sizeof(nfilename), "./files/%s", filename);

			//发送协议头文件
			int err=send_file_to_client(client_fd,nfilename,filename,status);
			if (err == -1)
			{
                        	printf("无法找到客户端请求文件：%s\n",buffer);
			}
			else
			{
                        	printf("开始向客户端发送协议头和文件：%s\n",buffer);

				if(err == -2)
				{
                                	printf("向客户端发送协议头失败\n");
				}
				else if(err == -3)
				{
                                	printf("向客户端发送文件失败\n");
				}
				else
				{
					const char *msg_101 = "101 Transfer Complete.OK.\n";
					send(client_fd,msg_101,strlen(msg_101),0);
                                	printf("向客户端发送文件成功\n");
				}
			}
		}
	}

	//7.关闭
	close(client_fd);
	close(server_fd);
	return 0;
}
