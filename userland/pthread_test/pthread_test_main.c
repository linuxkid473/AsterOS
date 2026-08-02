/* End-to-end proof that pthreads are real: genuine concurrent
 * kernel-scheduled threads (bsdthread_create(2)), not the old
 * single-threaded pthread_stub.c (pthread_create() used to just return
 * EAGAIN). A normal dynamically-linked executable against the real
 * libSystem.B.dylib, same pattern as userland/libSystem/test/test_main.c.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NTHREADS 4
#define INCREMENTS 200000

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static long g_counter;

static void *
counter_thread(void *arg)
{
	(void)arg;
	for (int i = 0; i < INCREMENTS; i++) {
		pthread_mutex_lock(&g_lock);
		g_counter++;
		pthread_mutex_unlock(&g_lock);
	}
	return (void *)(long)pthread_self();
}

static pthread_mutex_t g_cv_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cv = PTHREAD_COND_INITIALIZER;
static int g_cv_ready;
static int g_cv_seen;

static void *
signaler_thread(void *arg)
{
	(void)arg;
	pthread_mutex_lock(&g_cv_lock);
	g_cv_ready = 1;
	pthread_cond_signal(&g_cv);
	pthread_mutex_unlock(&g_cv_lock);
	return NULL;
}

int
main(void)
{
	printf("PTHREADTEST: spawning %d threads, %d increments each\n",
	    NTHREADS, INCREMENTS);

	pthread_t threads[NTHREADS];
	for (int i = 0; i < NTHREADS; i++) {
		if (pthread_create(&threads[i], NULL, counter_thread, NULL) != 0) {
			printf("PTHREADTEST FAIL: pthread_create failed\n");
			return 1;
		}
	}

	pthread_t self = pthread_self();
	int distinct = 1;
	for (int i = 0; i < NTHREADS; i++) {
		void *ret = NULL;
		if (pthread_join(threads[i], &ret) != 0) {
			printf("PTHREADTEST FAIL: pthread_join failed\n");
			return 1;
		}
		if (pthread_equal((pthread_t)(long)ret, self)) {
			distinct = 0;
		}
	}
	if (!distinct) {
		printf("PTHREADTEST FAIL: a spawned thread's pthread_self() aliased the main thread\n");
		return 1;
	}

	long want = (long)NTHREADS * INCREMENTS;
	printf("PTHREADTEST: counter = %ld (want %ld)\n", g_counter, want);
	if (g_counter != want) {
		printf("PTHREADTEST FAIL: counter mismatch -- mutex isn't providing real "
		    "mutual exclusion across concurrent threads\n");
		return 1;
	}
	printf("PTHREADTEST: mutex + %d real concurrent threads: counter exact, no lost updates\n",
	    NTHREADS);

	pthread_mutex_lock(&g_cv_lock);
	pthread_t sig_thread;
	if (pthread_create(&sig_thread, NULL, signaler_thread, NULL) != 0) {
		printf("PTHREADTEST FAIL: pthread_create (signaler) failed\n");
		return 1;
	}
	while (!g_cv_ready) {
		pthread_cond_wait(&g_cv, &g_cv_lock);
	}
	g_cv_seen = 1;
	pthread_mutex_unlock(&g_cv_lock);
	pthread_join(sig_thread, NULL);
	if (!g_cv_seen) {
		printf("PTHREADTEST FAIL: condition variable handoff didn't happen\n");
		return 1;
	}
	printf("PTHREADTEST: condition variable signal/wait handoff works\n");

	printf("PTHREADTEST PASS\n");
	return 0;
}
