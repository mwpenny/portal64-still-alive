
#ifndef __audio__
#define __audio__

#include <ultra64.h>
#include <libaudio.h>
#include <sched.h>

#define MAX_UPDATES             64
#define MAX_EVENTS              128
#define AUDIO_HEAP_SIZE         300000

#define MAX_VOICES              64
#define EXTRA_SAMPLES           80
#define NUM_OUTPUT_BUFFERS      3      /* Need three of these */
#define OUTPUT_RATE             44100
#define MAX_AUDIO_MESGS         32
#define QUIT_MSG                10

#define DMA_BUFFER_LENGTH       0x800  /* Larger buffers result in fewer DMA' but more  */
                                       /* memory being used.  */


#define NUM_ACMD_LISTS          2      /* two lists used by this example                */
#define MAX_RSP_CMDS            4096   /* max number of commands in any command list.   */
                                       /* Mainly dependent on sequences used            */

#define NUM_DMA_BUFFERS         24     /* max number of dma buffers needed.             */
                                       /* Mainly dependent on sequences and sfx's       */

#define NUM_DMA_MESSAGES        32     /* The maximum number of DMAs any one frame can  */
                                       /* have.                                         */

#define FRAME_LAG               1      /* The number of frames to keep a dma buffer.    */
                                       /* Increasing this number causes buffers to not  */
                                       /* be deleted as quickly. This results in fewer  */
                                       /* DMA's but you need more buffers.              */


#define AUDIO_STACKSIZE         0x2000

#define MAX_SEQ_LENGTH  20000


typedef struct {
    u32       outputRate;
    u32       framesPerField;
    u32       maxACMDSize;
} amConfig;


void amCreateAudioMgr(ALSynConfig *c, OSPri priority, amConfig *amc, int fps);
void initAudio(int fps);

extern u64        audYieldBuf[];
extern u8*        gAudioHeapBuffer;
extern ALHeap     gAudioHeap;

#endif

