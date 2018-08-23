/*
Lucas Stefan Abe - 8531612
Mathias Van Sluys Menck - 4343470
*/

#include <stdio.h> /* fopen, fclose, fscanf, fprintf*/
#include <stdlib.h> /* malloc */
#include <string.h> /* strcmp, strcpy*/
#include <math.h> /* pow */ 
#include <pthread.h> /* pthread_t, pthread_join, pthread_create, pthread_exit */ 
#include <time.h> /* clock_t, clock, CLOCKS_PER_SEC */
#include <sched.h> /* sched_getcpu */

#define QUANTUM 0.01

struct list {
  char name[110]; // nome do processo
  double t0; // tempo de chegada
  double tInit; // tempo em que o processo começa a rodar
  double tf; // tempo em que o processo termina
  double dt; // duração do processo
  double deadline; // processo deve terminar antes dessa deadline
  double quantum;
  int finish; // variável que marca se terminou o processo, para evitar comparação de float
  pthread_t tr; 
  struct list* next;
};
typedef struct list list;
typedef struct list* node;
int d = 0, cmc = 0, indice, linha = 1; // cmc é o contador de mudança de contexto 
clock_t global_time; // global_time marca o inicio da contagem do tempo
node head1; // cabeça da lista de processos que ainda não chegaram
node head2; // cabeça da lista de processos que já chegaram (multiFilas não usa)
node head[5]; // usado para o escalonador de multiplas filas

/* função que insere um node no final da lista lista com cabeça head*/
void insert_end (node data, node head);

void insert_mid (node data, node head);

/* função que insere um node no começo da lista com cabeça head*/
void insert_begin (node data, node head);

/* função que remove um node da lista com cabeça head e o retorna*/
node remove_node (node data, node head);

int isEmpty (node head[]) {
  int i;
  for (i = 0; i < 5; i++) 
    if (head[i]->next != NULL) return 0;
  return 1;
}

void FCFS (FILE* out);

void SRTN (FILE* out);

void multiFilas (FILE* out);

/* Função da thread para o escalonador FCFS*/
void *threadFunction1 ();

/* Função da thread para o escalonador SRTN*/
void *threadFunction2 ();

/* Função da thread para o escalonador de múltiplas filas*/
void *threadFunction3 ();


int main(int numargs, char **args) {
  int escalonador = atoi(args[1]);
  FILE *in, *out;
  char name[110];
  double t0, dt, deadline; 

  in = fopen(args[2], "r");
  out = fopen(args[3], "w");

  /* inicializando a lista de processos que não chegaram e chegaram*/
  head1 = malloc (sizeof (list));
  head1->next = NULL;
  head2 = malloc (sizeof (list));
  head2->next = NULL;

  /* fazendo leitura dos traces, e colocando na lista de processos que não chegaram*/
  while (fscanf(in, "%lf %s %lf %lf", &t0, name, &dt, &deadline) != EOF) {
    node data = malloc (sizeof(list));
    data->t0 = t0;
    strcpy (data->name, name);
    data->dt = dt;
    data->deadline = deadline;
    data->finish = 0;
    insert_end (data, head1);
  }

  fclose (in);

  if(numargs > 4 && strcmp(args[4], "d")==0) d=1;
  global_time = 0.0;

  /* Iniciando o simulador de processos */
  if (escalonador == 1) FCFS (out);
  else if (escalonador == 2) SRTN (out);
  else if (escalonador == 3) multiFilas (out);
  fprintf (out, "Quantidade de mudanças de contexo: %d\n", cmc);
  fclose(out);
  
  return 0;
}

void insert_end (node data, node head) {
  node aux = head;
  while (aux->next != NULL) aux = aux->next;
  aux->next = data;
  data->next = NULL;
}

void insert_begin (node data, node head) {
  node aux = head->next;
  head->next = data;
  data->next = aux;
}

void insert_mid (node data, node head) {
  node next, aux = head;
  while (aux->next != NULL && data->dt > aux->next->dt) aux = aux->next;
  next = aux->next;
  aux->next = data;
  data->next = next;
}

node remove_node (node data, node head) {
  node aux = head;
  while (aux->next != NULL) {
    if (aux->next == data) {
      aux->next = data->next;
      return data;
    }
    aux = aux->next;
  }
  fprintf (stderr, "Erro ao retirar nó da lista !!\n");
  return NULL;
}

void FCFS (FILE *out) {
  node aux;
  double time; // time é o instante de tempo
  int ret;

  global_time = clock ();
  while (head1->next != NULL || head2->next != NULL) {
    time = (double)(clock () - global_time)/CLOCKS_PER_SEC;
    if (head1->next != NULL && time > head1->next->t0) {
      aux = remove_node (head1->next, head1);
      insert_end (aux, head2);
      if(d) {
        fprintf(stderr, "Chegou um processo, trace: %lf %s %lf %lf.\n", aux->t0, aux->name, aux->dt, aux->deadline);
      }
    }
    if (head2->next != NULL) {
      aux = head2->next; // primeiro processo da lista
      
      /* Criando a thread*/
      ret = pthread_create( &aux->tr, NULL, threadFunction1, NULL);
      /* Fazendo o programa esperar a thread ser executada*/
      pthread_join (aux->tr, NULL);

      aux->tf = (double)(clock () - global_time)/CLOCKS_PER_SEC;
      fprintf(out, "%s %lf %lf\n", aux->name, aux->tf, aux->tf - aux->t0);
      if(d) fprintf(stderr, "Terminou um processo, saída: %s %lf %lf\n", aux->name, aux->tf, aux->tf - aux->t0);

      /*removendo o primeiro processo da lista*/
      head2->next = aux->next;
      free (aux);
    }
  }
}

void SRTN (FILE *out) {
  node aux, aux2;
  double time; // time é o instante de tempo
  int ret;

  global_time = clock ();
  while (head1->next != NULL || head2->next != NULL) {
    time = (double)(clock () - global_time)/CLOCKS_PER_SEC;
    if (head1->next != NULL && time > head1->next->t0) {
      aux = remove_node (head1->next, head1);
      insert_mid (aux, head2);
      if(d) {
        fprintf(stderr, "Chegou um processo, trace: %lf %s %lf %lf.\n", aux->t0, aux->name, aux->dt, aux->deadline);
      }
    }
    if (head2->next != NULL) {
      aux = head2->next; // primeiro processo da lista
      
      /* Criando a thread*/
      ret = pthread_create( &aux->tr, NULL, threadFunction2, NULL);

      /* Fazendo o programa esperar a thread ser executada*/
      pthread_join (aux->tr, NULL);

      if (aux->finish) {
        aux->tf = (double)(clock () - global_time)/CLOCKS_PER_SEC;
        fprintf(out, "%s %lf %lf\n", aux->name, aux->tf, aux->tf - aux->t0);
        if(d) fprintf(stderr, "Terminou um processo, saída: %s %lf %lf\n", aux->name, aux->tf, aux->tf - aux->t0);

        /*removendo o processo que terminou*/
        head2->next = aux->next;
        free (aux);
      }
      else {
          aux = remove_node (aux, head2);
          insert_mid (aux, head2);
          aux2 = remove_node (head1->next, head1);
          insert_begin (aux2, head2);
      }
    }
  }
}

void multiFilas (FILE *out) {
  node aux;
  double time; // time é o instante de tempo
  int ret, i;
  for (i = 0; i < 5; i++) {
    head[i] = malloc (sizeof (list));
    head[i]->next = NULL;
  }

  global_time = clock ();
  while (head1->next != NULL || !isEmpty (head)) {
    time = (double)(clock () - global_time)/CLOCKS_PER_SEC;
    if (head1->next != NULL && time > head1->next->t0) {
      aux = remove_node (head1->next, head1);
      aux->quantum = QUANTUM;
      insert_end (aux, head[0]);
      if(d) {
        fprintf(stderr, "Chegou um processo, trace: %lf %s %lf %lf.\n", aux->t0, aux->name, aux->dt, aux->deadline);
      }
    }
    for (indice = 0; indice < 5; indice++) { 
      if (head[indice]->next != NULL) {
        aux = head[indice]->next; // primeiro processo da lista
    
        /* Criando a thread*/
        ret = pthread_create( &aux->tr, NULL, threadFunction3, NULL);
        /* Fazendo o programa esperar a thread ser executada*/
        pthread_join (aux->tr, NULL);

        if (aux->finish)  {
          /*removendo o primeiro processo da lista*/
          aux->tf = (double)(clock () - global_time)/CLOCKS_PER_SEC;
          fprintf(out, "%s %lf %lf\n", aux->name, aux->tf, aux->tf - aux->t0);
          if(d) fprintf(stderr, "Terminou um processo, saída: %s %lf %lf\n", aux->name, aux->tf, aux->tf - aux->t0);

          head[indice]->next = aux->next;
          free (aux);
        }
        else {
          aux->quantum = QUANTUM*((double)pow(2, indice+1));
          /* se o processo não terminou, ele vai para o final da fila seguinte*/
          head[indice]->next = aux->next;
          if (indice <= 3) insert_end (aux, head[indice+1]);
          else insert_end (aux, head[4]);
        }
        break;
      }
    }
  }
}

void* threadFunction1 () {
  int cont = 0;
  double time;
  node aux = head2->next;
  if(d) fprintf(stderr, "Processo %s começou a usar a CPU %d\n", aux->name, sched_getcpu());
  aux->tInit = ((double)clock())/ CLOCKS_PER_SEC;
  while (1) { 
    time = (double)clock ()/CLOCKS_PER_SEC - aux->tInit;
    if (time > aux->dt) {
      aux->finish = 1;
      if(d) fprintf(stderr, "Processo %s deixou de usar a CPU %d\n", aux->name, sched_getcpu());
      pthread_exit (NULL);
    } 
    cont++;
  }
}

void* threadFunction2 () {
  int cont = 0;
  double time, exec_time; // instante de tempo atual
  node aux1 = head2->next; //nó do processo atual
  node aux2; // variável auxiliar de nó
  if(d) fprintf(stderr, "Processo %s começou a usar a CPU %d\n", aux1->name, sched_getcpu());

  aux1->tInit = clock();
  
  while (1) {
    time = (double) (clock () - global_time) / CLOCKS_PER_SEC; 
    exec_time = (clock () - aux1->tInit)/CLOCKS_PER_SEC;
    if (head1->next != NULL) {
      if (time > head1->next->t0) { // se chegou um novo processo no meio da execução
        if (head1->next->dt < aux1->dt - exec_time) { // se o tempo de execução do novo processo for menor ocorre preempção
          aux1->dt -= exec_time;
          cmc++;
          if(d) {
            fprintf(stderr, "Processo %s deixou de usar a CPU %d\n", aux1->name, sched_getcpu());
            fprintf(stderr, "Mudança de contexo, total: %d\n", cmc);
          }
          pthread_exit(NULL); 
        }
        else {          
          aux2 = remove_node (head1->next, head1);
          insert_mid (aux2, head2->next);
          
        } 
      }
    }
    if (exec_time > aux1->dt) {
        aux1->finish = 1;
        if(d) fprintf(stderr, "Processo %s deixou de usar a CPU %d\n", aux1->name, sched_getcpu());
        pthread_exit (NULL);
    }

    cont++;
  }
}

void* threadFunction3 () {
  int cont;
  double time;
  node aux = head[indice]->next;
  if(d) fprintf(stderr, "Processo %s começou a usar a CPU %d\n", aux->name, sched_getcpu());
  aux->tInit = ((double)clock())/ CLOCKS_PER_SEC;
  while (1) { 
    time = (double)clock ()/CLOCKS_PER_SEC - aux->tInit;
    if (time > aux->dt) {
      aux->finish = 1;
      if(d) fprintf(stderr, "Processo %s deixou de usar a CPU %d\n", aux->name, sched_getcpu());
      pthread_exit (NULL);
    }
    else if (time > aux->quantum) {
      cmc++;
      aux->dt -= aux->quantum;
      if(d) {
        fprintf(stderr, "Processo %s deixou de usar a CPU %d\n", aux->name, sched_getcpu());
        fprintf(stderr, "Mudança de contexo, total: %d\n", cmc);
      }
      pthread_exit (NULL);
    } 
    cont++;
  } 
}
