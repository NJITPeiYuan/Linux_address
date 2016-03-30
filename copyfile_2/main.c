#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>

#define MAX 1024

int read_line(int fd, char *buf, int len)//len为buf所指内存的字节数
{
    int i;

    char ch;

    int n_read;

    for(i = 0; i < len; i++)
    {
        n_read = read(fd,&ch,1);//读一个字节

	if(n_read == -1)
	{
	    return -1;
	}

	if(n_read == 0)
	{
	    return 0;
	}

        if(ch == '\n')    //当读到换行符时，break；表示读一行结束
	{
	    break;
	}

	*buf = ch;
	buf++;
    }

    *buf = '\0';

    return i;
}

void add(int *data1, int *data2, int *data3)//进行加法运算
{
    int i;

    for(i = 0; i < 3; i++)
    {
        *(data3 + i) = *(data1 + i) + *(data2 + i);
    }
}


void parse_data(char *src, int *data)
{
    int len = 0;

    char *temp = (char *)malloc(sizeof(char) * 100);
    
    while(*src != '\0')                 //当遇到空格时，把空格之前的数据放到temp所指的内存中，再调用atoi函数将字符串转换为数字
    {                                   //放到data指向的内存中
        if(*src == ' ')                 //同时len清零，表示下次再遇到空格时，所要复制的字符串为从上次遇到空格的下一个字符开始到
	{                               //最近的空格
            strncpy(temp,src - len,len);
	    *data = atoi(temp);
	    data++;
	    len = 0;
	}

	len++;

	src++;
    }

    strncpy(temp,src - len,len);        //将最后一次遇到空格的情况进行处理
    *data = atoi(temp);
}

int handle_data(int fd1, int fd2, int fd3)
{
    char buf1[MAX];
    char buf2[MAX];

    int data1[3];
    int data2[3];
    int data3[3];

    char result[MAX];

    read_line(fd1,buf1,sizeof(buf1));//读第一行
    read_line(fd2,buf2,sizeof(buf2));

    if(strcmp(buf1,"begin") != 0 || strcmp(buf2,"begin") != 0)//只要buf1 buf2中有一个不是以begin开头的 就不成立
    {
        return -1;
    }
    
    write(fd3,"begin",strlen("begin"));//把begin写到fd3所指的文件中去
    write(fd3,"\n",1);//换行
    
    memset(buf1,0,sizeof(buf1));//清空buf指针所指的内存中的内容
    memset(buf2,0,sizeof(buf2));

    read_line(fd1,buf1,sizeof(buf1));//读下一行
    read_line(fd2,buf2,sizeof(buf2));
    
    while(strcmp(buf1,"end") != 0 && strcmp(buf2,"end") != 0)//当buf1 buf2都没有到最后一行（end）时，进入循环
    {
       
	parse_data(buf1,data1);字符串转换成数字
	parse_data(buf2,data2);
	
	add(data1,data2,data3);
	
	sprintf(result,"%d %d %d%c",data3[0],data3[1],data3[2],'\n');
        
	write(fd3,result,strlen(result));//将进行加法运算后的数据写进fd3所指向的文件中

        memset(buf1,0,sizeof(buf1));
        memset(buf2,0,sizeof(buf2));
	
	read_line(fd1,buf1,sizeof(buf1));
        read_line(fd2,buf2,sizeof(buf2));
    }

    write(fd3,"end",strlen("end"));//将fd3所指的文件在最后面添上end 以表结束
    write(fd3,"\n",1);//换行符
}

int main(int argc, char *argv[])
{
    int fd1;
    int fd2;
    int fd3;

    if(argc != 4)                  //要求参数必须要有4个 argv[0]装字符串数组的路径，argv[1].argv[2]装被处理的两个字符串，argv[3]
    {                              //装处理后合并的数据
        printf("param error!\n");
	exit(1);
    }

    if((fd1 = open(argv[1],O_RDONLY)) == -1) // 要求argv[1]只能进行读操作
    {
        perror("open file error!");
	exit(1);
    }
    
    if((fd2 = open(argv[2],O_RDONLY)) == -1)//要求argv[2]只能进行读操作
    {
        perror("open file error!");
	exit(1);
    }
    
    if((fd3 = open(argv[3],O_CREAT | O_WRONLY)) == -1)//argv[3]进行写操作
    {
        perror("open file error!");
	exit(1);
    }

    handle_data(fd1, fd2, fd3);//调用函数处理字符串

    
    return 0;
}
