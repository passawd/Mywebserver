#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class sem
{
public:
    sem()
    {
        if(sem_init(&m_sem,0,0)!=0 )
        {
            throw std::exception();
            //初始化信号量
        }
    }
    sem(int num)
    {
        if( sem_init(&m_sem,0,num)!=0 )
        {
            throw std::exception();
            //初始化信号量
        }
    }
    ~sem()
    {
        sem_destroy(&m_sem);
        //析构函数销毁信号量
    }
    bool wait()
    {
        return sem_wait(&m_sem)==0;
        //占用一个信号量，若信号量为0则阻塞
    }
    bool post()
    {
        return sem_post(&m_sem)==0;
        //释放一个信号量
    }


private:
    sem_t m_sem;
    //创建信号量对象
};


class locker
{
public:
    locker()
    {
        if(pthread_mutex_init(&m_mutex,NULL)!=0 )
        {
            throw std::exception();
            //初始化锁
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex)==0;
        //阻塞调用锁，即在锁被占用的时候不能被抢占
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex)==0;
        //释放锁
    }

    pthread_mutex_t *get()
    {
        return &m_mutex;
    }


private:
    pthread_mutex_t m_mutex;
};


class cond
{
public:
    cond()
    {
        if( pthread_cond_init(&m_cond,NULL)==0 )
        {
            throw std::exception();
        }
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }

    bool wait(pthread_mutex_t *m_mutex)
    {
        int ret = 0;
        ret=pthread_cond_wait(&m_cond,m_mutex);
        return ret ==0;
    }
    bool timewait(pthread_mutex_t *m_mutex,struct  timespec t)
    {
         int ret = 0;
        ret=pthread_cond_timedwait(&m_cond,m_mutex,&t);
        return ret ==0;
    }
    bool signal()
    {
        return pthread_cond_signal(&m_cond)==0;
    }
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond)==0;
    }

private:
pthread_cond_t m_cond;

};
#endif


