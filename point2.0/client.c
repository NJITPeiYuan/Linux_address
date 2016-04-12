#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <unistd.h>

#include <signal.h>

#include <pthread.h>

#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>

#include <termio.h>

#include <locale.h>

#include <curses.h>

#include <time.h>

#define portnumber 3333

#define PASSWD_LEN 8
#define MAX_SIZE 1024

#define ENTER 13
#define ESCAPE 27

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
    HISTORY_CHAT = 12,
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

struct online *head;
struct online *info;
struct message msg;

char user[20];
char intr[1024];
int flag_blink;
int friend_on;
int friend_all;
int flag_say;
int add_friend;

/*******************************************************
***    创建链表
***
*******************************************************/
void create_online()
{
    head = (struct online *)malloc(sizeof(struct online));
    info = (struct online *)malloc(sizeof(struct online));
    head->next = NULL;
}

/*****************************************************
***    surses库初始化
***
*****************************************************/
void init_curses()
{
    setlocale(LC_ALL, "");
    initscr();
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_BLUE, COLOR_WHITE);
    init_pair(3, COLOR_RED, COLOR_WHITE);
    init_pair(4, COLOR_GREEN, COLOR_BLUE);
    curs_set(0);
    noecho();
    nonl();
    keypad(stdscr, TRUE);
}

/****************************************************
***读行
***
****************************************************/
int read_line(int fd, char *buf, int max_len)
{
    int i;
    int ret;

    char temp;

    for(i = 0; i < max_len; i++)
    {
        ret = read(fd, &temp, 1);

	if(ret == -1)
	{
	    perror("read error!");
	    exit(1);
	}
	if(ret == 0)
	{
	    break;
	}

	if(temp == '\n')
	{
	    i++;
	    break;
	}
	buf[i] = temp;
    }
    buf[i] = '\0';

    return i;
}

/*****************************************************
***    注册窗口
***
*****************************************************/
int reg_result(int sockfd)
{
    int ch;
    int k = 0;
    char confirm[PASSWD_LEN];

    clear();
    WINDOW *alertWindow;

    init_curses();

    alertWindow = subwin(stdscr, 15, 60, 5, 15);
    box(alertWindow, ACS_VLINE, ACS_HLINE);

    mvwaddstr(alertWindow, 2, 10, "申请帐号 :");
    mvwaddstr(alertWindow, 4, 10, "设置密码 :");
    mvwaddstr(alertWindow, 6, 10, "确认密码 :");
    wrefresh(alertWindow);

    while((ch = getch()) != ENTER)
    {
        if(ch == ESCAPE)
	{
	    return 0;
	}
    }

    memset(&msg, 0, sizeof(struct message));
    msg.action = REG;
    echo();
    wmove(alertWindow, 2, 25);
    wgetstr(alertWindow, msg.name);
    wmove(alertWindow, 4, 25);
    wgetstr(alertWindow, msg.passwd);
    wmove(alertWindow, 6, 25);
    wgetstr(alertWindow, confirm);
    noecho();

    if(strcmp(msg.passwd, confirm) != 0)
    {
        mvwaddstr(alertWindow, 6, 40, "确认错误");
	mvwaddstr(alertWindow, 10, 10, "按回车重新注册!");
	mvwaddstr(alertWindow, 12, 10, "按HOME键返回登录界面!");
	wrefresh(alertWindow);
    }
    else
    {
        write(sockfd, &msg, sizeof(struct message));
    }

    while((ch = getch()))
    {
        if(ch == ENTER)
	{
	    k = -1;
	    break;
	}
	else if(ch == KEY_HOME)
	{
	    k = 0;
	    break;
	}
    }

    delwin(alertWindow);
    return k;
}

/******************************************************
***    登录窗口
***
******************************************************/
int log_select(int sockfd)
{
    int ch;
    int count = 5;
    int selected = 0;

    WINDOW *alertWindow;
    WINDOW **items;
    items = (WINDOW **)malloc(5 * sizeof(WINDOW *));

    clear();
    init_curses();
    bkgd(COLOR_PAIR(2));

    alertWindow = subwin(stdscr, 10, 40, 10, 20);
    box(alertWindow, ACS_VLINE, ACS_HLINE);
    wrefresh(alertWindow);

    items[0] = subwin(stdscr, 1, 4, 12, 22);
    waddstr(items[0], "帐号");
    items[1] = subwin(stdscr, 1, 4, 14, 22);
    waddstr(items[1], "密码");
    items[2] = subwin(stdscr, 1, 4, 16, 30);
    waddstr(items[2], "注册");
    items[3] = subwin(stdscr, 1, 4, 16, 40);
    waddstr(items[3], "登录");
    items[4] = subwin(stdscr, 1, 1, 11, 57);
    waddstr(items[4], "X");
    wrefresh(items[0]);
    wrefresh(items[1]);
    wrefresh(items[2]);
    wrefresh(items[3]);
    wrefresh(items[4]);

    while((ch = getch()) != ENTER)
    {
        if(ch == KEY_DOWN || ch == KEY_UP)
	{
	    wbkgd(items[selected], COLOR_PAIR(2));
	    wnoutrefresh(items[selected]);
	    if(ch == KEY_DOWN)
	    {
	        selected = (selected + 1) % count;
	    }
	    else
	    {
	        selected = (selected + count -1) % count;
	    }
	    wbkgd(items[selected], COLOR_PAIR(3));
	    wnoutrefresh(items[selected]);
	    refresh();
	}
    }

    delwin(alertWindow);
    if(selected == 0)
    {
        msg.action = LOG;
	echo();
	move(12, 30);
	getstr(msg.name);
	move(14, 30);
	noecho();
	getstr(msg.passwd);
	write(sockfd, &msg, sizeof(struct message));
	usleep(10000);
	if(msg.action != LOG_SUCCESS)
	{
	    getch();
	}
    }
    else if(selected == 2)
    {
        int k = reg_result(sockfd);
	while(k == -1)
	{
	    k = reg_result(sockfd);
	}
    }
    else if(selected == 4)
    {
        return EXIT;
    }

    return 0;
}

/*****************************************************
***    用malloc创建的窗口,用完释放
***
*****************************************************/
void delete_menu(WINDOW **items, int count)
{
    int i;

    for(i = 0; i < count; i++)
    {
        delwin(items[i]);
    }
    free(items);
}

/****************************************************
***    版本介绍
***
****************************************************/
void version_menu()
{
    WINDOW *alertWindow;

    int ch;

    init_curses();

    alertWindow = subwin(stdscr, 12, 50, 5, 15);
    box(alertWindow, ACS_VLINE, ACS_HLINE);

    mvwaddstr(alertWindow, 2, 2, "软件介绍");
    mvwaddstr(alertWindow, 4, 8, "聊天室12.4.8");
    mvwaddstr(alertWindow, 5, 10, "支持即时通信，curses图形化界面");
    mvwaddstr(alertWindow, 7, 8, "designed by:裴园");
    mvwaddstr(alertWindow, 9, 40, "ESC返回");

    wrefresh(alertWindow);

    while((ch = getch()) != ESCAPE);
}

/****************************************************
***    修改密码
***
*****************************************************/
int modify_passwd(int sockfd)
{
    int ch;
    char confirm[10];

    WINDOW *alertWindow;

    init_curses();
    alertWindow = subwin(stdscr, 15, 60, 5, 15);
    box(alertWindow, ACS_VLINE, ACS_HLINE);
    mvwaddstr(alertWindow, 2, 10, "新密码 :");
    mvwaddstr(alertWindow, 6, 10, "确认密码 :");
    wmove(alertWindow, 2, 25);
    wrefresh(alertWindow);

    while((ch = getch()) != ENTER)
    {
        if(ch == ESCAPE)
	{
	    return 0;
	}
    }

    echo();
    memset(&msg, 0, sizeof(struct message));
    msg.action = MODIFY_PASSWD;
    strcpy(msg.name, user);
    wgetstr(alertWindow, msg.passwd);
    wmove(alertWindow, 6, 25);
    wgetstr(alertWindow, confirm);
    noecho();

    if(strcmp(msg.passwd, confirm) != 0)
    {
        mvwaddstr(alertWindow, 6, 40, "密码确认错误");
	mvwaddstr(alertWindow, 10, 10, "更改密码失败!");
	mvwaddstr(alertWindow, 12, 10, "ESC键退出修改!");
	refresh();
    }
    else
    {
        write(sockfd, &msg, sizeof(struct message));
    }

    usleep(10000);

    while((ch = getch()) != ESCAPE);

    delwin(alertWindow);

    return 0;
}

/*****************************************************
***收到消息开始闪烁
***
*****************************************************/
void blink_menu()
{
    int fd;
    int obj = 0;
    int i = 0;
    int y = 10;
    char name[20];
    char buf[100];
    char file[100];
    char mes[MAX_SIZE];

    struct online *temp = head->next;

    WINDOW *alertWindow;
    WINDOW *clearWindow;
    WINDOW **my_friend;

    my_friend = (WINDOW **)malloc(friend_on * sizeof(WINDOW *));

    init_curses();

    clearWindow = subwin(stdscr, friend_on * 2, 15, 10, 82);
    alertWindow = subwin(stdscr, 1, 15, 9, 82);
    sprintf(buf, ">我的好友(%d/%d)", friend_on, friend_all);
    waddstr(alertWindow, buf);

    strcpy(name, msg.name);

    sprintf(file, "./mychat/%s.txt", msg.name);
    fd = open(file, O_CREAT | O_RDWR | O_APPEND, 0655);

    time_t timep;
    time(&timep);
    memset(mes, 0, sizeof(mes));
    sprintf(mes, "%s", ctime(&timep));
    write(fd, mes, strlen(mes));

    memset(mes, 0, sizeof(mes));
    sprintf(mes, "%s:%s", msg.name, msg.msg);
    write(fd, mes, strlen(mes));
    write(fd, "\n", 1);
    write(fd, "\n", 1);

loop:

    while(flag_blink != 1)
    {
        wclear(alertWindow);
	wrefresh(alertWindow);
	usleep(500000);
	sprintf(buf, ">我的好友(%d/%d)", friend_on, friend_all);
	waddstr(alertWindow, buf);
	wrefresh(alertWindow);
	usleep(500000);
    }
    while(temp != NULL)
    {
        my_friend[i] = subwin(stdscr, 1, 15, y, 82);
	wprintw(my_friend[i], "%s", temp->name);
	wrefresh(my_friend[i]);
	if(strcmp(name, temp->name) == 0)
	{
	    obj = i;
	}
	i++;
	y = y + 2;
	temp = temp->next;
    }
    while(strcmp(name, info->name) != 0)
    {
        if(flag_blink != 1)
	{
	    wclear(clearWindow);
	    wrefresh(clearWindow);
	    goto loop;
	}
	wclear(my_friend[obj]);
	wrefresh(my_friend[obj]);
	usleep(500000);
	wprintw(my_friend[obj], "%s", name);
	wrefresh(my_friend[obj]);
	usleep(500000);
    }

    delwin(alertWindow);
    delete_menu(my_friend, friend_on);
}

/*****************************************************
***    群聊闪烁
***
******************************************************/
void sta_blink()
{
    int fd;
    char buf[100];
    char file[100];
    char mes[MAX_SIZE];

    WINDOW *alertWindow;

    init_curses();
    alertWindow = subwin(stdscr, 1, 15, 9, 82);
    wclear(alertWindow);
    sprintf(buf, "苏嵌108班");
    waddstr(alertWindow, buf);

    wrefresh(alertWindow);

    sprintf(file, "./mychat/sta_chat.txt");
    if((fd = open(file, O_CREAT | O_RDWR | O_APPEND, 0777)) < 0)
    {
        perror("open file error!");
	exit(1);
    }

    time_t timep;
    time(&timep);
    memset(mes, 0, sizeof(mes));
    sprintf(mes, "%s", ctime(&timep));
    write(fd, mes, strlen(mes));

    memset(mes, 0, sizeof(mes));
    sprintf(mes, "%s:%s", msg.name, msg.msg);
    write(fd, mes, strlen(mes));
    write(fd, "\n", 1);
    write(fd, "\n", 1);

    while(flag_blink != 2)
    {
        wclear(alertWindow);
	wrefresh(alertWindow);
	usleep(500000);
	sprintf(buf, "苏嵌108班");
	waddstr(alertWindow, buf);
	wrefresh(alertWindow);
	usleep(500000);
    }

    delwin(alertWindow);
}

/***************************************************
***    私聊聊天记录
***
***************************************************/
void history_chat()
{
    
    int ch = -1;
    int fd;
    char file[100];
    char buf[MAX_SIZE];

    WINDOW *hisWindow;
    WINDOW **func_menu;

    hisWindow = subwin(stdscr, 12, 36, 7, 18);
    wclear(hisWindow);
    scrollok(hisWindow, TRUE);

    func_menu = (WINDOW **)malloc(7 * sizeof(WINDOW *));
    init_curses();

    func_menu[0] = subwin(stdscr, 4, 60, 2, 15);
    mvwprintw(func_menu[0], 1, 6, "%s", info->name);
    mvwprintw(func_menu[0], 2, 8, "(%s)", info->intr);
    box(func_menu[0], ACS_VLINE, ACS_HLINE);

    func_menu[1] = subwin(stdscr, 15, 41, 5, 15);
    box(func_menu[1], ACS_VLINE, ACS_HLINE);

    func_menu[2] = subwin(stdscr, 15, 41, 5, 15);
    box(func_menu[2], ACS_VLINE, ACS_HLINE);

    func_menu[3] = subwin(stdscr, 3, 21, 19, 15);
    mvwaddstr(func_menu[3], 1, 5, "表情");
    box(func_menu[3], ACS_VLINE, ACS_HLINE);

    func_menu[4] = subwin(stdscr, 3, 21, 19, 35);
    mvwaddstr(func_menu[4], 1, 5, "文件");
    box(func_menu[4], ACS_VLINE, ACS_HLINE);


    func_menu[5] = subwin(stdscr, 3, 20, 19, 55);
    mvwaddstr(func_menu[5], 1, 5, "聊天记录");
    box(func_menu[5], ACS_VLINE, ACS_HLINE);

    
    func_menu[6] = subwin(stdscr, 5, 60, 21, 15);
    mvwaddstr(func_menu[6], 1, 5, "开始聊天");
    box(func_menu[6], ACS_VLINE, ACS_HLINE);

    wrefresh(func_menu[0]);
    wrefresh(func_menu[1]);
    wrefresh(func_menu[2]);
    wrefresh(func_menu[3]);
    wrefresh(func_menu[4]);
    wrefresh(func_menu[5]);
    wrefresh(func_menu[6]);
 
    sprintf(file, "./mychat/%s.txt", info->name);
    if((fd = open(file, O_CREAT | O_RDWR | O_APPEND, 0777)) < 0)
    {
        perror("open file failure!\n");
	exit(1);
    }

    lseek(fd, 0, SEEK_SET);
    memset(buf, 0, sizeof(buf));
    while(read_line(fd, buf, MAX_SIZE))
    {
        if((ch = getch()) == ESCAPE)
	{
	    break;
	}
        wprintw(hisWindow, "%s\n", buf);
	memset(buf, 0, sizeof(buf));
	wrefresh(hisWindow);
    }

    if(ch != ESCAPE)
    {
        memset(buf, 0, sizeof(buf));
	sprintf(buf, "聊天记录已读到文件末尾，ESC返回");
	wprintw(hisWindow, "%s\n", buf);
	wrefresh(hisWindow);
    }

    while((ch = getch()) != ESCAPE);

    wclear(hisWindow);
    delwin(hisWindow);
    delete_menu(func_menu, 7);
}


/****************************************************
***    群聊聊天记录
***
****************************************************/
void his_sta()
{
    int ch = -1;
    int fd;
    char file[100];
    char buf[MAX_SIZE];

    WINDOW *alertWindow;
    WINDOW *blankWindow;

    WINDOW **func_menu;

    func_menu = (WINDOW **)malloc(5 * sizeof(WINDOW *));
    init_curses();

    alertWindow = subwin(stdscr, 5, 60, 1, 15);
    mvwaddstr(alertWindow, 1, 4, "苏嵌108群聊");
    mvwaddstr(alertWindow, 2, 8, "本群管理员peiyuan");
    box(alertWindow, ACS_VLINE, ACS_HLINE);

    blankWindow = subwin(stdscr, 15, 60, 5, 15);
    box(blankWindow, ACS_VLINE, ACS_HLINE);

    func_menu[0] = subwin(stdscr, 5, 60, 21, 15);
    mvwaddstr(func_menu[0], 1, 5, "开始聊天");
    box(func_menu[0], ACS_VLINE, ACS_HLINE);

    func_menu[1] = subwin(stdscr, 3, 21, 19, 15);
    mvwaddstr(func_menu[1], 1, 5, "表情");
    box(func_menu[1], ACS_VLINE, ACS_HLINE);

    func_menu[2] = subwin(stdscr, 3, 21, 19, 35);
    mvwaddstr(func_menu[2], 1, 5, " 聊天记录");
    box(func_menu[2], ACS_VLINE, ACS_HLINE);

    func_menu[3] = subwin(stdscr, 3, 20, 19, 55);
    mvwaddstr(func_menu[3], 1, 5, "管理员操作");
    box(func_menu[3], ACS_VLINE, ACS_HLINE);

    func_menu[4] = subwin(stdscr, 12, 55, 7, 18);
    scrollok(func_menu[4], TRUE);
    wclear(func_menu[4]);

    wrefresh(alertWindow);
    wrefresh(blankWindow);
    wrefresh(func_menu[0]);
    wrefresh(func_menu[1]);
    wrefresh(func_menu[2]);
    wrefresh(func_menu[3]);
    wrefresh(func_menu[4]);

    sprintf(file, "./mychat/sta_chat.txt");
    if((fd = open(file, O_CREAT | O_RDWR | O_APPEND, 0777)) < 0)
    {
        perror("open file failure!\n");
	exit(1);
    }

    lseek(fd, 0, SEEK_SET);

    memset(buf, 0, sizeof(buf));

    while(read_line(fd, buf, MAX_SIZE))
    {
        if((ch = getch()) == ESCAPE)
	{
	    break;
	}

        wprintw(func_menu[4], "%s\n", buf);
	memset(buf, 0, sizeof(buf));
	wrefresh(func_menu[4]);
    }
    if(ch != ESCAPE)
    {
        memset(buf, 0, sizeof(buf));
	sprintf(buf, "聊天记录已经读到文件末尾， ESC返回");
	wprintw(func_menu[4], "%s\n", buf);
	wrefresh(func_menu[4]);
    }

    while((ch = getch()) != ESCAPE);

    wclear(func_menu[4]);
    delwin(alertWindow);
    delwin(blankWindow);

    delete_menu(func_menu, 5);
}

/*****************************************************
***    管理员操作后普通用户的状态
***
*****************************************************/
void say_menu()
{
    char buf[100];

    WINDOW *alertWindow;
    WINDOW *sayWindow;

    init_curses();
    alertWindow = subwin(stdscr, 1, 15, 9, 82);
    wclear(alertWindow);
    sprintf(buf, "苏嵌108班");
    waddstr(alertWindow, buf);

    wrefresh(alertWindow);

    while(flag_blink != 2)
    {
        wclear(alertWindow);
	wrefresh(alertWindow);
	usleep(500000);
	sprintf(buf, "苏嵌108班");
	waddstr(alertWindow, buf);
	wrefresh(alertWindow);
	usleep(500000);
    }

    sayWindow = subwin(stdscr, 3, 40, 22, 20);

    if(flag_say == SAY_NO)
    {
        waddstr(sayWindow, "已被禁言");
    }
    else if(flag_say == SAY_YES)
    {
        waddstr(sayWindow, "禁言解除");
    }
    else if(flag_say == SAY_OUT)
    {
        waddstr(sayWindow, "已被剔出");
    }

    wrefresh(sayWindow);

    delwin(sayWindow);
    delwin(alertWindow);
}

/*****************************************************
***    管理员操作
***
*****************************************************/
void root_menu(int sockfd)
{
    int ch;
    int count = 2;
    int selected = 0;

    WINDOW **root_menu;
    WINDOW *alertWindow;

    alertWindow = subwin(stdscr, 2, 55, 22, 18);
    root_menu = (WINDOW **)malloc(2 * sizeof(WINDOW *));

    root_menu[0] = subwin(stdscr, 1, 6, 22, 18);
    wclear(root_menu[0]);
    waddstr(root_menu[0], "禁言");
    root_menu[1] = subwin(stdscr, 1, 6, 22, 24);
    waddstr(root_menu[1], "解禁");

    wrefresh(root_menu[0]);
    wrefresh(root_menu[1]);

    while((ch = getch()) != ENTER)
    {
        wbkgd(root_menu[selected], COLOR_PAIR(2));
	wnoutrefresh(root_menu[selected]);

	if(ch == KEY_LEFT || ch == KEY_RIGHT)
	{
	    if(ch == KEY_RIGHT)
	    {
	        selected = (selected + 1) % count;
	    }
	    else if(ch == KEY_LEFT)
	    {
	        selected = (selected + count - 1) % count;
	    }
	}
	wbkgd(root_menu[selected], COLOR_PAIR(3));
	wnoutrefresh(root_menu[selected]);
	refresh();
    }

    if(selected == 0)
    {
        msg.action = ROOT_NOS;
    }
    else
    {
        msg.action = ROOT_SAY;
    }

    mvwaddstr(alertWindow, 0, 0, "输入你要操作的帐号");
    wrefresh(alertWindow);
    echo();
    mvwgetstr(alertWindow, 1, 0, msg.toname);
    noecho();
    strcpy(msg.name, user);
    write(sockfd, &msg, sizeof(struct message));

    wclear(root_menu[0]);
    wclear(root_menu[1]);
    wclear(alertWindow);
    delwin(alertWindow);
    delete_menu(root_menu, 2);
}

/******************************************************
***     聊天窗口
***
******************************************************/
void chat_menu(int sockfd)
{
    WINDOW *alertWindow;

    echo();

    alertWindow = subwin(stdscr, 3, 55, 22, 18);

    wgetstr(alertWindow, msg.msg);

    wrefresh(alertWindow);

    noecho();

    strcpy(msg.name, user);
    write(sockfd, &msg, sizeof(struct message));

    wclear(alertWindow);
    delwin(alertWindow);
}

/******************************************************
***    表情窗口
***
*******************************************************/
void mood_menu(int sockfd)
{
    int ch;
    int count = 2;
    int selected = 0;

    WINDOW **mood_menu;

    mood_menu = (WINDOW **)malloc(2 * sizeof(WINDOW *));

    mood_menu[0] = subwin(stdscr, 1, 6, 22, 18);
    waddstr(mood_menu[0], "(^-^)");
    mood_menu[1] = subwin(stdscr, 1, 6, 22, 24);
    waddstr(mood_menu[1], "(o_o)");

    wrefresh(mood_menu[0]);
    wrefresh(mood_menu[1]);

    while((ch = getch()) != ENTER)
    {
        wbkgd(mood_menu[selected], COLOR_PAIR(2));
	wnoutrefresh(mood_menu[selected]);
	if(ch == KEY_LEFT || ch == KEY_RIGHT)
	{
	    if(ch == KEY_RIGHT)
	    {
	        selected = (selected + 1) % count;
	    }
	    else if(ch == KEY_LEFT)
	    {
	        selected = (selected + count - 1) % count;
	    }
	}
	wbkgd(mood_menu[selected], COLOR_PAIR(3));
	wnoutrefresh(mood_menu[selected]);
	refresh();
    }

    if(selected == 0)
    {
        strcpy(msg.msg, "(^-^)");
    }
    else
    {
        strcpy(msg.msg, "(o_o)");
    }

    strcpy(msg.name, user);
    write(sockfd, &msg, sizeof(struct message));

    wclear(mood_menu[0]);
    wclear(mood_menu[1]);
    delete_menu(mood_menu, 2);
}

/******************************************************
***    文件窗口
***
*******************************************************/
void file_menu(int sockfd)
{
    int fd;
    char file_name[20];
    WINDOW *alertWindow;
    alertWindow = subwin(stdscr, 3, 40, 22, 18);

    mvwaddstr(alertWindow, 0, 0, "请输入你要发送的文件");
    wrefresh(alertWindow);

    echo();
    mvwgetstr(alertWindow, 1, 0, file_name);
    noecho();
    wrefresh(alertWindow);

    fd = open(file_name, O_RDWR | O_CREAT, 0655);

    msg.action = SEND_FILE;
    strcpy(msg.name, user);
    strcpy(msg.toname, info->name);
    strcpy(msg.msg, file_name);
    write(sockfd, &msg, sizeof(struct message));

    memset(msg.msg, 0, sizeof(char) * MAX_SIZE);

    while(read_line(fd, msg.msg, MAX_SIZE))
    {
        usleep(10000);
	write(sockfd, &msg, sizeof(struct message));
	memset(&msg, 0, sizeof(struct message));

	usleep(10000);
	strcpy(msg.msg, "\n");
	write(sockfd, &msg, sizeof(struct message));
	memset(&msg, 0, sizeof(struct message));
    }

    msg.action = SEND_OVER;
    write(sockfd, &msg, sizeof(struct message));

    mvwaddstr(alertWindow, 2, 0, "发送完毕");
    wrefresh(alertWindow);

    delwin(alertWindow);
}


/*******************************************************
***    私聊窗口
***
*******************************************************/
void dialog_menu(int sockfd)
{
    int ch;
    int fd;
    int count = 4;
    int selected = 0;

    char file[100];
    char buf[MAX_SIZE];

    WINDOW *alertWindow;
    WINDOW *blankWindow;
    WINDOW *adverWindow;
    WINDOW **func_menu;

    func_menu = (WINDOW **)malloc(5 * sizeof(WINDOW *));
    init_curses();

    alertWindow = subwin(stdscr, 4, 60, 2, 15);
    mvwprintw(alertWindow, 1, 6, "%s", info->name);
    mvwprintw(alertWindow, 2, 8, "(%s)", info->intr);
    box(alertWindow, ACS_VLINE, ACS_HLINE);

    blankWindow = subwin(stdscr, 15, 41, 5, 15);
    box(blankWindow, ACS_VLINE, ACS_HLINE);

    adverWindow = subwin(stdscr, 15, 20, 5, 55);
    box(adverWindow, ACS_VLINE, ACS_HLINE);

    func_menu[0] = subwin(stdscr, 5, 60, 21, 15);
    mvwaddstr(func_menu[0], 1, 5, "开始聊天");
    box(func_menu[0], ACS_VLINE, ACS_HLINE);

    func_menu[1] = subwin(stdscr, 3, 21, 19, 15);
    mvwaddstr(func_menu[1], 1, 5, "表情");
    box(func_menu[1], ACS_VLINE, ACS_HLINE);


    func_menu[2] = subwin(stdscr, 3, 21, 19, 35);
    mvwaddstr(func_menu[2], 1, 5, "文件");
    box(func_menu[2], ACS_VLINE, ACS_HLINE);

    
    func_menu[3] = subwin(stdscr, 3, 20, 19, 55);
    mvwaddstr(func_menu[3], 1, 5, "聊天记录");
    box(func_menu[3], ACS_VLINE, ACS_HLINE);

    wrefresh(alertWindow);
    wrefresh(blankWindow);
    wrefresh(func_menu[0]);
    wrefresh(func_menu[1]);
    wrefresh(func_menu[2]);
    wrefresh(func_menu[3]);

    func_menu[4] = subwin(stdscr, 12, 35, 7, 18);
    scrollok(func_menu[4], TRUE);

 
    sprintf(file, "./mychat/%s.txt", info->name);
    fd = open(file, O_CREAT | O_RDWR | O_APPEND, 0777);
    memset(buf, 0, sizeof(buf));
    while(read_line(fd, buf, MAX_SIZE));
    {
        wprintw(func_menu[4], "%s\n", buf);
	memset(buf, 0, sizeof(buf));
    }
    wrefresh(func_menu[4]);

    
    while((ch = getch()) != ESCAPE)
    {
        while((ch = getch()) != ENTER)
	{
	    if(ch == ESCAPE)
	    {
	        break;
	    }
	    if(add_friend == -1)
	    {
	        break;
	    }
	    wbkgd(func_menu[selected], COLOR_PAIR(2));
	    wnoutrefresh(func_menu[selected]);
	    if(ch == KEY_DOWN || ch == KEY_UP)
	    {
	        if(ch == KEY_DOWN)
		{
		    selected = (selected + 1) % count;
		}
		else if(ch == KEY_UP)
		{
		    selected = (selected + count - 1) % count;
		}
	    }
	    wbkgd(func_menu[selected], COLOR_PAIR(3));
	    wnoutrefresh(func_menu[selected]);
	    refresh();
	}

	if(ch == ESCAPE || add_friend == -1)
	{
	    break;
	}

	wclear(func_menu[0]);
	func_menu[0] = subwin(stdscr, 5, 60, 21, 15);
	box(func_menu[0], ACS_VLINE, ACS_HLINE);
	if(selected == 0)
	{
	    msg.action = STO;
	    strcpy(msg.toname, info->name);
	    chat_menu(sockfd);
	}
	else if(selected == 1)
	{
	    msg.action = STO;
	    strcpy(msg.toname, info->name);
	    mood_menu(sockfd);
	}
	else if(selected == 2)
	{
	    file_menu(sockfd);
	}
	else if(selected == 3)
	{
	    history_chat();
	}
    }

    delwin(alertWindow);
    delwin(blankWindow);
    delwin(adverWindow);
    delete_menu(func_menu, 5);
}

/*****************************************************
***    群聊窗口
***
*****************************************************/
void sta_menu(int sockfd)
{
    int ch;
    int fd;
    int count = 4;
    int selected = 0;

    char file[100];
    char buf[MAX_SIZE];

    WINDOW *alertWindow;
    WINDOW *blankWindow;
    WINDOW *flagWindow;
    WINDOW **func_menu;

    func_menu = (WINDOW **)malloc(5 * sizeof(WINDOW *));
    init_curses();

    flagWindow = subwin(stdscr, 3, 40, 22, 20);
    wrefresh(flagWindow);

    alertWindow = subwin(stdscr, 5, 60, 1, 15);
    mvwaddstr(alertWindow, 1, 4, "苏嵌108群聊");
    mvwaddstr(alertWindow, 2, 8, "本群管理员peiyuan");
    box(alertWindow, ACS_VLINE, ACS_HLINE);
    wrefresh(alertWindow);

    blankWindow = subwin(stdscr, 15, 60, 5, 15);
    box(blankWindow, ACS_VLINE, ACS_HLINE);
    wrefresh(blankWindow);

    func_menu[0] = subwin(stdscr, 5, 60, 21, 15);
    mvwaddstr(func_menu[0], 1, 5, "开始聊天");
    box(func_menu[0], ACS_VLINE, ACS_HLINE);

    func_menu[1] = subwin(stdscr, 3, 21, 19, 15);
    mvwaddstr(func_menu[1], 1, 5, "表情");
    box(func_menu[1], ACS_VLINE, ACS_HLINE);

    func_menu[2] = subwin(stdscr, 3, 21, 19, 35);
    mvwaddstr(func_menu[2], 1, 5, " 聊天记录");
    box(func_menu[2], ACS_VLINE, ACS_HLINE);

    func_menu[3] = subwin(stdscr, 3, 20, 19, 55);
    mvwaddstr(func_menu[3], 1, 5, "管理员操作");
    box(func_menu[3], ACS_VLINE, ACS_HLINE);

    func_menu[4] = subwin(stdscr, 12, 55, 7, 18);
    scrollok(func_menu[4], TRUE);

    wrefresh(func_menu[0]);
    wrefresh(func_menu[1]);
    wrefresh(func_menu[2]);
    wrefresh(func_menu[3]);
    wrefresh(func_menu[4]);

    sprintf(file, "./mychat/sta_chat.txt");
    if((fd = open(file, O_CREAT | O_RDWR | O_APPEND, 0777)) < 0)
    {
        perror("open file failure!\n");
	exit(1);
    }

    memset(buf, 0, sizeof(buf));

    while(read_line(fd, buf, MAX_SIZE))
    {
        wprintw(func_menu[4], "%s\n", buf);
	memset(buf, 0, sizeof(buf));
    }

    wrefresh(func_menu[4]);

    while((ch = getch()) != ESCAPE)
    {
        while((ch = getch()) != ENTER)
	{
	    if(ch == ESCAPE)
	    {
	        break;
	    }

	    wbkgd(func_menu[selected], COLOR_PAIR(2));
	    wnoutrefresh(func_menu[selected]);
	    if(ch == KEY_DOWN || ch == KEY_UP)
	    {
	        if(ch == KEY_DOWN)
		{
		    selected = (selected + 1) % count;
		}
		else if(ch == KEY_UP)
		{
		    selected = (selected + count - 1) % count;
		}
	    }
	    wbkgd(func_menu[selected], COLOR_PAIR(3));
	    wnoutrefresh(func_menu[selected]);
	    refresh();
	}

	if(ch == ESCAPE)
	{
	    break;
	}

	wclear(func_menu[0]);
	func_menu[0] = subwin(stdscr, 5, 60, 21, 15);
	box(func_menu[0], ACS_VLINE, ACS_HLINE);

	if(selected == 0)
	{
	    if(flag_say == SAY_NO)
	    {
	        wclear(flagWindow);
		waddstr(flagWindow, "已被禁言");
		wrefresh(flagWindow);
	    }
	    else
	    {
	        msg.action = STA;
		chat_menu(sockfd);
	    }
	}
	else if(selected == 1)
	{
	    if(flag_say == SAY_NO)
	    {
	        wclear(flagWindow);
		waddstr(flagWindow, "已被禁言");
		wrefresh(flagWindow);
	    }
	    else
	    {
	        msg.action = STA;
		mood_menu(sockfd);
	    }
	}
	else if(selected == 2)
	{
	    his_sta();
	}
	else if(selected == 3)
	{
	    if(strcmp(user, "peiyuan") == 0)
	    {
	       root_menu(sockfd);
	    }
	    else
	    {
	        wclear(flagWindow);
		waddstr(flagWindow, "操作错误");
		wrefresh(flagWindow);
	    }
	}
    }
    delwin(flagWindow);
    delwin(alertWindow);
    delwin(blankWindow);
    delete_menu(func_menu, 5);
}

/*******************************************************
***查看某个好友的具体信息
***
*******************************************************/
int search_friend_info(int k, char *name)
{
    struct online *temp = head->next;

    if(k == -1)
    {
        while(temp != NULL)
	{
	    if(strcmp(name, temp->name) == 0)
	    {
	        info->fd = temp->fd;
		strcpy(info->name, temp->name);
		strcpy(info->intr, temp->intr);
		break;
	    }
	    temp = temp->next;
	}
	return 0;
    }

    while(temp != NULL)
    {
        if(k == temp->fd)
	{
	    info->fd = temp->fd;
	    strcpy(info->name, temp->name);
	    strcpy(info->intr, temp->intr);
	    break;
	}
	temp = temp->next;
    }
    return 0;
}

/********************************************************
***    在线好友显示
***
********************************************************/
int friend_menu()
{
loop:
    add_friend = 0;
    int ch;
    int i = 0;
    int y = 10;
    int fd[friend_on];

    struct online *temp = head->next;

    WINDOW **my_friend;
    
    my_friend = (WINDOW **)malloc(friend_on * sizeof(WINDOW *));

    init_curses();
    while(temp != NULL)
    {
        my_friend[i] = subwin(stdscr, 1, 15, y, 82);
	wprintw(my_friend[i], "%s", temp->name);
	wrefresh(my_friend[i]);
	fd[i] = temp->fd;
	i++;
	y = y + 2;
	temp = temp->next;
    }

    i = 0;

    while((ch = getch()) != ESCAPE)
    {
        while((ch = getch()) != ENTER)
	{
	    if(ch == ESCAPE)
	    {
	        delete_menu(my_friend, friend_on);
		return -1;
	    }
	    if(add_friend == 1)
	    {
	        goto loop;
	    }
	    if(ch == KEY_DOWN || ch == KEY_UP)
	    {
	        wbkgd(my_friend[i], COLOR_PAIR(2));
		wnoutrefresh(my_friend[i]);

		if(ch == KEY_DOWN)
		{
		    i = (i + 1) % friend_on;
		}
		else if(ch == KEY_UP)
		{
		    i = (i  + friend_on - 1) % friend_on;
		}

		wbkgd(my_friend[i], COLOR_PAIR(3));
		wnoutrefresh(my_friend[i]);
		refresh();
	    }
	}
	search_friend_info(fd[i], 0);
        delete_menu(my_friend, friend_on);
	return 0;
    }

    delete_menu(my_friend, friend_on);

    return -1;
}

/********************************************************
***    登录成功后的窗口显示
***
********************************************************/
void main_menu(int sockfd)
{
    int ch;
    int count = 6;
    int selected = 0;
    char buf[100];

    WINDOW *alertWindow;
    WINDOW **items;
    items = (WINDOW **)malloc(7 * sizeof(WINDOW *));

    init_curses();
    clear();

    flag_blink = 0;

    alertWindow = subwin(stdscr, 28, 28, 2, 80);
    box(alertWindow, ACS_VLINE, ACS_HLINE);
    mvwaddstr(alertWindow, 1, 1, "聊天室");
    mvwaddstr(alertWindow, 2, 2, user);
    wrefresh(alertWindow);

    items[0] = subwin(stdscr, 1, 1, 3, 101);
    waddstr(items[0], "?");
    items[1] = subwin(stdscr, 1, 1, 3, 103);
    waddstr(items[1], "*");
    items[2] = subwin(stdscr, 1, 1, 3, 105);
    waddstr(items[2], "X");
    items[3] = subwin(stdscr, 1, 15, 5, 85);
    wprintw(items[3], "%s", intr);
    items[4] = subwin(stdscr, 1, 6, 7, 82);
    waddstr(items[4], "联系人");
    items[5] = subwin(stdscr, 1, 4, 7, 96);
    waddstr(items[5], "群组");
    items[6] = subwin(stdscr, 1, 15, 9, 82);
    sprintf(buf, ">我的好友(%d/%d)", friend_on, friend_all);
    waddstr(items[6], buf);

    wrefresh(items[0]);
    wrefresh(items[1]);
    wrefresh(items[2]);
    wrefresh(items[3]);
    wrefresh(items[4]);
    wrefresh(items[5]);
    wrefresh(items[6]);

    while((ch = getch()) != ENTER)
    {
        wbkgd(items[selected], COLOR_PAIR(2));
	wnoutrefresh(items[selected]);
	if(ch == KEY_RIGHT || ch == KEY_LEFT || ch == KEY_DOWN || ch == KEY_UP)
	{
	    if(ch == KEY_RIGHT)
	    {
	        selected = (selected + 1) % count;
	    }
	    else if(ch == KEY_LEFT)
	    {
	        selected = (selected + count -1) % count;
	    }
	    else if(ch == KEY_UP || ch == KEY_DOWN)
	    {
	        selected = 6;
	    }
	}
	if(selected == 5)
	{
	    wclear(items[6]);
	    sprintf(buf, "苏嵌108班");
	    waddstr(items[6], buf);
	    wrefresh(items[6]);
	}
	if(selected == 4)
	{
	    wclear(items[6]);
	    sprintf(buf, ">我的好友(%d/%d)", friend_on, friend_all);
	    waddstr(items[6], buf);
	    wrefresh(items[6]);
	}

	wbkgd(items[selected], COLOR_PAIR(3));
	wnoutrefresh(items[selected]);
	refresh();
    }

    memset(&msg, 0, sizeof(struct message));

    if(selected == 0)
    {
        version_menu();
    }

    else if(selected == 1)
    {
        modify_passwd(sockfd);
    }

    else if(selected == 2)
    {
        msg.action = EXIT;
	strcpy(msg.name, user);
	write(sockfd, &msg, sizeof(struct message));
	flag_say = SAY_OUT;
    }

    else if(selected == 3)
    {
        echo();
	wclear(items[3]);
	wgetstr(items[3], intr);

	msg.action = MODIFY_INTR;
	strcpy(msg.name, user);
	strcpy(msg.msg, intr);
	write(sockfd, &msg, sizeof(struct message));
    }

    else if(selected == 6)
    {
        if(strcmp(buf, "苏嵌108班") == 0)
	{
	    flag_blink = 2;
	    sta_menu(sockfd);
	}
	else
	{
	    flag_blink = 1;

	    if(friend_menu(sockfd) == 0)
	    {
	        dialog_menu(sockfd);
	    }
	}
    }

    delwin(alertWindow);
    delete_menu(items, 7);
}

/*********************************************************
***    查询一共有多少个好友在线
***
**********************************************************/
int search_friend(struct online *new_user)
{
    struct online *temp = head->next;

    while(temp != NULL)
    {
        if(strcmp(temp->name, new_user->name) == 0)
	{
	    return -1;
	}

	temp = temp->next;
    }

    return 0;
}

/*********************************************************
***    当有好友下线删除
***
*********************************************************/
int delete_friend(struct online *new_user)
{
    struct online *temp = head->next;
    struct online *p = head;

    while(temp != NULL)
    {
        if(strcmp(new_user->name, temp->name) == 0)
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

/*********************************************************
***    当有好友上线插入链表
***
*********************************************************/
int insert_friend(struct online *new_user)
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

/**********************************************************
***    接收在线好友和下线好友
***
**********************************************************/
void rec_friend(int sockfd, struct online *str)
{
    int k;

    if(read(sockfd, str, sizeof(struct online)) == -1)
    {
        fprintf(stderr, "Read Error:%s\n", strerror(errno));
	exit(1);
    }

    k = str->fd;

    if(read(sockfd, str, sizeof(struct online)) == -1)
    {
        fprintf(stderr, "Read Error:%s\n", strerror(errno));
	exit(1);
    }

    friend_on = str->fd;

    if(read(sockfd, str, sizeof(struct online)) == -1)
    {
        fprintf(stderr, "Read Error:%s\n", strerror(errno));
	exit(1);
    }

    friend_all = str->fd;

    if(k == 1)
    {
        while(read(sockfd, str, sizeof(struct online)))
	{
	    if(str->fd == -1)
	    {
	        break;
	    }
	    if(search_friend(str) != -1)
	    {
	        struct online *ptr = (struct online *)malloc(sizeof(struct online));

		ptr->fd = str->fd;
		strcpy(ptr->name, str->name);
		strcpy(ptr->intr, str->intr);
		insert_friend(ptr);
	    }
	}

	add_friend = 1;
    }
    else
    {
        read(sockfd, str, sizeof(struct online));
	delete_friend(str);
	add_friend = -1;
    }
}

/**********************************************************
***    收到私聊
***
**********************************************************/
void rec_sto()
{
    int fd;
    char file[100];
    char buf[MAX_SIZE];

    WINDOW *alertWindow;
    alertWindow = subwin(stdscr, 12, 35, 7, 18);
    WINDOW **func_menu;
    func_menu = (WINDOW **)malloc(7 * sizeof(WINDOW *));

    init_curses();

    scrollok(alertWindow, TRUE);

    func_menu[0] = subwin(stdscr, 4, 60, 2, 15);
    mvwprintw(func_menu[0], 1, 6, "%s", info->name);
    mvwprintw(func_menu[0], 2, 8, "(%s)", info->intr);
    box(func_menu[0], ACS_VLINE, ACS_HLINE);

    func_menu[1] = subwin(stdscr, 15, 41, 5 ,15);
    box(func_menu[1], ACS_VLINE, ACS_HLINE);

    func_menu[2] = subwin(stdscr, 15, 20, 5, 55);
    box(func_menu[2], ACS_VLINE, ACS_HLINE);

    func_menu[3] = subwin(stdscr, 3, 21, 19, 15);
    mvwaddstr(func_menu[3], 1, 5, "表情");
    box(func_menu[3], ACS_VLINE, ACS_HLINE);

    func_menu[4] = subwin(stdscr, 3, 21, 19, 35);
    mvwaddstr(func_menu[4], 1, 5, "文件");
    box(func_menu[4], ACS_VLINE, ACS_HLINE);

    func_menu[5] = subwin(stdscr, 3, 20, 19, 55);
    mvwaddstr(func_menu[5], 1, 5, "聊天记录");
    box(func_menu[5], ACS_VLINE, ACS_HLINE);

    func_menu[6] = subwin(stdscr, 5, 60, 21, 15);
    mvwaddstr(func_menu[6], 1, 5, "开始聊天");
    box(func_menu[6], ACS_VLINE, ACS_HLINE);
    wrefresh(func_menu[0]);
    wrefresh(func_menu[1]);
    wrefresh(func_menu[2]);
    wrefresh(func_menu[3]);
    wrefresh(func_menu[4]);
    wrefresh(func_menu[5]);
    wrefresh(func_menu[6]);

    sprintf(file, "./mychat/%s.txt", info->name);
    if((fd = open(file, O_CREAT | O_RDWR | O_APPEND, 0777)) < 0)
    {
        printf("打开文件失败!\n");
	exit(1);
    }

    memset(buf, 0, sizeof(buf));
    lseek(fd, 0, SEEK_SET);
    while(read_line(fd, buf, MAX_SIZE))
    {
        wprintw(alertWindow, "%s\n", buf);
	memset(buf, 0, sizeof(buf));
    }

    wrefresh(alertWindow);

    delwin(alertWindow);
    delete_menu(func_menu, 7);
}

/********************************************************
***    收到群聊消息
***
********************************************************/
void rec_sta()
{
    int fd;
    char file[100];
    char buf[MAX_SIZE];

    init_curses();

    WINDOW *alertWindow;
    WINDOW *blankWindow;
    WINDOW **func_menu;
    func_menu = (WINDOW **)malloc(5 * sizeof(WINDOW *));

    alertWindow = subwin(stdscr, 5, 60, 1, 15);
    mvwaddstr(alertWindow, 1, 4, "苏嵌108群聊");
    mvwaddstr(alertWindow, 2, 8, "本群管理员peiyuan");
    box(alertWindow, ACS_VLINE, ACS_HLINE);
    wrefresh(alertWindow);

    blankWindow = subwin(stdscr, 15, 60, 5, 15);
    box(blankWindow, ACS_VLINE, ACS_HLINE);
    wrefresh(blankWindow);

    func_menu[0] = subwin(stdscr, 5, 60, 21, 15);
    mvwaddstr(func_menu[0], 1, 5, "开始聊天");
    box(func_menu[0], ACS_VLINE, ACS_HLINE);

    func_menu[1] = subwin(stdscr, 3, 21, 19, 15);
    mvwaddstr(func_menu[1], 1, 5, "表情");
    box(func_menu[1], ACS_VLINE, ACS_HLINE);

    func_menu[2] = subwin(stdscr, 3, 21, 19, 35);
    mvwaddstr(func_menu[2], 1, 5, "聊天记录");
    box(func_menu[2], ACS_VLINE, ACS_HLINE);

    func_menu[3] = subwin(stdscr, 3, 20, 19, 55);
    mvwaddstr(func_menu[3], 1, 5, "管理员操作");
    box(func_menu[3], ACS_VLINE, ACS_HLINE);

    func_menu[4] = subwin(stdscr, 12, 55, 7, 18);
    scrollok(func_menu[4], TRUE);

    wrefresh(func_menu[0]);
    wrefresh(func_menu[1]);
    wrefresh(func_menu[2]);
    wrefresh(func_menu[3]);
    wrefresh(func_menu[4]);

    sprintf(file, "./mychat/sta_chat.txt");
    if((fd = open(file, O_CREAT | O_RDWR | O_APPEND, 0777)) < 0)
    {
        perror("open file failure!\n");
	exit(1);
    }

    memset(buf, 0, sizeof(buf));
    lseek(fd, 0, SEEK_SET);

    while(read_line(fd, buf, MAX_SIZE))
    {
        wprintw(func_menu[4], "%s\n", buf);
	memset(buf, 0, sizeof(buf));
    }

    wrefresh(func_menu[4]);

    delwin(alertWindow);
    delwin(blankWindow);
    delete_menu(func_menu, 5);
}

/*********************************************************
***    发送私聊
***
**********************************************************/
void send_sto()
{
    int fd;
    char file[100];
    char buf[MAX_SIZE];

    WINDOW *alertWindow;
    alertWindow = subwin(stdscr, 12, 35, 7, 18);

    init_curses();

    scrollok(alertWindow, TRUE);

    sprintf(file, "./mychat/%s.txt", msg.toname);
    if((fd = open(file, O_CREAT | O_RDWR | O_APPEND, 0777)) < 0)
    {
        printf("打开文件失败!\n");
	exit(1);
    }

    time_t timep;
    time(&timep);
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%s", ctime(&timep));
    write(fd, buf, strlen(buf));

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "我:%s", msg.msg);
    write(fd, buf, strlen(buf));
    write(fd, "\n", 1);
    write(fd, "\n", 1);

    lseek(fd, 0, SEEK_SET);
    memset(buf, 0, sizeof(buf));

    while(read_line(fd, buf, MAX_SIZE))
    {
        wprintw(alertWindow, "%s\n", buf);
	memset(buf, 0, sizeof(buf));
    }

    wrefresh(alertWindow);

    delwin(alertWindow);
}

/*********************************************************
***    发送群聊消息
***
*********************************************************/
void send_sta()
{
    int fd;
    char file[100];
    char buf[MAX_SIZE];

    init_curses();

    WINDOW *alertWindow;
    WINDOW *blankWindow;
    WINDOW **func_menu;
    func_menu = (WINDOW **)malloc(5 * sizeof(WINDOW *));

    alertWindow = subwin(stdscr, 5, 60, 1, 15);
    mvwaddstr(alertWindow, 1, 4, "苏嵌108群聊");
    mvwaddstr(alertWindow, 2, 8, "本群管理员peiyuan");
    box(alertWindow, ACS_VLINE, ACS_HLINE);
    wrefresh(alertWindow);

    blankWindow = subwin(stdscr, 15, 60, 5, 15);
    box(blankWindow, ACS_VLINE, ACS_HLINE);
    wrefresh(blankWindow);

    func_menu[0] = subwin(stdscr, 5, 60, 21, 15);
    mvwaddstr(func_menu[0], 1, 5, "开始聊天");
    box(func_menu[0], ACS_VLINE, ACS_HLINE);

    func_menu[1] = subwin(stdscr, 3, 21, 19, 15);
    mvwaddstr(func_menu[1], 1, 5, "表情");
    box(func_menu[1], ACS_VLINE, ACS_HLINE);

    func_menu[2] = subwin(stdscr, 3, 21, 19, 35);
    mvwaddstr(func_menu[2], 1, 5, "聊天记录");
    box(func_menu[2], ACS_VLINE, ACS_HLINE);

    func_menu[3] = subwin(stdscr, 3, 20, 19, 55);
    mvwaddstr(func_menu[3], 1, 5, "管理员操作");
    box(func_menu[3], ACS_VLINE, ACS_HLINE);

    func_menu[4] = subwin(stdscr, 12, 55, 7, 18);
    scrollok(func_menu[4], TRUE);

    wrefresh(func_menu[0]);
    wrefresh(func_menu[1]);
    wrefresh(func_menu[2]);
    wrefresh(func_menu[3]);
    wrefresh(func_menu[4]);

    sprintf(file, "./mychat/sta_chat.txt");
    if((fd = open(file, O_CREAT | O_RDWR | O_APPEND, 0777)) < 0)
    {
        perror("open file failure!\n");
	exit(1);
    }

    time_t timep;
    time(&timep);
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%s", ctime(&timep));
    write(fd, buf, strlen(buf));

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "我:%s", msg.msg);
    write(fd, buf, strlen(buf));
    write(fd, "\n", 1);
    write(fd, "\n", 1);

    lseek(fd, 0, SEEK_SET);

    memset(buf, 0, sizeof(buf));
    while(read_line(fd, buf, MAX_SIZE))
    {
        wprintw(func_menu[4], "%s\n", buf);
	memset(buf, 0, sizeof(buf));
    }
    wrefresh(func_menu[4]);

    delwin(alertWindow);
    delwin(blankWindow);
    delete_menu(func_menu, 5);
}

/**********************************************************
***    收到文件
***
***********************************************************/
void rec_file(int sockfd)
{
    int fd;
    int obj = 0;
    int i = 0;
    int y = 10;
    char name[20];
    char file[100];
    char buf[100];

    struct online *temp = head->next;

    WINDOW *alertWindow;
    WINDOW *recWindow;
    WINDOW **my_friend;
    WINDOW **func_menu;

    my_friend = (WINDOW **)malloc(friend_on * sizeof(WINDOW *));
    func_menu = (WINDOW **)malloc(7 * sizeof(WINDOW *));

    init_curses();

    alertWindow = subwin(stdscr, 1, 15, 9, 82);
    sprintf(buf, ">我的好友(%d/%d)", friend_on, friend_all);
    waddstr(alertWindow, buf);

    strcpy(name, msg.name);
    strcpy(file, msg.msg);

    while(flag_blink != 1)
    {
        wclear(alertWindow);
	wrefresh(alertWindow);
	usleep(500000);
	sprintf(buf, ">我的好友(%d/%d)", friend_on, friend_all);
	waddstr(alertWindow, buf);
	wrefresh(alertWindow);
	usleep(500000);
    }

    while(temp != NULL)
    {
        my_friend[i] = subwin(stdscr, 1, 15, y, 82);
	wprintw(my_friend[i], "%s", temp->name);
	wrefresh(my_friend[i]);
	if(strcmp(name, temp->name) == 0)
	{
	    obj = i;
	}
	i++;
	y = y + 2;
	temp = temp->next;
    }

    while(strcmp(name, info->name) != 0)
    {
        wclear(my_friend[obj]);
	wrefresh(my_friend[obj]);
	usleep(500000);
	wprintw(my_friend[obj], "%s", name);
	wrefresh(my_friend[obj]);
	usleep(500000);
    }

    func_menu[0] = subwin(stdscr, 4, 60, 2, 15);
    mvwprintw(func_menu[0], 1, 6, "%s", info->name);
    mvwprintw(func_menu[0], 2, 8, "(%s)", info->intr);
    box(func_menu[0], ACS_VLINE, ACS_HLINE);

    func_menu[1] = subwin(stdscr, 15, 41, 5, 15);
    box(func_menu[1], ACS_VLINE, ACS_HLINE);

    func_menu[2] = subwin(stdscr, 15, 20, 5, 55);
    box(func_menu[2], ACS_VLINE, ACS_HLINE);

    func_menu[3] = subwin(stdscr, 3, 21, 19, 15);
    mvwaddstr(func_menu[3], 1, 5, "表情");
    box(func_menu[3], ACS_VLINE, ACS_HLINE);

    func_menu[4] = subwin(stdscr, 3, 21, 19, 35);
    mvwaddstr(func_menu[4], 1, 5, "文件");
    box(func_menu[4], ACS_VLINE, ACS_HLINE);

    func_menu[5] = subwin(stdscr, 3, 20, 19, 55);
    mvwaddstr(func_menu[5], 1, 5, "聊天记录");
    box(func_menu[5], ACS_VLINE, ACS_HLINE);

    func_menu[6] = subwin(stdscr, 5, 60, 21, 15);
    box(func_menu[6], ACS_VLINE, ACS_HLINE);
    wrefresh(func_menu[0]);
    wrefresh(func_menu[1]);
    wrefresh(func_menu[2]);
    wrefresh(func_menu[3]);
    wrefresh(func_menu[4]);
    wrefresh(func_menu[5]);
    wrefresh(func_menu[6]);

    recWindow = subwin(stdscr, 12, 35, 7, 18);
    wclear(recWindow);

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "./myfile/%s.txt", file);

    fd = open(buf, O_RDWR | O_CREAT | O_TRUNC, 0655);

    wprintf(recWindow, "正在接收来自%s的文件!", name);
    wrefresh(recWindow);

    while(read(sockfd, &msg, sizeof(struct message)))
    {
        if(msg.action == REC_OVER)
	{
	    break;
	}

	write(fd, msg.msg, strlen(msg.msg));
	memset(msg.msg, 0, sizeof(msg.msg));
    }

    waddstr(recWindow, "\n接收完毕!");
    wrefresh(recWindow);

    delwin(recWindow);
    delwin(alertWindow);
    delete_menu(my_friend, friend_on);
    delete_menu(func_menu, 7);
}

/***********************************************************
***    显示好友
***
***********************************************************/
void dis_friend(int sockfd, struct online *str)
{
    int i = 0;
    int y = 0;
    int fd[friend_on];
    struct online *temp = head->next;

    WINDOW *alertWindow;
    WINDOW *clearWindow;
    WINDOW *dialogWindow;
    WINDOW **my_friend;
    my_friend = (WINDOW **)malloc(friend_on *sizeof(WINDOW *));

    init_curses();

    alertWindow = subwin(stdscr, 1, 15, 9, 82);
    wprintw(alertWindow, ">我的好友(%d/%d)", friend_on, friend_all);
    wrefresh(alertWindow);

    clearWindow = subwin(stdscr, (friend_on + 1) * 2, 15, 10, 82);
    dialogWindow = subwin(stdscr, 25, 60, 2, 15);

    if(flag_blink == 1)
    {
        wclear(clearWindow);
	wrefresh(clearWindow);

	while(temp != NULL)
	{
	    my_friend[i] = subwin(stdscr, 1, 15, y, 82);
	    wprintw(my_friend[i], "%s", temp->name);
	    wrefresh(my_friend[i]);
	    fd[i] = temp->fd;
	    i++;
	    y = y + 2;
	    temp = temp->next;
	}
    }

    if(strcmp(info->name, str->name) == 0)
    {
        wclear(dialogWindow);
	wrefresh(dialogWindow);
    }

    delwin(clearWindow);
    delwin(alertWindow);
    delwin(dialogWindow);
    delete_menu(my_friend, friend_on);
}

/************************************************************
**    客户端写线程
**
************************************************************/
void write_result(int sockfd)
{
    int n;

    while(msg.action != LOG_SUCCESS)
    {
        n = log_select(sockfd);
	if(n == EXIT)
	{
	    clear();
	    endwin();
	    exit(1);
	}
    }

    msg.action = CHE;
    strcpy(msg.name, user);
    write(sockfd, &msg, sizeof(struct message));

    while(1)
    {
        if(flag_say == SAY_OUT)
	{
	    msg.action = EXIT;
	    strcpy(msg.name, user);
	    write(sockfd, &msg, sizeof(struct message));
	    clear();
	    endwin();
	    exit(1);
	}
	main_menu(sockfd);
    }
}

/*****************************************************
***    客户端读线程
***
*****************************************************/
void read_result(int sockfd)
{
    char buf[MAX_SIZE];

    WINDOW *alertWindow;

    init_curses();

    alertWindow = subwin(stdscr, 15, 60, 5, 15);

    while(read(sockfd, &msg, sizeof(msg)))
    {
        switch(msg.action)
	{
	    case REG_SUCCESS:
	    {
	        alertWindow = subwin(stdscr, 15, 60, 5, 15);
		sprintf(buf, "注册成功,您的ID:%s", msg.msg);
		mvwaddstr(alertWindow, 8, 10, buf);
		mvwaddstr(alertWindow, 10, 10, "按回车键重新注册!");
		mvwaddstr(alertWindow, 12, 10, "按HOME键返回登录界面!");
		wrefresh(alertWindow);
		break;	    
	    }
	    case NAME_EXIST:
	    {
	        alertWindow = subwin(stdscr, 15, 60, 5, 15);
		mvwaddstr(alertWindow, 2, 40, "用户已存在!");
		mvwaddstr(alertWindow, 10, 10, "按回车键重新注册!");
		mvwaddstr(alertWindow, 12, 10, "按HOME键返回登录界面!");
		wrefresh(alertWindow);
		break;
	    }
	    case USER_ONLINE:
	    {
	        alertWindow = subwin(stdscr, 1, 20, 18, 30);
		waddstr(alertWindow, "用户在线");
		wrefresh(alertWindow);

		break;
	    }
	    case NAME_WRONG:
	    {
	        alertWindow = subwin(stdscr, 1, 20, 18, 30);
		waddstr(alertWindow, "帐号错误!");
		wrefresh(alertWindow);
		break;
	    }
	    case PASSWD_WRONG:
	    {
	        alertWindow = subwin(stdscr, 1, 20, 18, 30);
		waddstr(alertWindow, "密码错误!");
		wrefresh(alertWindow);
		break;
	    }
	    case LOG_SUCCESS:
	    {
	        strcpy(user, msg.name);
		strcpy(intr, msg.msg);
		break;
	    }
	    case ONLINE_FRIEND:
	    {
	        struct online *str = (struct online *)malloc(sizeof(struct online));
		rec_friend(sockfd, str);
		dis_friend(sockfd, str);

		break;
	    }
	    case PASSWD_MODIFY_SUCCESS:
	    {
	        alertWindow = subwin(stdscr, 15, 60, 5, 15);
		mvwaddstr(alertWindow, 8, 10, "密码更改成功!");
		mvwaddstr(alertWindow, 12, 10, "按ESC键退出重新登录!");
		wrefresh(alertWindow);
		flag_say = SAY_OUT;
		break;
	    }
            case STO_MSG:
	    {
	        blink_menu();
		rec_sto();
		break;
	    }
	    case STO_SUCCESS:
	    {
	        send_sto();
		break;
	    }
	    case REC_FILE:
	    {
	        rec_file(sockfd);
		break;
	    }
	    case STA_MSG:
	    {
	        sta_blink();
		rec_sta();
		break;
	    }
	    case STA_SUCCESS:
	    {
	        send_sta();
		break;
	    }
	    case SAY_NO:
	    {
	        flag_say = SAY_NO;

		say_menu();
		break;
	    }
	    case SAY_YES:
	    {
	        flag_say = SAY_YES;
		say_menu();
		break;
	    }
	    case NO_PERSON:
	    {
	        alertWindow = subwin(stdscr, 3, 50, 22, 20);
		wclear(alertWindow);
		waddstr(alertWindow, "无此好友");
		wrefresh(alertWindow);
		break;
	    }
	    case ALREADY_NO:
	    {
	        alertWindow = subwin(stdscr, 3, 50, 22, 20);
		wclear(alertWindow);
		waddstr(alertWindow, "重复操作");
		wrefresh(alertWindow);
		break;
	    }
	    case ALREADY_YES:
	    {
	        alertWindow = subwin(stdscr, 3, 50, 22, 20);
		wclear(alertWindow);
		waddstr(alertWindow, "重复操作");
		wrefresh(alertWindow);
		break;
	    }
	}
    }
    delwin(alertWindow);
}

void write_msg(void *arg)
{
    int sockfd = *((int *)arg);

    write_result(sockfd);
}

void read_msg(void *arg)
{
    int sockfd = *((int *)arg);

    read_result(sockfd);
}

void func(int signal)
{
    if(signal == SIGINT)
    {
    
    }
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in server_addr;
    if(argc != 2)
    {
        fprintf(stderr, "Usage:%s hostname \a\n", argv[0]);
	exit(1);
    }

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Socket Error:%s\a\n", strerror(errno));
	exit(1);
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portnumber);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    if(connect(sockfd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr)) == -1)
    {
        fprintf(stderr, "Connect Error:%s\a\n", strerror(errno));
	exit(1);
    }

    signal(SIGINT, func);

    create_online();

    clear();
    init_curses();
    bkgd(COLOR_PAIR(2));
    refresh();

    pthread_t id1;
    pthread_t id2;

    pthread_create(&id1, NULL, (void *)read_msg, (void *)&sockfd);
    pthread_create(&id2, NULL, (void *)write_msg, (void *)&sockfd);

    pthread_join(id1, NULL);
    printf("thread1 is closed!\n");
    pthread_join(id2, NULL);
    printf("thread2 is closed!\n");

    close(sockfd);
    exit(0);
}


