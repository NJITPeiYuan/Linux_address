#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h>

#include <sqlite3.h>

#include <termios.h>

#define portnumber 3333
#define MAX_SIZE 1024	

struct information
{
    int action;
    char ID[20];
    char username[20];
    char password[7];
    char toname[20];
    char msg[1024];
};
//typedef struct information info;

int flag = 1;

char my_getch()
{
    struct termios tm,tm_old;
    int fd=0;
    char ch;

    if(tcgetattr(fd,&tm)<0)
    {
        return -1;
    }

    tm_old=tm;
    cfmakeraw(&tm);

    if(tcsetattr(fd,TCSANOW,&tm)<0)
    {
        return -1;
    }

    ch=getchar();

    if(tcsetattr(fd,TCSANOW,&tm_old)<0)
    {
        return -1;
    }
    return ch;
}

void read_msg(void *arg)
{
    int fd = *((int *)arg);

    int n_read = 0;

  //  info i;

    struct information *Info = (struct information *)malloc(sizeof(struct information));

    while(1)
    { 
        n_read = read(fd, Info, sizeof(struct information));


//	memset(Info, 0, sizeof(struct information));

        if(n_read == 0)
        {
            close(fd);
	    pthread_exit(NULL);
        }

        switch(Info->action)
        {
	    case -1:
	    {
	        printf("register failure!\n");
		break;
	    }
	    case 0:
	    {
                printf("register success!\n");
		break;
	    }
            case 1:
	    {
	        printf("landing success!\n");
	        flag = 6;
		break;
	    }
	    case 2:
	    {
	        printf("%s to the msg:%s\n", Info->username, Info->msg);
	        break;
	    }
	    case -2:
	    {
	        printf("%s is not online!\n", Info->username);
	        break;
	    }
	    case 3:
	    {
		printf("%s to everyone say:%s\n",Info->username,Info->msg);
		break;
	    }
	    case 4:
	    {
		printf("管理员上线！\n");
		flag=2;
		break;
	    }
	    case 5:
	    {
	        printf("%s 下线成功!\n", Info->username);
		exit(1);
	    }
	    case 6:
	    {
		printf("%s当前在线\n",Info->username);
		break;
	    }
	    case 7:
	    {
	        printf("修改密码验证成功!\n");
		flag = 3;
		break;
	    }
	    case 8:
	    {
	        printf("修改密码成功!\n");
		break;
	    }
	    case 9:
	    {
	        printf("%s已被踢出！\n", Info->username);
		break;
	    }
	    case 10:
	    {
		printf("用户不存在！\n");
	        break;
            }	
	    case 11:
	    {
	        printf("该用户已登录!\n");
		break;
	    }
	    case 12:
	    {
	        printf("%s已被禁言!\n", Info->username);
		flag = 4;
		break;
	    }
	    case 13:
	    {
	        printf("%s已被解禁!\n", Info->username);
		flag = 5;
		break;
	    }
	    case 14:
	    {
	        printf("你还没有注册帐号!\n");
		break;
	    }
        }
    }
}

int main(int argc, char *argv[])
{ 
    int sockfd; 
    char buffer[1024]; 
    struct sockaddr_in server_addr; 
    int nbytes;

  //  int action;
  //  char ID[100];   
  //  char username[100];
    char password[100];

    char cmd[20];

    int n = 0;

    char ch;


    if(argc!=2) 
    { 
	fprintf(stderr,"Usage:%s hostname \a\n",argv[0]); 
	exit(1); 
    }

    /* 客户程序开始建立 sockfd描述符 */ 
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) // AF_INET:Internet;SOCK_STREAM:TCP
    { 
        fprintf(stderr,"Socket Error:%s\a\n",strerror(errno)); 
	exit(1); 
    } 

    /* 客户程序填充服务端的资料 */ 
    bzero(&server_addr,sizeof(server_addr)); // 初始化,置0
    server_addr.sin_family=AF_INET;          // IPV4
    server_addr.sin_port=htons(portnumber);  // (将本机器上的short数据转化为网络上的short数据)端口号
												
    server_addr.sin_addr.s_addr=inet_addr(argv[1]);  //用于绑定到一个固定IP,inet_addr用于把数字加格式的ip转化为整形ip
    /* 客户程序发起连接请求 */ 
    if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1) 
    { 
        fprintf(stderr,"Connect Error:%s\a\n",strerror(errno)); 
	exit(1); 
    } 

    pthread_t id;
    pthread_create(&id, NULL, read_msg, (void *)&sockfd);
    struct information *Info = (struct information *)malloc(sizeof(struct information));
    
    system("reset");

    printf("**************************************\n");
    printf("**************************************\n");
    printf("********注册***register***************\n");
    printf("********登录***landing****************\n");
    printf("********退出***quit*******************\n");
    printf("********显示***display****************\n");
    printf("**************************************\n");

    while(1)
    {
        memset(Info, 0, sizeof(struct information));

        sleep(1);

        printf("please input cmd:\n");
	scanf("%s", cmd);

	if(strcmp(cmd, "register") == 0)
        {
	    printf("注册帐号:\n");
            printf("--------------------------------\n");

	    Info->action = 0;

	    printf("输入你的ID:\n");
            scanf("%s", Info->ID);

            printf("输入你的用户名:\n");
	    scanf("%s", Info->username);

	    printf("输入你要设置的密码:\n");

            setbuf(stdin,NULL);

	    while((ch=my_getch())!='\r')
	    {
	        if(ch==127)
		{
		    if(n>0)
		    { 
		        n--;
			putchar('\b');
			putchar(' ');
			putchar('\b');
			ch='\0';
		    }
		}
		else
		{
		    password[n]=ch;
		    n++;
		    printf("*");
	        }
	    }
	    password[n]='\0';
	    printf("\n");
	    n=0;

	    strcpy(Info->password, password);

            if(write(sockfd, Info, sizeof(struct information)) == -1)
            {	    
		printf("write error!\n");
		exit(1);
            }

	}
        else if(strcmp(cmd, "quit") == 0)
        {
	    Info->action = 7;

	    if(write(sockfd, Info, sizeof(struct information)) == -1)
	    {
	        printf("write error!\n");
	        exit(1);
            }
        }
        else if(strcmp(cmd, "landing") ==0)
	{  
	    
            printf("登录帐号:\n");
	    printf("-------------------------------\n");

	    printf("please input ID:\n");
	    scanf("%s", Info->ID);

	    printf("please input username:\n");
	    scanf("%s", Info->username);

	    printf("please input password:\n");

            setbuf(stdin,NULL);

	    while((ch=my_getch())!='\r')
	    {
	        if(ch==127)
		{
		    if(n>0)
		    { 
		        n--;
			putchar('\b');
			putchar(' ');
			putchar('\b');
			ch='\0';
		    }
		}
		else
		{
		    password[n]=ch;
		    n++;
		    printf("*");
	        }
	    }
	    password[n]='\0';
	    printf("\n");
	    n=0;

            strcpy(Info->password,password);

	    Info->action = 1;

	    if(write(sockfd, Info, sizeof(struct information)) == -1)
            {
	         printf("write error!\n");
		 break;
	    }

	   // memset(Info,0,sizeof(struct information));

	    sleep(1);

	    if(flag==2)
	    {
	        system("reset");
	        printf("/***************管理员*************/\n");
		printf("/*********禁言***banned************/\n");
		printf("/**********解禁**lifted**************/\n");
		printf("/***********踢除***kicked***********/\n");
		printf("/***********退出***quit*************/\n");
                while(1)
		{

		printf("please input the cmd:\n");
		scanf("%s",cmd);

		if(strcmp(cmd,"banned")==0)
		{
		    Info->action=8;

		    printf("请输入你要禁言的名字:\n");
		    scanf("%s",Info->username);

		    if(write(sockfd,Info,sizeof(struct information))==-1)
		    {
		        printf("write error!\n");
			exit(1);
		    }
		    memset(Info,0,sizeof(struct information));
	        }
		else if(strcmp(cmd, "lifted") == 0)
		{
		    Info->action = 9;

		    printf("请输入你要解禁的名字:\n");
		    scanf("%s", Info->username);

		    if(write(sockfd, Info, sizeof(struct information)) == -1)
		    {
		        printf("write error!\n");
			exit(1);
		    }
		    memset(Info, 0, sizeof(struct information));
		}
		else if(strcmp(cmd, "kicked") == 0)
		{
                    memset(Info, 0, sizeof(struct information));

		    Info->action = 10;

		    printf("请输入你要踢出用户的ID:\n");
		    scanf("%s", Info->ID);

		    printf("请输入你要踢出用户的名字:\n");
		    scanf("%s", Info->username);

		    printf("请输入你要踢出用户的密码:\n");
//		    scanf("%s", Info->password);

                    setbuf(stdin,NULL);

	            while((ch=my_getch())!='\r')
	            {
	                if(ch==127)
		        {
		            if(n>0)
		            { 
		                n--;
			        putchar('\b');
			        putchar(' ');
			        putchar('\b');
			        ch='\0';
		            }
		        }
		        else
		        {
		            password[n]=ch;
		            n++;
		            printf("*");
	                }
	            }
	            password[n]='\0';
	            printf("\n");
	            n=0;

                    strcpy(Info->password, password);

		    if(write(sockfd, Info, sizeof(struct information)) == -1)
		    {
		        printf("write error!\n");
			exit(1);
		    }
		    memset(Info, 0, sizeof(struct information));
		}
		else if(strcmp(cmd, "quit") == 0)
		{
		    printf("**************注册***register**********\n");
		    printf("**************登录***landing***********\n");
		    printf("**************退出***quit**************\n");
		    printf("**************显示***display***********\n");
		    break;
		}
		else
		{
		    printf("cmd is error!\n");
		}
		}
	    }
	    if(flag == 6)
	    {
	        system("reset");
                printf("****************聊天室*****************\n");
		printf("**********聊天****chat***************\n");
		printf("**********群聊****groupchat***********\n");
		printf("**********下线****quit***************\n");
                
		while(1)
		{  
                printf("please input your cmd:\n");
		scanf("%s", cmd);

	        if(strcmp(cmd, "chat") == 0)
	        {
		    if(flag == 4)
		    {
		        printf("你已被禁言!\n");
			break;
		    }

                    printf("please input your username:\n");
	            scanf("%s",Info->username);

	            printf("please input toname:\n");
	            scanf("%s", Info->toname);

	            printf("please input message:\n");
	            scanf("%s", Info->msg);

	            Info->action = 2;

	            if(write(sockfd, Info, sizeof(struct information)) == -1)
                    {
	                printf("write error!");
		        exit(1);
	            }
	        }
	        else if(strcmp(cmd, "groupchat") == 0)
	        {
		    if(flag == 4)
		    {
		        printf("你已被禁言!\n");
			break;
		    }
                    printf("please input your name:\n");
	            scanf("%s", Info->username);

	            printf("please input message:\n");
	            scanf("%s", Info->msg);

	            Info->action = 3;

	            if(write(sockfd, Info, sizeof(struct information)) == -1)
	            {
	                printf("write error!\n");
		        exit(1);
	            }
	        }
		else if(strcmp(cmd, "quit") == 0)
		{
		    Info->action = 7;

		    if(write(sockfd, Info, sizeof(struct information)) == -1)
		    {
		        printf("write error!\n");
			exit(1);
		    }

		    exit(1);
		}
		else
		{
		    printf("cmd is error!\n");
		}
		}
            }
        }
	else if(strcmp(cmd,"display")==0)
	{
	    Info->action=6;

	    if(write(sockfd, Info,sizeof(struct information))==-1)
	    {
	        printf("write error!\n");
		exit(1);
	    }
	}
        else if(strcmp(cmd, "change") == 0)
	{
	    printf("修改密码:\n");
            printf("----------------------------------\n");
                
            printf("输入你的ID:\n");
            scanf("%s", Info->ID);

	 //   strcpy(Info->ID, ID);

	    printf("输入你的用户名:\n");
	    scanf("%s", Info->username);

	//    strcpy(Info->username, username);

	    printf("输入你的密码:\n");
	    
            setbuf(stdin,NULL);

	    while((ch=my_getch())!='\r')
	    {
	        if(ch==127)
		{
		    if(n>0)
		    { 
		        n--;
			putchar('\b');
			putchar(' ');
			putchar('\b');
			ch='\0';
		    }
		}
		else
		{
		    password[n]=ch;
		    n++;
		    printf("*");
	        }
	    }
	    password[n]='\0';
	    printf("\n");
	    n=0;

	    strcpy(Info->password, password);


         //   strcpy(Info->password, password);

	    Info->action = 4;

            if(write(sockfd, Info, sizeof(struct information)) < 0)
            {
	        perror("write error!\n");
		exit(1);
	    }

	    memset(Info, 0, sizeof(struct information));

	    if(flag == 3)
	    {
                printf("输入你要修改的密码:\n");
		scanf("%s", Info->password);
                
                Info->action = 5;

		if(write(sockfd, Info, sizeof(struct information)) < 0)
		{
		    perror("write error!\n");
		    exit(1);
		}
	    }
	}	
        else
	{
	        printf("cmd is error!\n");
	}
    }    


    /* 结束通讯 */ 
    close(sockfd); 

    return 0;
}    

