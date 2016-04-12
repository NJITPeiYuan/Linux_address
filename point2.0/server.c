#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#include <sqlite3.h>

#define portnumber 3333

#define MAX_SQL 1024

enum ALL
{
    ALREADY_YES = -10,
    ALREADY_NO = -9,
    SAY_OUT = -8,
    SAY_NO = -7,
    PHRASE_WRONG = -6,
    NO_PERSON = -5,
    USER_ONLINE = -4,
    PASSWD_WRONG = -3,
    NAME_WRONG = -2,
    NAME_EXIST = -1,

    REG = 0,
    LOG = 1,
    STO = 2,
    STA = 3,
    CHE = 4,
    MODIFY_INTR = 5,
    SEND_FILE = 6, 
    EXIT = 7,
    ROOT_NOS = 8,
    ROOT_SAY = 9,
    ROOT_OUT = 10,
    MODIFY_PASSWD = 11,
    STO_MSG = 13,
    STA_MSG = 14,
    REC_PHRASE = 15,
    REG_SUCCESS = 16,
    SAY_YES = 17,
    LOG_SUCCESS = 18,
    PASSWD_MODIFY_SUCCESS = 19,
    SEND_OVER = 20,
    REC_FILE = 21,
    REC_OVER = 22,
    ONLINE_FRIEND = 23,
    STO_SUCCESS = 24,
    STA_SUCCESS = 25,
};

struct message
{
    int action;
    char name[20];
    char passwd[8];
    char toname[20];
    char msg[1024];
};

struct online
{
    int fd;
    int say;
    char name[20];
    char intr[1024];
    struct online *next;
};

struct online * head;

pthread_mutex_t mutex;

void create_online()
{
    head = (struct online *)malloc(sizeof(struct online));
    head->next = NULL;
}

int search_user(struct message *msg)
{
    struct online *temp = head->next;

    while(temp != NULL)
    {
        if(strcmp(temp->name, msg->name) == 0)
	{
	    return -1;
	}

	temp = temp->next;
    }
    return 0;
}

int search_fd(struct message *msg)
{
    struct online *temp = head->next;

    while(temp != NULL)
    {
        if(strcmp(temp->name, msg->toname) == 0)
	{
	    return temp->fd;
	}

	temp = temp->next;
    }

    return -1;
}

int send_file(int w_fd, int r_fd, struct message *msg)
{
    while(read(r_fd, msg, sizeof(struct message)))
    {
        if(msg->action == SEND_OVER)
	{
	    break;
	}

	write(w_fd, msg, sizeof(struct message));
	
	memset(msg->msg, 0, sizeof(msg->msg));
    }

    msg->action = REC_OVER;

    write(w_fd, msg, sizeof(struct message));

    return 0;
}

int send_all(int new_fd, struct message *msg)
{
    struct online *temp = head->next;

    msg->action = STA_MSG;

    while(temp != NULL)
    {
        if(temp->fd != new_fd)
	{
	    write(temp->fd, msg, sizeof(struct message));
	}

	temp = temp->next;
    }

    msg->action = STA_SUCCESS;
    write(new_fd, msg, sizeof(struct message));

    return 0;
}

int insert_user(struct online *new_user)
{
    struct online *temp = head;

    while(temp->next != NULL)
    {
        temp = temp->next;
    }
    temp->next = new_user;
    new_user->next = NULL;

    return 0;
}

int delete_user(struct message *msg)
{
    struct online *temp = head->next;
    struct online *p = head;

    while(temp != NULL)
    {
        if(strcmp(msg->name, temp->name) == 0)
	{
	    p->next = temp->next;
	    free(temp);
	    temp = NULL;
	    return 0;
	}
	p = temp;
	temp = temp->next;
    }
    return -1;
}

int root_nos(struct message *msg)
{
    struct online *temp = head->next;

    while(temp != NULL)
    {
        if(strcmp(msg->toname, temp->name) == 0)
	{
	    if(temp->say == SAY_YES)
	    {
	        temp->say = SAY_NO;
		return temp->fd;
	    }
	    else
	    {
	        return ALREADY_NO;
	    }
	}
	temp = temp->next;
    }

    return -1;
}

int root_say(struct message *msg)
{
    struct online *temp = head->next;

    while(temp != NULL)
    {
        if(strcmp(msg->toname, temp->name) == 0)
	{
	    if(temp->say != SAY_YES)
	    {
	        temp->say = SAY_YES;
		return temp->fd;
	    }
	    else
	    {
	        return ALREADY_YES;
	    }
	}
	temp = temp->next;
    }

    return -1;
}

int root_out(struct message *msg)
{
    int fd;

    struct online *temp = head->next;
    struct online *p = head;

    while(temp != NULL)
    {
        if(strcmp(msg->toname, temp->name) == 0)
	{
	    fd = temp->fd;
	    p->next = temp->next;
	    free(temp);
	    temp = NULL;
	    return fd;
	}

	p = temp;
	temp = temp->next;
    }

    return -1;

}

int modify_intr(struct message *msg)
{
    int i;
    int nrow;
    int ncolumn;

    char *errmsg;
    char **result;

    char sql[MAX_SQL];

    sqlite3 *db;

    int ret = sqlite3_open("user.db", &db);

    if(ret != SQLITE_OK)
    {
        printf("open db error!\n");
	exit(1);
    }

    sqlite3_get_table(db, "select * from user", &result, &nrow, &ncolumn, &errmsg);

    for(i = 1; i <= nrow; i++)
    {
       if(strcmp(result[i * ncolumn + 1], msg->name) == 0)
       {
           sprintf(sql, "delete from user where name = '%s'", msg->name);
	   sqlite3_exec(db, sql, NULL, NULL, &errmsg);

	   memset(sql, 0, sizeof(char) * MAX_SQL);

           sprintf(sql, "insert into user(id, name, passwd, intr) values('%s', '%s', '%s', '%s')", result[i * ncolumn], result[i * ncolumn + 1], result[i * ncolumn + 2], msg->msg);
	   sqlite3_exec(db, sql, NULL, NULL, &errmsg);

	   break;
       }
    }

    sqlite3_close(db);

    return 0;    
}

int modify_passwd(struct message *msg)
{    
    int i;
    int nrow;
    int ncolumn;

    char *errmsg;
    char **result;

    char sql[MAX_SQL];

    sqlite3 *db;

    int ret = sqlite3_open("user.db", &db);

    if(ret != SQLITE_OK)
    {
        printf("open db error!\n");
	exit(1);
    }

    sqlite3_get_table(db, "select * from user", &result, &nrow, &ncolumn, &errmsg);

    for(i = 1; i <= nrow; i++)
    {
       if(strcmp(result[i * ncolumn + 1], msg->name) == 0)
       {
           sprintf(sql, "delete from user where name = '%s'", msg->name);
	   sqlite3_exec(db, sql, NULL, NULL, &errmsg);

	   memset(sql, 0, sizeof(char) * MAX_SQL);

           sprintf(sql, "insert into user(id, name, passwd, intr) values('%s', '%s', '%s', '%s')", result[i * ncolumn], result[i * ncolumn + 1], msg->passwd,  result[i * ncolumn + 3]);
	   sqlite3_exec(db, sql, NULL, NULL, &errmsg);

	   break;
       }
    }

    sqlite3_close(db);

}

int write_online(int k, struct message *msg)
{
    if(head == NULL)
    {
        return -1;
    }

    int count = 0;
    struct online *temp = head->next;
    struct online *ptr = head->next;
    struct online *s = (struct online *)malloc(sizeof(struct online));
    
    int nrow;
    int ncolumn;

    char **result;
    char *errmsg;

    sqlite3 *db;

    int ret = sqlite3_open("user.db", &db);

    if(ret != SQLITE_OK)
    {
        printf("open db error!\n");
	exit(1);
    }
    
    sqlite3_get_table(db, "select * from user", &result, &nrow, &ncolumn, &errmsg);

    while(temp != NULL)
    {
        count++;
	temp = temp->next;
    }

    memset(s, 0, sizeof(struct online));

    s->fd = count;
    msg->action = ONLINE_FRIEND;
    temp = head->next;

    while(temp != NULL)
    {
        write(temp->fd, msg, sizeof(struct message));

	s->fd = k;	
        write(temp->fd, s, sizeof(struct online));

	s->fd = count;	
        write(temp->fd, s, sizeof(struct online));

        s->fd = nrow;
        write(temp->fd, s, sizeof(struct online));

	if(k == 1)
	{
	    ptr = head->next;
	    
	    while(ptr != NULL)
	    {
	        usleep(10000);
		write(temp->fd, ptr, sizeof(struct online));
		ptr = ptr->next;
	    }

	    s->fd = -1;
	    write(temp->fd, s, sizeof(struct online));
	}
	else
	{
	    strcpy(s->name, msg->name);
	    write(temp->fd, s, sizeof(struct message));
	}

	temp = temp->next;
    }

    return 0;
}


void func_select(int new_fd, struct message *msg)
{
    int k;

    while(read(new_fd, msg, sizeof(struct message)))
    {
        printf("msg->action = %d\n", msg->action);

	switch(msg->action)
	{
	    case MODIFY_INTR:
	    {
	        modify_intr(msg);
		break;
	    }
	    case EXIT:
	    {
	       delete_user(msg);
	       write_online(-1, msg);
	       break;
	    }
	    case MODIFY_PASSWD:
	    {
	        msg->action = PASSWD_MODIFY_SUCCESS;
		delete_user(msg);
		modify_passwd(msg);
		write(new_fd, msg, sizeof(struct message));
		break;
	    }
	    case CHE:
	    {
	        write_online(1, msg);
		break;
	    }
	    case SEND_FILE:
	    {
	         
		 k = search_fd(msg);
		 if(k == -1)
		 {
		     msg->action = NO_PERSON;
		     write(new_fd, msg, sizeof(struct message));
		 } 
		 else
		 {
		     msg->action = REC_FILE;
		     write(k, msg, sizeof(struct message));
		     send_file(k, new_fd, msg);
		 }

		 break;
		 
	    }
	    case STO:
	    {
	        
		k = search_fd(msg);
		if(k == -1)
		{
		    msg->action = NO_PERSON;
		    write(new_fd, msg, sizeof(struct message));
		}
		else
		{
		    msg->action = STO_SUCCESS;
		    write(new_fd, msg, sizeof(struct message));
		    msg->action = STO_MSG;
		    write(k, msg, sizeof(struct message));
		}
		break;
		
	    }
	    case STA:
	    {
	        send_all(new_fd, msg);
		break;
	    }
	    case ROOT_NOS:
	    {
	    
	        k = root_nos(msg);
		if(k == ALREADY_NO)
		{
		    msg->action = ALREADY_NO;
		    write(new_fd, msg, sizeof(struct message));
		}
		else if(k != -1)
		{
		    msg->action = SAY_NO;
		    write(k, msg, sizeof(struct message));
		}
		else
		{
		    msg->action = NO_PERSON;
		    write(new_fd, msg, sizeof(struct message));
		}
		break;
        	
	    }
	    case ROOT_SAY:
	    {
	        
		k = root_say(msg);
		if(k == ALREADY_YES)
		{
		    msg->action = ALREADY_YES;
		    write(new_fd, msg, sizeof(struct message));
		}
		else if(k != -1)
		{
		    msg->action = SAY_YES;
		    write(k, msg, sizeof(struct message));
		}
		else
		{
		    msg->action = NO_PERSON;
		    write(new_fd, msg, sizeof(struct message));
		}
		break;
		
	    }
	    case ROOT_OUT:
	    {
	        
		k = root_out(msg);
		msg->action = SAY_OUT;
		if(k != -1)
		{
		    write(k, msg, sizeof(struct message));
		}
		break;
		
	    }
	}
        memset(msg, 0, sizeof(struct message));
    }
}

int reg(struct message *msg)
{
    int i;
    int n;
    int nrow;
    int ncolumn;
    int flag = 1;

    char **result;
    char *errmsg;
    char sql[MAX_SQL];
    char buf[MAX_SQL];

    sqlite3 *db;

    int ret = sqlite3_open("user.db", &db);

    if(ret != SQLITE_OK)
    {
        printf("open db error!\n");
	exit(1);
    }

    sqlite3_get_table(db, "select * from user", &result, &nrow, &ncolumn, &errmsg);

    for(i = 1; i <= nrow; i++)
    {
        if(strcmp(msg->name, result[i * ncolumn + 1]) == 0)
	{
	    break;
	}
    }
    if(i != nrow + 1)
    {
        sqlite3_close(db);
	return NAME_EXIST;
    }
    else
    {
        memset(sql, 0, sizeof(sql));
	while(flag == 1)
	{
	    n = rand() % 10000 + 10000;
	    sprintf(msg->msg, "%d", n);
	    for(i = 1; i <= nrow; i++)
	    {
	        if(strcmp(msg->msg, result[i * ncolumn]) == 0)
		{
		    break;
		}
	    }
	    if(i == nrow + 1)
	    {
	        flag = 0;
	    }
	}
	sprintf(buf, "这个人很懒,什么都没留下");
	sprintf(sql, "insert into user(id, name, passwd, intr) values('%s', '%s', '%s', '%s')", msg->msg, msg->name, msg->passwd, buf);
	sqlite3_exec(db, sql, NULL, NULL, &errmsg);

	sqlite3_close(db);

	return REG_SUCCESS;
    }
}

int log_in(struct message *msg)
{
    int i;
    int nrow;
    int ncolumn;

    char *errmsg;
    char **result;

    sqlite3 *db;

    int ret = sqlite3_open("user.db", &db);
    if(ret != SQLITE_OK)
    {
        printf("open db error!\n");
	exit(1);
    }

    sqlite3_get_table(db, "select * from user", &result, &nrow, &ncolumn, &errmsg);

    for(i = 1; i <= nrow; i++)
    {
        if(strcmp(msg->name, result[i * ncolumn]) == 0 || strcmp(msg->name, result[i * ncolumn + 1]) == 0)
	{
	    strcpy(msg->name, result[i * ncolumn + 1]);
	    strcpy(msg->msg, result[i * ncolumn + 3]);
	    break;
	}
    }
    
    if(i == nrow + 1)
    {
        sqlite3_close(db);
	return NAME_WRONG;
    }
    if(strcmp(msg->passwd, result[i * ncolumn + 2]) != 0)
    {
        sqlite3_close(db);
	return PASSWD_WRONG;
    }

    sqlite3_close(db);

    int k = search_user(msg);

    if(k == 0)
    {
        return LOG_SUCCESS;
    }
    else
    {
        return USER_ONLINE;
    }
}

int log_result(int new_fd, struct message *msg)
{
    int n_read;

    while(1)
    {
        n_read = read(new_fd, msg, sizeof(struct message));
	
	if(n_read == 0)
	{
	    printf("client is close!\n");
	    close(new_fd);
	    pthread_exit(NULL);
	}

	switch(msg->action)
	{
	    case REG:
	    {
	        msg->action = reg(msg);

		if(write(new_fd, msg, sizeof(struct message)) == -1)
		{
		    fprintf(stderr, "Write Error:%s\n", strerror(errno));
		    exit(1);
		}
		break;
	    }
	    case LOG:
	    {
	        msg->action = log_in(msg);

		if(write(new_fd, msg, sizeof(struct message)) == -1)
		{
		    fprintf(stderr, "Write Error:%s\n", strerror(errno));
		    exit(1);
		}
		break;
	    }
	}
	if(msg->action == LOG_SUCCESS)
	{
	    return 0;
	}
    }
    return 0;
}

void read_msg(void *arg)
{
    int new_fd = *((int *)arg);

    struct message *msg = (struct message *)malloc(sizeof(struct message));

    log_result(new_fd, msg);
    struct online *new_user = (struct online *)malloc(sizeof(struct online));

    new_user->fd = new_fd;
    new_user->say = SAY_YES;
    strcpy(new_user->name, msg->name);
    strcpy(new_user->intr, msg->msg);

    insert_user(new_user);

    func_select(new_fd, msg);
}

int main(int argc, char *argv[])
{
    int sockfd, new_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int sin_size;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Socket error:%s\n\a", strerror(errno));
	exit(1);
    }

    int opt = 1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	    
    bzero(&server_addr,sizeof(struct sockaddr_in)); 
    server_addr.sin_family=AF_INET;                 
    server_addr.sin_addr.s_addr=inet_addr("192.168.1.110");
    server_addr.sin_port=htons(portnumber);         	
			            
    if(bind(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1) 
    { 
	fprintf(stderr,"Bind error:%s\n\a",strerror(errno)); 
	exit(1); 
    } 

    if(listen(sockfd,5)==-1) 
    { 
        fprintf(stderr,"Listen error:%s\n\a",strerror(errno)); 
	exit(1); 
    } 

    pthread_t id;

    create_online();


    while(1)
    {
        sin_size = sizeof(struct sockaddr_in);
	if((new_fd = accept(sockfd, (struct sockaddr *)(&client_addr), &sin_size)) == -1)
	{
	    fprintf(stderr, "Accept error:%s\n\a", strerror(errno));
	    exit(1);
	}
	fprintf(stderr, "Server get connection from %s\n", inet_ntoa(client_addr.sin_addr));

	pthread_create(&id, NULL, (void *)read_msg, (void *)&new_fd);
    }

    close(sockfd);
    exit(0);
}
