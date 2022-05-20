
#include "soundarray.h"

void _bnkfPatchWaveTable(ALWaveTable *w, s32 offset, s32 table)
{
    if (w->flags)
        return;

    w->flags = 1;
    
    w->base += table;

    /* sct 2/14/96 - patch wavetable loop info based on type. */
    if (w->type == AL_ADPCM_WAVE)
    {
	w->waveInfo.adpcmWave.book  = (ALADPCMBook *)((u8 *)w->waveInfo.adpcmWave.book + offset);
	if (w->waveInfo.adpcmWave.loop)
	    w->waveInfo.adpcmWave.loop = (ALADPCMloop *)((u8 *)w->waveInfo.adpcmWave.loop + offset);
    }
    else if (w->type == AL_RAW16_WAVE)
    {
	if (w->waveInfo.rawWave.loop)
	    w->waveInfo.rawWave.loop = (ALRawLoop *)((u8 *)w->waveInfo.rawWave.loop + offset);
    }	
}

void _bnkfPatchSound(ALSound *s, s32 offset, s32 table)
{
    if (s->flags)
        return;

    s->flags = 1;
    
    s->envelope  = (ALEnvelope *)((u8 *)s->envelope + offset);
    s->keyMap    = (ALKeyMap *)((u8 *)s->keyMap + offset);

    s->wavetable = (ALWaveTable *)((u8 *)s->wavetable + offset);
    _bnkfPatchWaveTable(s->wavetable, offset, table);
}

void soundArrayInit(struct SoundArray* soundArray, void* tbl) {
    int i;
    for (i = 0; i < soundArray->soundCount; ++i) {
        soundArray->sounds[i] = (ALSound*)((u8*)soundArray->sounds[i] + (u32)soundArray);
        _bnkfPatchSound(soundArray->sounds[i], (u32)soundArray, (u32)tbl);
    }
}