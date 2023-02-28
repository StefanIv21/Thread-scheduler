#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include "so_scheduler.h"

typedef struct thread {
	pthread_t tid;
	unsigned int time;
	unsigned int priority;
	so_handler *handler;
	unsigned int io;
	struct thread *urm_priority;
	struct thread *urm_threads;
	struct thread *urm_free;
	sem_t sem;
} thread;

typedef struct {
	unsigned int events;
	unsigned int time_quantum;
	thread *running;
} scheduler;

static scheduler *s;
static int count;
static int alloc;
static  int nr_blocked;
static thread **blocked_queue; 
static thread *priority_queue;
static thread *threads_queue;
static thread *free_queue;


/*
	Adaug un element in coada in functie de prioritate
*/
void add_queue(thread *nou)
{
	count++;
	nou->urm_priority = NULL;
	if (priority_queue == NULL) {
		priority_queue = nou;
		nou = priority_queue;
		return;
	}
	if (priority_queue->priority < nou->priority) {
		nou->urm_priority = priority_queue;
		priority_queue = nou;
		return;
	}
	thread *start = priority_queue;

	while (start->urm_priority != NULL && start->urm_priority->priority >= nou->priority)
		start = start->urm_priority;
	nou->urm_priority = start->urm_priority;
	start->urm_priority = nou;
}
/* Extrag primul element din coada */
thread *extract(void)
{
	if (priority_queue == NULL)
		return NULL;

	thread *extract = priority_queue;

	if (priority_queue->urm_priority == NULL)
		priority_queue = NULL;
	else
		priority_queue = priority_queue->urm_priority;
	return extract;
}
/* Scot primul thread din coada*/
/*il pun in running si il pornesc */
void switchthread(void)
{
	int err;

	count--;
	thread *new_thread = extract();

	s->running = new_thread;
	new_thread->time = s->time_quantum;

	err = sem_post(&new_thread->sem);
	if (err != 0)
		exit(err);
}
/* se determina contextul in care se va executa thredul */
void *start_thread(void *arg)
{
	/* thred ul asteapta sa fie notificat(planificat)*/
	/*dupa ce isi termina executia, se face planificarea pentru un nou thread */
	int err;

	thread *arg_thread = (thread *)arg;

	err = sem_wait(&arg_thread->sem);
	if (err != 0)
		exit(err);

	arg_thread->handler(arg_thread->priority);

	if (count != 0)
		switchthread();
	return NULL;
}

int so_init(unsigned int time_quantum, unsigned int io)
{
	if (s != NULL)
		return -1;

	if (time_quantum <= 0 || io > SO_MAX_NUM_EVENTS)
		return -1;

	/* Aloc memorie pentru scheduler si fac initializarea pentru variabilele globale */
	s = malloc(sizeof(*s));
	if (s == NULL)
		return -1;

	s->events = io;
	s->time_quantum = time_quantum;
	s->running = NULL;
	alloc = 2;
	blocked_queue = (thread **)calloc(alloc, sizeof(thread *));
	if (blocked_queue == NULL)
		return -1;

	count = 0;
	nr_blocked = 0;
	return 0;
}
/* Planifica ce thread va rula in continuare */
void plan_scheduler(void)
{
	int err;

	thread *current;

	current = s->running;
	/* cat timp exista threduri in coada le compar cu thredul curent */
	if (count > 0) {
		/* thredul curent nu a terminat dar exista alt thread cu prioritate mai mare asa ca fac schimbul */
		if (current->priority < priority_queue->priority) {
			/* adaug thredul curent  in coada,il pun in asteptare si il notific pe cel nou ca poate porni */
			add_queue(current);
			switchthread();
			err = sem_wait(&current->sem);
			if (err != 0)
				exit(err);
			return;
		}
		/* thredul curent a terminat executia */
		if (current->time <= 0) {
			/* daca thredul curent inca are cea mai mare prioritate */
			/*il las tot pe el sa ruleze si ii cresc cuanta de timp */
			/*altfel, adaug thredul curent  in coada,il pun in asteptare si il notific pe cel nou ca poate porni */
			if (current->priority == priority_queue->priority) {
				add_queue(current);
				switchthread();
				err = sem_wait(&current->sem);
				if (err != 0)
					exit(err);
				return;
			}
			current->time = s->time_quantum;
		}
	}
}

tid_t so_fork(so_handler *func, unsigned int priority)
{
	int err;

	thread *new_thread;

	if (func == NULL || priority > SO_MAX_PRIO)
		return INVALID_TID;
	/* Aloc memorie thedului si il initializez cu datele necesare */
	new_thread = malloc(sizeof(thread));
	if (new_thread == NULL)
		return -1;
	new_thread->priority = priority;
	new_thread->handler = func;
	new_thread->tid = pthread_self();
	new_thread->time = s->time_quantum;
	/* Il adaung in listele unde nu conteaza prioritatile*/
	/*doar prezenta thredurilor(listele vor fi utizilate la functia so_end) */
	new_thread->urm_threads = threads_queue;
	threads_queue = new_thread;

	new_thread->urm_free = free_queue;
	free_queue = new_thread;

	err = sem_init(&new_thread->sem, 0, 0);
	if (err != 0)
		exit(err);
	/* Creez thredul si il adaug in coada cu prioritati */
	err = pthread_create(&new_thread->tid, NULL, &start_thread, new_thread);
	if (err != 0)
		exit(err);
	add_queue(new_thread);
	/* Daca este primul thread, il pun direct in executie, altfel apelez planificatorul */
	if (s->running == NULL)
		switchthread();
	else {
		s->running->time--;
		plan_scheduler();
	}
	return new_thread->tid;
}

void so_exec(void)
{
	/* Scad cuanta de timp si apelez planificatorul de threduri */
	s->running->time--;
	plan_scheduler();
}

void so_end(void)
{
	int err;

	if (s == NULL)
		return;
	/* Astept terminarea tuturilor thredurilor */
	while (threads_queue != NULL) {
		thread *thread = threads_queue;

		threads_queue = threads_queue->urm_threads;
		err = pthread_join(thread->tid, NULL);
		if (err != 0)
			exit(err);
	}
	/* Eliberez memoria */
	while (free_queue != NULL) {
		thread *delete = free_queue;

		free_queue = free_queue->urm_free;
		free(delete);
	}
	free(blocked_queue);
	free(s);
	s = NULL;
}

int so_wait(unsigned int io)
{
	int err;

	if (io >= s->events)
		return -1;
	s->running->time--;
	s->running->io = io;
	/* Adaug thredul in vectorul de threduri blocate Maresc dimensiunea vectorului daca este cazul */
	if (nr_blocked == alloc) {
		alloc = alloc + 2;
		blocked_queue = (thread **)realloc(blocked_queue, alloc * sizeof(thread *));
		if (blocked_queue == NULL)
			return -1;
	}
	blocked_queue[nr_blocked++] = s->running;
	/* Thredul curent il pun in asteptare si fac planificarea pentru un nou thread */
	thread *thread_curent = s->running;

	switchthread();
	err = sem_wait(&thread_curent->sem);
	if (err != 0)
		exit(err);
	return 0;
}
int so_signal(unsigned int io)
{
	s->running->time--;
	int nr = 0;

	if (io >= s->events)
		return -1;
	/* Pun in coada toate thredurile ce erau in asteptarea IO ului Aplez planificatorul de threduri */
	for (int i = 0; i < nr_blocked; i++) {
		if (blocked_queue[i]->io == io) {
			add_queue(blocked_queue[i]);
			blocked_queue[i]->io = -1;
			nr++;
			}
		}
	plan_scheduler();
	return nr;
}
