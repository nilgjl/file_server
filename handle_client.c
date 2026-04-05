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

// 构造并发送协议头
// 成功返回 0，协议头发送失败返回-2
int send_protocol_header(int cfd, int fd, const char *filename, int status) {
	struct proto_head head;
	//构造协议头
	head.status = status;
	strncpy(head.filename, filename, sizeof(head.filename) - 1);
	head.filename[sizeof(head.filename) - 1] = '\0'; 
	if (status == 100)//获取文件长度
	{
		struct stat st;
	        fstat(fd, &st);
        	head.file_size = st.st_size;
	}
	// 发送头部
	int ret = send(cfd, &head, sizeof(head), 0);
	if (ret == -1) {
		fprintf(stderr, "[pthread %lu] send head failed:%s\n", pthread_self(), strerror(errno));
		return -2;
	}

	return 0;
}

// 发送文件数据
// 成功返回 0，文件信息获取失败返回-1，发送失败返回-3
int send_file_data(int cfd, int fd)
{
	char buffer[4096];
	ssize_t n;
	while ((n = read(fd,buffer,sizeof(buffer)))>0)
	{
		if (send(cfd,buffer,n,0) == -1)
		{
			fprintf(stderr, "[pthread %lu] send failed:%s\n", pthread_self(), strerror(errno));
			return -3;
		}
	}
	return 0;
}


// 向客户端发送文件
// 成功返回 0，文件信息获取失败返回-1,协议头发送失败返回-2,文件发送失败返回-3
int send_file_to_client(int cfd, const char *nfilename,const char *filename,int status)
{
	//判断文件是否能打开
        int fd = open(nfilename,O_RDONLY);
        if (fd == -1)
	{
		fprintf(stderr, "[pthread %lu] open failed:%s\n", pthread_self(), strerror(errno));
                status = 404;
	}
	int ret;
	//发送头部
	if ((ret= (send_protocol_header(cfd, fd, filename, status))) != 0)
	{
		if (fd != -1)
			close(fd);
        	return ret;
	}
	//发送数据
	if (status == 100)
	{
		if ((ret= (send_file_data(cfd, fd))) != 0)
        	        return ret;
		close(fd);
	}
	return 0;
}

//完成连接、接收和发送消息
void* handle_client(void* arg)
{
	int client_fd = (int)(uintptr_t)arg;

	//1.接收消息
	char buffer[1024]={0};
	ssize_t n = read(client_fd,buffer,sizeof(buffer)-1);
	if (n <= 0)
	{
		if (n == 0)
			printf("客户端正常断开\n");
		else
			fprintf(stderr, "[pthread %lu] read failed:%s\n", pthread_self(), strerror(errno));

		close(client_fd);
                return NULL;
        }
	printf("[pthread %lu] 收到消息：%s\n", pthread_self(), buffer);

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

	//3.发送回复
	int status = 100;	
	//命令不支持
	if(res==0)
	{
		status = 400;
		printf("[pthread %lu] 客户端发送了不支持的命令：%s\n", pthread_self(), buffer);
	}
	//命令支持
	else
	{	
		// 检查文件名是否合法
		int rc=is_valid_filename(filename);
		if(rc==-1)//不合法
		{
			status = 400;
                	printf("[pthread %lu] 客户端请求文件不合法：%s\n", pthread_self(), buffer);
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
                        	printf("[pthread %lu] 无法找到客户端请求文件：%s\n", pthread_self(), buffer);
			}
			else
			{
                        	printf("[pthread %lu] 开始向客户端发送协议头和文件：%s\n", pthread_self(), buffer);

				if(err == -2)
				{
                                	printf("[pthread %lu] 向客户端发送协议头失败\n", pthread_self());
				}
				else if(err == -3)
				{
                                	printf("[pthread %lu] 向客户端发送文件失败\n", pthread_self());
				}
				else
				{
                                	printf("[pthread %lu] 向客户端发送文件成功\n", pthread_self());
				}
			}
		}
	}

	//4.关闭
	close(client_fd);
	return NULL;
}
