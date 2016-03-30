#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>

#define MAX_SIZE 1024

int main(int argc, char *argv[])
{
    int r_fd;
    int w_fd;
    
    int r_len;
    char buf[MAX_SIZE];
    
    if((r_fd = open(argv[1], O_RDONLY)) < 0)  //函数出错返回－1，成功返回一个整数。函数参数以只读方式打开
    {                                         //此函数打开的文件为作为复制的文件
        perror("open file error!");
	exit(1);
    }

    if((w_fd = open(argv[2], O_WRONLY | O_CREAT, 0655)) < 0) // 此函数打开的文件为空文件或者将要被上个文件复制的文件
    {                                                        //以只写方式打开，若文件不存在则自动创建
        perror("open file error!");
	exit(1);
    }

    while(r_len = read(r_fd, buf, sizeof(buf)))//把r_fd所指的文件传送1024个字节到buf所指的内存 返回值为实际读到的字节数
    {                                          //当有错误发生时返回－1
        if(r_len == -1)
	{
	    perror("read error!");
	    exit(1);
	}

	buf[r_len] = '\0';             //字符串结束标志

	write(w_fd, buf, strlen(buf));  //把buf所指的内存写1024个字节到w_fd所指的文件中

	memset(buf, 0, sizeof(buf));
    }	

    close(r_fd);
    close(w_fd);
    
    
    return 0;
}
