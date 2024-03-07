#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unordered_map>
#include <queue>

#include <time.h>
#include "../log/log.h"


class util_timer;

struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

struct TimerNode 
{
    int fd;
    //超时时间
    time_t expire;
    //回调函数
    void (* cb_func)(client_data *);
    //连接资源
    client_data *user_data;
    bool operator<(const TimerNode& t) {
        return expire < t.expire;
    }
};

class HeapTimer {
public:
    HeapTimer() { heap_.reserve(64); }

    ~HeapTimer() { clear(); }
    
    void adjust(int id, int newExpires);

    void add(util_timer *timer);

    void doWork(int id);

    void clear();

    void tick();

    void pop();

  //  int GetNextTick();

private:
    void del_(size_t i);
    
    void siftup_(size_t i);

    bool siftdown_(size_t index, size_t n);

    void SwapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;

    std::unordered_map<int, size_t> ref_;
};

class util_timer
{
public:
    util_timer() : prev(NULL),next(NULL){}


public:
    //超时时间
    time_t expire;
    //回调函数
    void (* cb_func)(client_data *);
    //连接资源
    client_data *user_data;
    //前向定时器
    util_timer *prev;
    //后继定时器
    util_timer *next;
};


class sort_timer_lst
{
public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(util_timer *timer);
    void adjust_timer(util_timer *timer);
    void del_timer(util_timer *timer);
    void tick();

private:
    void add_timer(util_timer *timer,util_timer *lst_head);
    util_timer *head;
    util_timer *tail;
};


class Utils
{
public:
    Utils(){}
    ~Utils(){}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd,int fd,bool one_shot,int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

     //设置信号函数
    void addsig(int sig,void (handler)(int),bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd,const char *info);

public:
    static int *u_pipefd;
    sort_timer_lst m_timer_lst;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(client_data *user_data);


#endif



