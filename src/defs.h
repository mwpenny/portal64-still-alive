
#ifndef __DEFS_H__
#define __DEFS_H__

#define	STACKSIZEBYTES	0x2000

#ifdef _LANGUAGE_C

#define MAX_FRAME_BUFFER_MESGS	8

#define HIGH_RES    0

#define INIT_PRIORITY		10
#define GAME_PRIORITY		10
#define AUDIO_PRIORITY		12
#define SCHEDULER_PRIORITY	13
#define NUM_FIELDS      1 

#define LEVEL_SEGMENT 2
#define DYNAMIC_MODEL_SEGMENT 3

#define DMA_QUEUE_SIZE  200

#ifndef SCENE_SCALE
#define SCENE_SCALE 256
#endif

#define MAX_DYNAMIC_OBJECTS     32

#define MAX_RENDER_COUNT        256

#define SIMPLE_CONTROLLER_MSG	    (5)

#define PRINTF(a) 

#endif /* _LANGUAGE_C */

#endif
