
#include "lst_timer.h"
#include "../http/http_conn.h"

HeapTimer heaptimer;

sort_timer_lst::sort_timer_lst()
{
    head=NULL;
    tail=NULL;
}

sort_timer_lst::~sort_timer_lst()
{
    util_timer *tmp=head;
    while(tmp)
    {
        head=tmp->next;
        delete tmp;
        tmp=head;
    }
}
void sort_timer_lst::add_timer(util_timer *timer)
{
    heaptimer.add(timer);
}

void sort_timer_lst::adjust_timer(util_timer *timer)
{
    if(!timer)
    {
        return;
    }
    util_timer *tmp=timer->next;
    if(!tmp||(timer->expire < tmp->expire) )
    {
        return;
    }
    if(timer==head)
    {
        head=head->next;
        head->prev=NULL;
        timer->next=NULL;
        add_timer(timer,head);
    }
    else
    {
        timer->prev->next=timer->next;
        timer->next->prev=timer->prev;
        add_timer(timer,timer->next);
    }
}

void sort_timer_lst::del_timer(util_timer *timer)
{
    heaptimer.clear();
}

void sort_timer_lst::tick()
{
    heaptimer.tick();
}

void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_head)
{
    util_timer *prev = lst_head;
    util_timer *tmp = prev->next;
    while(tmp)
    {
        if(timer->expire < tmp->expire )
        {
            prev->next=timer;
            timer->next=tmp;
            tmp->prev=timer;
            timer->prev=prev;
            break;
        }
        prev=tmp;
        tmp=tmp->next;
    }

    if (!tmp)
    {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}

void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}
//信号处理函数
void Utils::sig_handler(int sig)
{
    int save_errno=errno;
    int msg=sig;
    send(u_pipefd[1],(char *)&msg,1,0 );
    errno=save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{

    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa) );
    sa.sa_handler=handler;
    if(restart)
    {
        sa.sa_flags|=SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL)!=-1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_timer_lst.tick();
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd,EPOLL_CTL_DEL,user_data->sockfd,0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}




void HeapTimer::siftup_(size_t i) 
{
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;
    while(j >= 0)
    {
        if(heap_[j] < heap_[i]) 
        { 
            break; 
        }
        SwapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

void HeapTimer::SwapNode_(size_t i, size_t j) 
{
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].fd] = i;
    ref_[heap_[j].fd] = j;
} 

bool HeapTimer::siftdown_(size_t index, size_t n) 
{
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while(j < n) 
    {
        if(j + 1 < n && heap_[j + 1] < heap_[j]) j++;
        if(heap_[i] < heap_[j]) break;
        SwapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void HeapTimer::add(util_timer *timer) 
{
    int id=timer->user_data->sockfd;
    assert(id >= 0);

    size_t i;

    TimerNode tmp;
    tmp.fd=timer->user_data->sockfd;
    tmp.cb_func=timer->cb_func;
    tmp.expire=timer->expire;
    tmp.user_data=timer->user_data;

    if(ref_.count(id) == 0) {
        /* 新节点：堆尾插入，调整堆 */
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back(tmp);
        siftup_(i);
    } 
    else {
        /* 已有结点：调整堆 */
        i = ref_[id];
        heap_[i].expire = tmp.expire;
        heap_[i].cb_func = cb_func;
        if(!siftdown_(i, heap_.size())) {
            siftup_(i);
        }
    }
}

void HeapTimer::doWork(int id) 
{
    /* 删除指定id结点，并触发回调函数 */
    if(heap_.empty() || ref_.count(id) == 0) 
    {
        return;
    }
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    node.cb_func(node.user_data);
    del_(i);
}

void HeapTimer::del_(size_t index) 
{
    /* 删除指定位置的结点 */
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    /* 将要删除的结点换到队尾，然后调整堆 */
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if(i < n) 
    {
        SwapNode_(i, n);
        if(!siftdown_(i, n)) 
        {
            siftup_(i);
        }
    }
    /* 队尾元素删除 */
    ref_.erase(heap_.back().fd);
    heap_.pop_back();
}

void HeapTimer::adjust(int id, int timeout) 
{
    /* 调整指定id的结点 */
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expire = timeout;
    siftdown_(ref_[id], heap_.size());
}

void HeapTimer::tick() 
{
    /* 清除超时结点 */
    if(heap_.empty()) 
        return;

    time_t cur=time(NULL);
    while(!heap_.empty()) 
    {
        TimerNode node = heap_.front();
        if(cur<node.expire ) 
            break; 

        node.cb_func(node.user_data);
        pop();
    }
}

void HeapTimer::pop() 
{
    assert(!heap_.empty());
    del_(0);
}

void HeapTimer::clear() 
{
    ref_.clear();
    heap_.clear();
}






