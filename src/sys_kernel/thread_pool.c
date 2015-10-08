#include <mqueue.h>
#include <stdlib.h>
#include <pthread.h>
#include "thread_pool.h"

#define QUEUE_NAME_LENGTH 10000

struct threadpool_t {
  pthread_mutex_t lock;
  pthread_cond_t cond;
  pthread_t *threads;
  mqd_t mq;
  int pending_count;   //Count of the number of pending jobs in the message queue
  int thread_count;
  int shutdown;
};

struct threadpool_job {
  void(*func)(void *);
  void *arg;
};

static void *thr_fn(void *arg);

char q_name[QUEUE_NAME_LENGTH];

/*Generate a random queue name everytime so processes don't interfere with each other*/
static void get_random_qname(char *qname, int len)
{
  const char alph [] = "ABCDEFGHIJKLMONPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz";
  qname[0] = '/';
  int i;
  for(i = 1; i < len; i++) {
    qname[i] = alph[rand() % (sizeof(alph) -1)];
  }

  qname[len] = 0;
}

/*Function to allocate data structures and create worker threads, returns if error is encountered in any function*/
void *init_threadpool(int num_threads)
{
  /*Message queue attributes*/
  struct mq_attr attr;
  attr.mq_maxmsg = 10;
  attr.mq_flags = 0;
  attr.mq_msgsize = sizeof(struct threadpool_job);
  attr.mq_curmsgs = 0;
  get_random_qname(q_name, QUEUE_NAME_LENGTH);


  struct threadpool_t *pool;
  int i;

  if((pool = (struct threadpool_t *)malloc(sizeof(struct threadpool_t))) == NULL) {
    return NULL;
  }

  pool->thread_count = 0;
  pool->pending_count = 0;
  pool->shutdown = 0;
  mqd_t m;

  /*Create a message queue for storing pending job requests*/
  mq_unlink(q_name);

  if(( m = mq_open(q_name, O_RDWR | O_CREAT, 0666, &attr)) == -1) {
    free(pool);
    return NULL;
  }

  pool->mq = m;

  /*Initialize mutex and condition variables*/
  if((pthread_mutex_init(&pool->lock, NULL)) != 0) {
    free(pool);
    return NULL;
  }

  if(pthread_cond_init(&pool->cond, NULL) != 0) {
    pthread_mutex_destroy(&pool->lock);
    free(pool);
    return NULL;
  }

  if((pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads)) == NULL) {
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
    free(pool);
    return NULL;
  }

  /*Start Worker threads*/
  for(i = 0; i < num_threads; i++) {
    if(pthread_create(&pool->threads[i], NULL, thr_fn, (void *)pool) != 0) {
      pthread_mutex_destroy(&pool->lock);
      pthread_cond_destroy(&pool->cond);
      free(pool->threads);
      free(pool);
      return NULL;
      }

    pool->thread_count++;
    }

  /*Returns opaque pointer*/
  return (void *)pool;

}

/*Function adds a job to the message queue, returns 0 on success and -1 on failure amd 1 if pool is shutting down*/
int submit_job(void *p, void(*function)(void *), void *argument)
{
  struct threadpool_t *pool = (struct threadpool_t *)p;
  struct threadpool_job j;
  j.func = function;
  j.arg = argument;

  if(pool == NULL || function == NULL) {
    return -1;
  }

   if(pthread_mutex_lock(&pool->lock) != 0) {
     return -1;
  }

   if(pool->shutdown) {
     pthread_mutex_unlock(&pool->lock);
     return 1;
   }

   if(pthread_mutex_unlock(&pool->lock) != 0) {
     return -1;
  }

  /*Add job to message queue, sending pointer*/
  if(mq_send(pool->mq, (const char *)&j, sizeof(j), 0) == -1) {
    return -1;
  }


  if(pthread_mutex_lock(&pool->lock) != 0) {
    return -1 ;
  }

  pool->pending_count++;

  if(pthread_cond_signal(&pool->cond) != 0) {
    return -1;
  }

  if(pthread_mutex_unlock(&pool->lock) != 0) {
    return -1;
  }

  return 0;
}

/*Used by threadpool_shutdown to free data structures after joining worker threads*/
static int free_threadpool(struct threadpool_t *pool)
{
  if(pool == NULL)
    return -1;

  /*Free only if pool was allocated*/
  if(pool->threads) {

    free(pool->threads);
    pthread_mutex_lock(&pool->lock);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
    mq_close(pool->mq);
    mq_unlink(q_name);
  }

  free(pool);
  return 0;

}

/*Function sets shutdown variable and joins all worker threads, and then calls free_threadpool() to free the data structures. Returns 0 on success and -1 on failure and 1 if shutdown is already set*/
int threadpool_shutdown(void *p, int flag)
{
  struct threadpool_t *pool = (struct threadpool_t *)p;
  int i;

  if(pthread_mutex_lock(&pool->lock) != 0) {
    return -1;
  }

  pool->shutdown = flag;

  if(flag == ABORT_SHUT) {
    pthread_mutex_unlock(&pool->lock);
    for(i = 0; i < pool->thread_count; i++) {
      pthread_cancel(pool->threads[i]);
	}
  }

  else{
    if(pthread_cond_broadcast(&pool->cond) != 0) {
      return -1;
    }

    pthread_mutex_unlock(&pool->lock);

    for(i = 0; i < pool->thread_count; i++) {
      if(pthread_join(pool->threads[i], NULL) != 0) {
	return -1;
      }
    }
  }

  if(free_threadpool(pool) == -1) {
    return -1;
  }

  return 0;
}

/*Thread worker function which receives from the message queue and executes the job function*/
static void *thr_fn(void *arg)
{
  struct threadpool_t *pool = (struct threadpool_t *)arg;
  struct threadpool_job j;

  while(1) {
    if(pthread_mutex_lock(&pool->lock) != 0) {
      pthread_exit(NULL);
    }

    /*Wait on condition variable and check for spurious wakeups*/
    while(pool->pending_count == 0 && pool->shutdown == 0) {
      pthread_cond_wait(&pool->cond, &pool->lock);
    }

    /*If thread was woken up by the shutdown condition*/
    if((pool->shutdown == GRACEFUL_SHUT && pool->pending_count == 0) || pool->shutdown == IMM_SHUT || pool->shutdown == ABORT_SHUT){
      break;
    }


    /*Pull job from the message queue*/
    if(mq_receive(pool->mq, (char *)&j, sizeof(j), NULL) == -1) {
      pthread_mutex_unlock(&pool->lock);
      pthread_exit(NULL);
    }

    pool->pending_count--;
    pthread_mutex_unlock(&pool->lock);
    (*(j.func))(j.arg);

  };

  pthread_mutex_unlock(&pool->lock);

  pthread_exit(NULL);


}
