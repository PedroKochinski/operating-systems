// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// Estruturas de dados internas do sistema operacional
// GRR20206144 Pedro Henrique Kochinki Silva

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto

/*define the status codes*/
typedef enum {
    READY = 3,
    RUNNING = 2,
    SUSPENDED = 1,
    TERMINATED = 0,
} status_t;

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next ;		// ponteiros para usar em filas
  int id ;				// identificador da tarefa
  ucontext_t context ;			// contexto armazenado da tarefa
  short status ;			// pronta, rodando, suspensa, ...
  int static_prio ;
  int dynamic_prio ;
  int exit_code ;
  unsigned int init_time ;
  unsigned int proc_time ;
  unsigned int activations ;
  int quantum ;
  int wake_up_time ;
  int waiting_id ;
  // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  int counter;
  int is_destroyed;
  task_t * semaphore_queue;
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  int size;
  int max;
  int msg_qtd;
  void *queue;
  int start, end;
  semaphore_t s_buffer;
  semaphore_t s_positions;
  semaphore_t s_items;

} mqueue_t ;

#endif
