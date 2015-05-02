#ifndef _THREAD_QUEUE_HEADER_INCLUDE_
#define _THREAD_QUEUE_HEADER_INCLUDE_

// Thread-safe templatized queue class

#include <queue>
#include <time.h>
#include <pthread.h>
#include <errno.h>

template <class T>
class ThreadQueue
{
public:
  ThreadQueue();
  virtual ~ThreadQueue();

  int putMessage(const T& msg);

  int clear();
  int empty();

  template< typename FUNCTOR >
  int apply( FUNCTOR f );

  int getMessage(T& msg, const timespec* abs_timeout);
  int getMessage(T& msg, int msec_timeout);

private:
  pthread_mutex_t m_mutex;
  pthread_cond_t m_condition;

  std::queue<T> m_queue;
};

template< typename T >
class ClearQueueFunctor
{
public:
  int operator() ( std::queue<T> & queue )
  { while ( queue.size() > 0 ) { queue.pop(); } return 0; }
};

template< typename T >
class QueueEmptyFunctor
{
public:
  int operator() ( std::queue<T> & queue )
  { return queue.empty() ? 1:0; }
};

template< typename T >
class QueueSizeFunctor
{
public:
  int operator() ( std::queue<T> & queue )
  { return queue.size(); }
};

namespace ThreadQueueNS
{
class TGuard
{
public:
  TGuard(pthread_mutex_t* pm)
    : m_pm(pm),
    m_lockHeld(false) {}

  ~TGuard()
  {
    if (m_lockHeld)
    {
      int e = pthread_mutex_unlock(m_pm);
      if (0 == e)
        m_lockHeld = false;
    }
  }

  int acquire()
  {
    int e = pthread_mutex_lock(m_pm);
    if (e == 0)
    {
      m_lockHeld = true;
    }
    return e;
  }

  int acquire_timed(const timespec* abs_timeout)
  {
    int e = pthread_mutex_timedlock(m_pm, abs_timeout);
    if (e == 0)
    {
      m_lockHeld = true;
    }
    return e;
  }

  int cond_timedwait(pthread_cond_t* pCond,
		     const timespec* abs_timeout)
  {
    m_lockHeld = false;
    int e = pthread_cond_timedwait(pCond, m_pm, abs_timeout);
    if (e == 0 || e == ETIMEDOUT)
    {
      m_lockHeld = true;
    }
    return e;
  }

  int release()
  {
    if (m_lockHeld)
    {
      int e = pthread_mutex_unlock(m_pm);
      if (e == 0)
      {
	m_lockHeld = false;
      }
      return e;
    }
    return 0;
  }
private:
  pthread_mutex_t* m_pm;
  bool m_lockHeld;
};
} // end namespace

template <class T>
ThreadQueue<T>::ThreadQueue()
{
  pthread_mutex_init(&m_mutex, NULL);
  pthread_cond_init(&m_condition, NULL);
}

template <class T>
ThreadQueue<T>::~ThreadQueue()
{
  pthread_mutex_destroy(&m_mutex);
  pthread_cond_destroy(&m_condition);
}

template <class T>
int ThreadQueue<T>::putMessage(const T& msg)
{
  ThreadQueueNS::TGuard g(&m_mutex);
  g.acquire();
  // Note that with standard queues, push is essentially push_back
  m_queue.push(msg);
  g.release();
  pthread_cond_broadcast(&m_condition);
  return 0;
}

template <class T>
int ThreadQueue<T>::clear()
{
  return this->apply( ClearQueueFunctor<T>() );
}

template <class T>
int ThreadQueue<T>::empty()
{
  return this->apply( QueueEmptyFunctor<T>() );
}

template <class T>
template <class FUNCTOR>
int ThreadQueue<T>::apply( FUNCTOR f )
{
  ThreadQueueNS::TGuard g(&m_mutex);
  g.acquire();
  int retval = f( m_queue );
  g.release();
  return retval;
}

template <class T>
int ThreadQueue<T>::getMessage(T& msg, int msec_timeout)
{
  struct timespec delay;
  clock_gettime(CLOCK_REALTIME, &delay);
  delay.tv_sec = delay.tv_sec + (msec_timeout / 1e3);
  delay.tv_nsec = delay.tv_nsec + (msec_timeout % (int)1e6);
  if (delay.tv_nsec > 1e9)
  {
    delay.tv_nsec -= 1e9;
    delay.tv_sec += 1;
  }
  return getMessage(msg, &delay);
}

template <class T>
int ThreadQueue<T>::getMessage(T& msg, const timespec* abs_timeout)
{
  // Check timespec input - return an error immediately if it is not valid
  if (abs_timeout != 0 &&
      (abs_timeout->tv_nsec > 999999999 || abs_timeout->tv_nsec < 0) )
  {
    // timespec is expected to be split into seconds and nanoseconds
    // Deeper layers will fail if this split isn't respected, due to having
    // more than 1 billion nanoseconds specified.
    // Prevent deeper failure here by denying the getMessage call.
    return EINVAL;
  }

  ThreadQueueNS::TGuard g(&m_mutex);
  int e = g.acquire_timed(abs_timeout);
  if (e != 0)
  {
    return e;
  }
  if (m_queue.size() > 0)
  {
    // Peel oldest elements off the front
    msg = m_queue.front();
    // Note that with standard queues, pop is essentially pop_front
    m_queue.pop();
    // Rely on the guard to release the lock
    return 0;
  }
  e = g.cond_timedwait(&m_condition, abs_timeout);
  while (e == 0)
  {
    // At this point we have the lock.  Check to see if there really
    // is a message waiting.  It is possible that this could be a
    // spurious wake that should be ignored.
    if (m_queue.size() > 0)
    {
      // Peel oldest elements off the front
      msg = m_queue.front();
      // Note that with standard queues, pop is essentially pop_front
      m_queue.pop();
      // Rely on the guard to release the lock
      return 0;
    }
    // If there was no message waiting, the wake was spurious - but
    // was not due to timeout (or e would be nonzero).  Repeat the wait
    // on the condition variable and loop.
    e = g.cond_timedwait(&m_condition, abs_timeout);
  }
  // If we got here, it was because pthread_cond_timedwait returned
  // an error.  Simply pass that error along to the user.
  return e;
}

#endif
