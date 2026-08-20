#include "threads_libultra.h"

#include "system/defs.h"

#include <ultra64.h>

#define IDLE_STACK_SIZE_BYTES 128
#define GAME_STACK_SIZE_BYTES 4096

static OSThread sIdleThread;
static OSThread sGameThread;

u64 bootStack[BOOT_STACK_SIZE_BYTES / sizeof(u64)];
static u64 sIdleThreadStack[IDLE_STACK_SIZE_BYTES / sizeof(u64)];
static u64 sGameThreadStack[GAME_STACK_SIZE_BYTES / sizeof(u64)];

extern int main();

static void gameThreadEntry(void* arg) {
    main();
}

static void idleThreadEntry(void* arg) {
    osCreateThread(
        &sGameThread,
        GAME_THREAD_ID,
        gameThreadEntry,
        NULL,
        sGameThreadStack + (GAME_STACK_SIZE_BYTES / sizeof(u64)),
        (OSPri)GAME_THREAD_PRIORITY
    );

    osStartThread(&sGameThread);

    while (1) {
        // Idle
    }
}

void boot() {
    osInitialize();

    osCreateThread(
        &sIdleThread,
        IDLE_THREAD_ID,
        idleThreadEntry,
        NULL,
        sIdleThreadStack + (IDLE_STACK_SIZE_BYTES / sizeof(u64)),
        (OSPri)OS_PRIORITY_IDLE
    );

    osStartThread(&sIdleThread);
}
