#include "ppos.h"

#include <stdio.h>

void ppos_init() {
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf(stdout, 0, _IONBF, 0);
    //salva o contexto da main?
}

// Inicializa uma nova tarefa. Retorna um ID> 0 ou erro.
int task_init(task_t *task, void (*start_func)(void *), void *arg){
    //preenche os campos da struct
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id (){
 //return curr->id
}

// Termina a tarefa corrente com um valor de status de encerramento
void task_exit (int exit_code){

}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task){
    //curr = task
    //executa a função do contexto ao trocar de contexto
}