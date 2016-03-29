#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 

#include <sqlite3.h>

#include <pthread.h>

#define portnumber 3333

int flag = 1;

struct information
{
    int action;
    char ID[20];
    char username[20];
    char password[7];
    char toname[20];
    char msg[1024];
};

struct online
{
    int fd;
    char name[20];

    struct online *next;
};
typedef struct online Online;
typedef struct online *Link;

Link head = NULL;

int creat_sqlite3(char *ID, char *username, char *password)
{  
    sqlite3 *db;
    char *errmsg;

    char **result;
    int i;
    int j;
    int nrow;
    int ncolumn;


    char sql[1024] = "create table password(ID txt primary key, username txt, password txt, flag integer)";

    int ret = sqlite3_open("password.db", &db);

    if(ret != SQLITE_OK)
    {
	printf("sqlite open error!\n");
	exit(1);
    }

    sqlite3_exec(db, sql, NULL, NULL, &errmsg);

    sqlite3_get_table(db,"select * from password",&result,&nrow,&ncolumn,&errmsg);
    for(i=1;i<=nrow;i++)
    {
        for(j=0;j<ncolumn;j++)
        {
	    if(strcmp(result[i*ncolumn+j], ID) == 0)
	    {
	        printf("注册失败!\n");
	        return 1;
	    }
	
	}
    }    
    memset(sql, 0, sizeof(sql));

    sprintf(sql, "insert into password(ID, username, password, flag) values('%s', '%s', '%s', %d )", ID, username, password, flag);

    sqlite3_exec(db, sql, NULL, NULL, &errmsg);

    printf("注册成功!\n");

    return 0;

}

int match_information(char *ID, char *username, char *password)
{   
    sqlite3 *db;
    char *errmsg;
    char sql[1024];

    int ret=sqlite3_open("password.db",&db);

    if(ret!=SQLITE_OK)
    {
        printf("open error!\n");
	exit(1);
    }

    char **result;
    int i;
    int j;
    int nrow;
    int ncolumn;

    sqlite3_get_table(db,"select * from password",&result,&nrow,&ncolumn,&errmsg);
    for(i=1;i<=nrow;i++)
    {
        for(j=0;j<ncolumn;j++)
	{
	    if(strcmp(result[i*ncolumn+j],ID)==0&&strcmp(result[i*ncolumn+j+1],username)==0&&strcmp(result[i*ncolumn+j+2],password)==0)
	    {
	        if(atoi(result[i*ncolumn+j+3]) == 1)
                {
	            printf("验证成功!\n");
		    if(strcmp(result[i*ncolumn+j+1],"admin")==0)
		    {
		        printf("admin online!\n");
		        return 2;
		    }
		    else
		    {
		        return 1;
		    }    
	        }
		else
		{
		    printf("用户已登录，请退出!\n");
		    return 3;
		}
            }
	}
    }
    printf("用户名不存在或密码错误!\n");
    return 0;
}

void change_password(char *ID, char *password)
{
    sqlite3 *db;
    char *errmsg;

    char sql[1024];

    int ret = sqlite3_open("password.db", &db);

    if(ret != SQLITE_OK)
    {
        printf("sqlite open error!\n");
	exit(1);
    }

    sprintf(sql, "update password set password = %s where ID = %s", password, ID);

    sqlite3_exec(db, sql, NULL, NULL, &errmsg);

    printf("密码重设成功!\n");

}

void change_flag_0(char *ID)
{
    sqlite3 *db;
    char *errmsg;

    char sql[1024];

    int ret = sqlite3_open("password.db", &db);

    if(ret != SQLITE_OK)
    {
        printf("sqlite open error!\n");
	exit(1);
    }

    printf("ID = %s\n", ID);

    sprintf(sql, "update password set flag = 0  where ID = '%s'", ID);

    sqlite3_exec(db, sql, NULL, NULL, &errmsg);

    printf("置flag为零成功!\n");
}

void change_flag_1(char *ID)
{
    sqlite3 *db;
    char *errmsg;

    char sql[1024];

    int ret = sqlite3_open("password.db", &db);

    if(ret != SQLITE_OK)
    {
        printf("sqlite open error!\n");
	exit(1);
    }

    printf("ID = %s\n", ID);

    sprintf(sql, "update password set flag = 1  where ID = '%s'", ID);

    sqlite3_exec(db, sql, NULL, NULL, &errmsg);

    printf("置flag为1成功!\n");
}    

void insert_user(Link *newnode, Link *head)
{
    (*newnode)->next = *head;
    *head = *newnode;
}

int find_fd(Link *head, char *toname)
{
    struct information *Info = (struct information *)malloc(sizeof(struct information));

    if(*head == NULL)
    {
        return -1;
    }

    Link temp = *head;

    while(temp != NULL)
    {
        if(strcmp(temp->name, toname) == 0)
	{
	    return temp->fd;
	}

	temp = temp->next;
    }

    return -1;
}

char *find_name(Link *head, int fd)
{
    Link temp = *head;

    while(temp != NULL)
    {
        if(temp->fd == fd)
	{
	    return temp->name;
	}

	temp = temp->next;
    }

  //  return -1;
}

int all_fd(Link *head,int dest[])
{
    struct information *Info = (struct information *)malloc(sizeof(struct information));

    Link temp=*head;
    int i=0;

    while(temp!=NULL)
    {
        dest[i]=temp->fd;
	i++;

	temp=temp->next;
    }
    return i;
}

int delete_link(Link *head, int to_fd)
{
    Link temp = *head;
    Link temp1 = temp;
    temp = temp->next;

    if(*head == NULL)
    {
        return -3;
    }
    else
    {
        if((*head)->fd == to_fd)
	{
	    *head = (*head)->next;

	    free(temp);

	    temp = NULL;

	    return 0;
	}
	else
	{
	    while(temp != NULL)
	    {
	        if((*head)->fd == to_fd)
		{
		    temp1->next = temp->next;

                    free(temp);

		    temp = NULL;

		    return 0;
		}
	    }
	}
    }
}

int delete_link_name(Link *head, char *username)
{
    if(*head==NULL)
    {
        return -1;
    }

    Link temp=*head;
    if(strcmp(temp->name,username)==0)
    {
	*head=(*head)->next;
	free(temp);
	temp=NULL;
	return 0;
    }

    Link ptr=temp;
    temp=temp->next;

    while(temp!=NULL)
    {
        if(strcmp(temp->name,username)==0)
        {
            ptr->next=temp->next;
            free(temp);
            temp=NULL;

	    return 0;
        }
	ptr=temp;
	temp=temp->next;
    }
    return -1;
}

void read_msg(void *arg)
{
    char buffer[1024];

    int new_fd = *((int *)arg);

    int r_read = 0;
    int flag;
    int flag1;

    int i;
    int dest[10];

    Link newnode;

    struct information *Info = (struct information *)malloc(sizeof(struct information));
	            
    while(1)
    {
      //      memset(Info, 0, sizeof(struct information));

	    r_read = read(new_fd, Info, sizeof(struct information));

	    if(r_read == 0)
	    {   
	        printf("%s 客户端下线!\n", Info->username);

	        char *temp = find_name(&head, new_fd);
 
                strcpy(Info->username, temp);

		change_flag_1(Info->ID);

               // delete_link(&head, new_fd);

		delete_link_name(&head,Info->username);
	
	        close(new_fd);
		pthread_exit(NULL);
		    
	    }
	    if(r_read == -1)
	    {
	        perror("read error!");
	    }
	    switch(Info->action)
	    {
	        case 0:
		{
		    flag = creat_sqlite3(Info->ID, Info->username, Info->password);

		    if(flag == 0)
		    {
		        Info->action=0;
			if(write(new_fd,Info,sizeof(struct information))==-1)
		        {
			    printf("write error!\n");
			    exit(1);
			}
			
		    }
		    if(flag == 1)
		    {
		        Info->action = -1;
			if(write(new_fd, Info, sizeof(struct information)) == -1)
			{
			    printf("write error!\n");
			    exit(1);
			}
		        
		    }

		    memset(Info,0,sizeof(struct information));

		    break;
		}
		
                case 1:
		{
                    flag=match_information(Info->ID,Info->username,Info->password);


		    if(flag == 1) 
		    {
                        change_flag_0(Info->ID);

		        newnode=(Link)malloc(sizeof(Online));
                        newnode->fd = new_fd;
		        strcpy(newnode->name, Info->username);

		        insert_user(&newnode, &head);
		    
		        Info->action = 1;

		        if(write(new_fd, Info, sizeof(struct information)) == -1)
			{
			    printf("write error!\n");
			    exit(1);
			}

			memset(Info,0,sizeof(struct information));
		    }
		    else if(flag == 3)
		    {
                        printf("用户已登录!\n");

		        Info->action = 11;

			if(write(new_fd, Info, sizeof(struct information)) == -1)
			{
			    printf("write error!\n");
			    exit(1);
			}
		    }
		    else if(flag == 0)
		    {
                        printf("帐号没注册!\n");

			Info->action = 14;

			if(write(new_fd, Info, sizeof(struct information)) == -1)
			{
		            printf("write error!\n");
			    exit(1);
			}
		    }
		    else if(flag == 2)
		    {
			Info->action=4;
			if(write(new_fd,Info,sizeof(struct information))==-1)
			{
			    printf("write error!\n");
															   
			    exit(1);
			}
		    }	
		    break;
		}
		
		case 2:
		{
                    int to_fd = find_fd(&head, Info->toname);

		    if(to_fd == -1)
		    {
		        Info->action = -2;

			if(write(new_fd, Info, sizeof(struct information)) == -1)
			{
			    printf("write erroe!\n");
			    exit(1);
			}
		    }
		    else
		    {
		        Info->action = 2;

			if(write(to_fd, Info, sizeof(struct information)) == -1)
			{
			    printf("write error!\n");
			    exit(1);
			}
		    }
		    break;
                }

		case 3:
		{
		    Info->action=3;
		    flag=all_fd(&head,dest);
		    for(i=0;i<flag;i++)
		    {
		        write(dest[i],Info,sizeof(struct information));
		    }
		    break;
		}

		case 4:
		{
		    flag = match_information(Info->ID, Info->username, Info->password);

		    if(flag == 1)
		    {
                        Info->action = 7;

			write(new_fd, Info, sizeof(struct information));
		    }
		    if(flag == 0)
		    {
		        write(new_fd, "0", strlen("0"));
			exit(1);
		    }
		    
		    break;		    
		}

		case 5:
		{
		    change_password(Info->ID, Info->password); 

		    Info->action = 8;

                    if(write(new_fd, Info, sizeof(struct information)) == -1)
		    {
		        printf("write error!\n");
			exit(1);
		    }

		    break;
		}

		case 6:
	        {
	            Info->action=6;
		    Link temp=head;

		  //  int to_fd = find_fd(&head, Info->username)

		    while(temp!=NULL)
		    {		        			
		        strcpy(Info->username,temp->name);
		        if(write(new_fd,Info,sizeof(struct information))==-1)
		        {
		            printf("write error!\n");
		        }
			    
		        temp=temp->next;
	            }
		    break;
	        }

		case 7:
		{
		
//		    change_flag_1(Info->ID);

  //                  printf("username = %s\n", Info->username);

//		    int to_fd = find_fd(&head, Info->username);

//		    flag = delete_link(&head, to_fd);
                    
  //                  if(flag == 0)
//		    {

		        Info->action = 5;

                        if(write(new_fd, Info, sizeof(struct information)) == -1)
		        {
		            printf("write error!\n");
			    exit(1);
		        }
  //                  }
//		    if(flag == -3)
//		    {
//		        printf("下线失败!\n");
//		    }
                
		    break;
		}

		case 8:
		{
                    int to_fd = find_fd(&head, Info->username);

		    if(to_fd == -1)
		    {
		        Info->action = -2;

			if(write(new_fd, Info, sizeof(struct information)) == -1)
			{
			    printf("write error!\n");
			    exit(1);
			}
		    }
		    else
		    {
		        Info->action = 12;

			if(write(to_fd, Info, sizeof(struct information)) == -1)
			{
			    printf("write error!\n");
			    exit(1);
			}
		    }
		    break;		   		    
		}

		case 9:
		{
		    int to_fd = find_fd(&head, Info->username);

		    if(to_fd == -1)
		    {
		        Info->action = -2;

			if(write(new_fd, Info, sizeof(struct information)) == -1)
			{
			    printf("write error!\n");
			    exit(1);
			}
		    }
		    else
		    {
		        Info->action = 13;

			if(write(to_fd, Info, sizeof(struct information)) == -1)
			{
			    printf("write error!\n");
			    exit(1);
			}
		    }
		    break;
		}

		case 10:
		{
                    printf("ID = %s\n", Info->ID);

		    change_flag_1(Info->ID);

                    int to_fd = find_fd(&head, Info->username);

		    flag=delete_link(&head, to_fd);

		    if(flag == 0)
		    {
		        Info->action=9;

			if(write(to_fd,Info,sizeof(struct information))==-1)
			{
			    printf("write error!\n");
			}
			memset(Info,0,sizeof(struct information));
		    }
		    if(flag == -3)
		    {
		        Info->action=10;

			if(write(new_fd,Info,sizeof(struct information))==-1)
			{
			    printf("write error!\n");
			}
			memset(Info,0,sizeof(struct information));
		    }
//		    change_flag_1(Info->ID);
		    break;
		}
		
	   } 
       } 
}


int main(int argc, char *argv[]) 
{ 
    int sockfd,new_fd; 
    struct sockaddr_in server_addr; 
    struct sockaddr_in client_addr; 
    int sin_size;
    int r_read;
    int flag;
    char create[]="create success!\n"; 
    char loading_s[] = "loading success!";
    char loading_e[] = "loading error!";

    sqlite3 *db;
    char *errmsg;

    pthread_t id;
						

/* 服务器端开始建立sockfd描述符 */ 
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) // AF_INET:IPV4;SOCK_STREAM:TCP
    { 
        fprintf(stderr,"Socket error:%s\n\a",strerror(errno)); 
        exit(1); 
    } 

/* 服务器端填充 sockaddr结构 */ 
    bzero(&server_addr,sizeof(struct sockaddr_in)); // 初始化,置0
    server_addr.sin_family=AF_INET;                 // Internet
//server_addr.sin_addr.s_addr=htonl(INADDR_ANY);  // (将本机器上的long数据转化为网络上的long数据)和任何主机通信  //INADDR_ANY 表示可以接收任意IP地址的数据，即绑定到所有的IP
    server_addr.sin_addr.s_addr=inet_addr("192.168.1.10");  //用于绑定到一个固定IP,inet_addr用于把数字加格式的ip转化为整形ip
    server_addr.sin_port=htons(portnumber); // (将本机器上的short数据转化为网络上的short数据)端口号

    int opt;

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

/* 捆绑sockfd描述符到IP地址 */ 
    if(bind(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1) 
    { 
        fprintf(stderr,"Bind error:%s\n\a",strerror(errno)); 
        exit(1); 
    } 

/* 设置允许连接的最大客户端数 */ 
    if(listen(sockfd,5)==-1) 
    { 
        fprintf(stderr,"Listen error:%s\n\a",strerror(errno)); 
        exit(1); 
    } 

    while(1) 
    { 
/* 服务器阻塞,直到客户程序建立连接 */ 
        sin_size=sizeof(struct sockaddr_in); 
        if((new_fd=accept(sockfd,(struct sockaddr *)(&client_addr),&sin_size))==-1) 
        { 
            fprintf(stderr,"Accept error:%s\n\a",strerror(errno)); 
	    exit(1); 
        } 
        fprintf(stderr,"Server get connection from %s\n",inet_ntoa(client_addr.sin_addr)); // 将网络地址转换成.字符串
/*
        if(write(new_fd,hello,strlen(hello))==-1) 
        { 
            fprintf(stderr,"Write Error:%s\n",strerror(errno)); 
	    exit(1); 
        }
*/
        pthread_create(&id, NULL, (void *)read_msg, (void *)&new_fd);
    } 

    close(sockfd); 
    exit(0); 
} 

