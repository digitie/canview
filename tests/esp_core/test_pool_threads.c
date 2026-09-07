/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_esp_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef CRITICAL_SECTION test_mutex_t;
#else
#include <pthread.h>
#include <sched.h>
typedef pthread_mutex_t test_mutex_t;
#endif
#define CHECK(value)                                                                               \
    do                                                                                             \
    {                                                                                              \
        if (!(value))                                                                              \
        {                                                                                          \
            (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #value);                      \
            abort();                                                                               \
        }                                                                                          \
    } while (0)
#define THREADS (4U)
#define ROUNDS (2000U)
typedef struct
{
    canview_esp_pool_t *pool;
    unsigned id;
    unsigned completed;
} worker_t;

static void lock_enter(void *context)
{
#if defined(_WIN32)
    EnterCriticalSection(context);
#else
    CHECK(pthread_mutex_lock(context) == 0);
#endif
}
static void lock_leave(void *context)
{
#if defined(_WIN32)
    LeaveCriticalSection(context);
#else
    CHECK(pthread_mutex_unlock(context) == 0);
#endif
}
static void yield_thread(void)
{
#if defined(_WIN32)
    (void)SwitchToThread();
#else
    CHECK(sched_yield() == 0);
#endif
}
static void run_worker(worker_t *worker)
{
    uint8_t input[CANVIEW_ESP_POOL_BYTES];
    uint8_t output[CANVIEW_ESP_POOL_BYTES];
    for (unsigned round = 0U; round < ROUNDS; ++round)
    {
        for (size_t byte = 0U; byte < sizeof(input); ++byte)
        {
            input[byte] = (uint8_t)((worker->id * 71U + round + byte) & 0xffU);
        }
        canview_esp_pool_token_t token = {0};
        canview_status_t status;
        do
        {
            status = canview_esp_pool_acquire(worker->pool, input, sizeof(input), &token);
            CHECK(status == CANVIEW_OK || status == CANVIEW_RESOURCE_BUSY);
            if (status == CANVIEW_RESOURCE_BUSY)
            {
                yield_thread();
            }
        } while (status != CANVIEW_OK);
        yield_thread();
        size_t length = 0U;
        CHECK(canview_esp_pool_copy(worker->pool, token, output, sizeof(output), &length) ==
              CANVIEW_OK);
        CHECK(length == sizeof(input) && memcmp(input, output, length) == 0);
        canview_esp_pool_stats_t stats = {0};
        CHECK(canview_esp_pool_stats(worker->pool, &stats) == CANVIEW_OK);
        CHECK(stats.used > 0U && stats.used <= 2U && stats.high_water <= 2U);
        CHECK(canview_esp_pool_release(worker->pool, token) == CANVIEW_OK);
        CHECK(canview_esp_pool_release(worker->pool, token) == CANVIEW_STALE);
        ++worker->completed;
    }
}
#if defined(_WIN32)
static DWORD WINAPI thread_entry(LPVOID context)
{
    run_worker(context);
    return 0U;
}
#else
static void *thread_entry(void *context)
{
    run_worker(context);
    return NULL;
}
#endif
int main(void)
{
    test_mutex_t mutex;
#if defined(_WIN32)
    InitializeCriticalSection(&mutex);
    HANDLE threads[THREADS];
#else
    CHECK(pthread_mutex_init(&mutex, NULL) == 0);
    pthread_t threads[THREADS];
#endif
    canview_esp_pool_t pool = {0};
    const canview_esp_pool_port_t port = {lock_enter, lock_leave, &mutex};
    CHECK(canview_esp_pool_init(&pool, 2U, &port) == CANVIEW_OK);
    worker_t workers[THREADS];
    for (unsigned index = 0U; index < THREADS; ++index)
    {
        workers[index] = (worker_t){&pool, index, 0U};
#if defined(_WIN32)
        threads[index] = CreateThread(NULL, 0U, thread_entry, &workers[index], 0U, NULL);
        CHECK(threads[index] != NULL);
#else
        CHECK(pthread_create(&threads[index], NULL, thread_entry, &workers[index]) == 0);
#endif
    }
    for (unsigned index = 0U; index < THREADS; ++index)
    {
#if defined(_WIN32)
        CHECK(WaitForSingleObject(threads[index], 20000U) == WAIT_OBJECT_0);
        CHECK(CloseHandle(threads[index]) != 0);
#else
        CHECK(pthread_join(threads[index], NULL) == 0);
#endif
        CHECK(workers[index].completed == ROUNDS);
    }
    canview_esp_pool_stats_t stats = {0};
    CHECK(canview_esp_pool_stats(&pool, &stats) == CANVIEW_OK);
    CHECK(stats.used == 0U && stats.high_water > 0U && stats.high_water <= 2U);
    CHECK(stats.stale == THREADS * ROUNDS);
#if defined(_WIN32)
    DeleteCriticalSection(&mutex);
#else
    CHECK(pthread_mutex_destroy(&mutex) == 0);
#endif
    (void)puts("PASS: 4 host threads, 8000 owned payloads, 2 slots; not target scheduling proof");
    return 0;
}
