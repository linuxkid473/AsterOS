/* End-to-end proof that libdispatch's v1 core works: serial-queue FIFO
 * ordering, dispatch_sync/dispatch_once/dispatch_after/dispatch_group,
 * and -- in the same spirit as userland/pthread_test/pthread_test_main.c's
 * 4-thread exact-counter check -- concurrent dispatch_async onto a global
 * queue under a dispatch_semaphore_t landing on an exact count, proving
 * the worker pool's scheduling invariant (a serial queue never double-
 * drains) and the semaphore actually serializes concurrent workers.
 */
#include <dispatch/dispatch.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHECK(cond, msg) \
	do { \
		if (!(cond)) { \
			printf("DISPATCHTEST FAIL: %s\n", msg); \
			exit(1); \
		} \
	} while (0)

static int g_order[5];
static int g_order_idx;

static void
test_serial_order(void)
{
	dispatch_queue_t q = dispatch_queue_create("com.asteros.dispatchtest.serial", DISPATCH_QUEUE_SERIAL);
	dispatch_semaphore_t done = dispatch_semaphore_create(0);
	g_order_idx = 0;
	for (int i = 0; i < 5; i++) {
		dispatch_async(q, ^{
			g_order[g_order_idx++] = i;
			if (i == 4) {
				dispatch_semaphore_signal(done);
			}
		});
	}
	dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
	for (int i = 0; i < 5; i++) {
		CHECK(g_order[i] == i, "serial queue FIFO ordering");
	}
	dispatch_release(done);
	dispatch_release(q);
	printf("DISPATCHTEST: serial queue FIFO ordering ok\n");
}

static void
test_sync(void)
{
	dispatch_queue_t q = dispatch_queue_create("com.asteros.dispatchtest.sync", DISPATCH_QUEUE_SERIAL);
	__block int val = 0;
	dispatch_sync(q, ^{
		val = 42;
	});
	CHECK(val == 42, "dispatch_sync ran synchronously (and __block byref capture worked)");
	dispatch_release(q);
	printf("DISPATCHTEST: dispatch_sync ok\n");
}

static int g_once_count;
static dispatch_once_t g_once_pred;

static void *
once_worker(void *arg)
{
	(void)arg;
	dispatch_once(&g_once_pred, ^{
		g_once_count++;
	});
	return NULL;
}

static void
test_once(void)
{
	pthread_t threads[8];
	for (int i = 0; i < 8; i++) {
		CHECK(pthread_create(&threads[i], NULL, once_worker, NULL) == 0, "pthread_create for dispatch_once test");
	}
	for (int i = 0; i < 8; i++) {
		pthread_join(threads[i], NULL);
	}
	CHECK(g_once_count == 1, "dispatch_once ran exactly once across concurrent threads");
	printf("DISPATCHTEST: dispatch_once ok\n");
}

struct asyncf_ctx {
	int *val;
	dispatch_semaphore_t done;
};

static void
asyncf_fn(void *context)
{
	struct asyncf_ctx *c = context;
	(*c->val)++;
	dispatch_semaphore_signal(c->done);
}

static void
test_async_f(void)
{
	int val = 0;
	dispatch_semaphore_t done = dispatch_semaphore_create(0);
	struct asyncf_ctx ctx = { &val, done };
	dispatch_async_f(dispatch_get_global_queue(0, 0), &ctx, asyncf_fn);
	dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
	CHECK(val == 1, "dispatch_async_f ran");
	dispatch_release(done);
	printf("DISPATCHTEST: dispatch_async_f ok\n");
}

static void
test_after(void)
{
	dispatch_semaphore_t sem = dispatch_semaphore_create(0);
	struct timespec t0;
	clock_gettime(CLOCK_REALTIME, &t0);
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 200000000ll), dispatch_get_global_queue(0, 0), ^{
		dispatch_semaphore_signal(sem);
	});
	long r = dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 2000000000ll));
	CHECK(r == 0, "dispatch_after fired within timeout");
	struct timespec t1;
	clock_gettime(CLOCK_REALTIME, &t1);
	long elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec - t0.tv_nsec) / 1000000;
	CHECK(elapsed_ms >= 150, "dispatch_after did not fire early");
	dispatch_release(sem);
	printf("DISPATCHTEST: dispatch_after ok (elapsed %ldms)\n", elapsed_ms);
}

static void
test_notify(void)
{
	dispatch_group_t grp = dispatch_group_create();
	dispatch_semaphore_t done = dispatch_semaphore_create(0);
	dispatch_group_enter(grp);
	dispatch_async(dispatch_get_global_queue(0, 0), ^{
		dispatch_group_leave(grp);
	});
	dispatch_group_notify(grp, dispatch_get_global_queue(0, 0), ^{
		dispatch_semaphore_signal(done);
	});
	long r = dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, 2000000000ll));
	CHECK(r == 0, "dispatch_group_notify fired");
	dispatch_release(done);
	dispatch_release(grp);
	printf("DISPATCHTEST: dispatch_group_notify ok\n");
}

#define NTASKS 4
#define INCREMENTS 50000

static long g_counter;
static dispatch_semaphore_t g_counter_sem;

static void
test_group_concurrency(void)
{
	dispatch_queue_t q = dispatch_get_global_queue(0, 0);
	dispatch_group_t grp = dispatch_group_create();
	g_counter = 0;
	g_counter_sem = dispatch_semaphore_create(1);
	for (int t = 0; t < NTASKS; t++) {
		dispatch_group_async(grp, q, ^{
			for (int i = 0; i < INCREMENTS; i++) {
				dispatch_semaphore_wait(g_counter_sem, DISPATCH_TIME_FOREVER);
				g_counter++;
				dispatch_semaphore_signal(g_counter_sem);
			}
		});
	}
	long r = dispatch_group_wait(grp, dispatch_time(DISPATCH_TIME_NOW, 30000000000ll));
	CHECK(r == 0, "dispatch_group_wait didn't time out");
	long want = (long)NTASKS * INCREMENTS;
	printf("DISPATCHTEST: counter = %ld (want %ld)\n", g_counter, want);
	CHECK(g_counter == want, "concurrent dispatch_async under a semaphore: counter exact, no lost updates");
	dispatch_release(g_counter_sem);
	dispatch_release(grp);
	printf("DISPATCHTEST: dispatch_group + concurrent queue ok\n");
}

int
main(void)
{
	printf("DISPATCHTEST: starting\n");
	test_serial_order();
	test_sync();
	test_once();
	test_async_f();
	test_after();
	test_notify();
	test_group_concurrency();
	printf("DISPATCHTEST PASS\n");
	return 0;
}
