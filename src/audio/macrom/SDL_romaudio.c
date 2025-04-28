/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/*#define DEBUG_AUDIO 1
#define DEBUG_CALLBACK 1*/

#if defined(__APPLE__) && defined(__MACH__)
#  include <Carbon/Carbon.h>
#elif TARGET_API_MAC_CARBON && (UNIVERSAL_INTERFACES_VERSION > 0x0335)
#  include <Carbon.h>
#else
#  include <Sound.h> /* SoundManager interface */
#  include <Gestalt.h>
#  include <DriverServices.h>
#endif

#if !defined(NewSndCallBackUPP) && (UNIVERSAL_INTERFACES_VERSION < 0x0335)
#if !defined(NewSndCallBackProc) /* avoid circular redefinition... */
#define NewSndCallBackUPP NewSndCallBackProc
#endif
#if !defined(NewSndCallBackUPP)
#define NewSndCallBackUPP NewSndCallBackProc
#endif
#endif

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "../SDL_sysaudio.h"
#include "SDL_romaudio.h"

/* Audio driver functions */

static void Mac_CloseAudio(_THIS);
static int Mac_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void Mac_LockAudio(_THIS);
static void Mac_UnlockAudio(_THIS);

/* Audio driver bootstrap functions */


static int Audio_Available(void)
{
    return(1);
}

static void Audio_DeleteDevice(SDL_AudioDevice *device)
{
    SDL_free(device->hidden);
    SDL_free(device);
}

static SDL_AudioDevice *Audio_CreateDevice(int devindex)
{
    SDL_AudioDevice *this;

    /* Initialize all variables that we clean on shutdown */
    this = (SDL_AudioDevice *)SDL_malloc(sizeof(SDL_AudioDevice));
    if ( this ) {
        SDL_memset(this, 0, (sizeof *this));
        this->hidden = (struct SDL_PrivateAudioData *)
                SDL_malloc((sizeof *this->hidden));
    }
    if ( (this == NULL) || (this->hidden == NULL) ) {
        SDL_OutOfMemory();
        if ( this ) {
            SDL_free(this);
        }
        return(0);
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* Set the function pointers */
    this->OpenAudio   = Mac_OpenAudio;
    this->CloseAudio  = Mac_CloseAudio;
    this->LockAudio   = Mac_LockAudio;
    this->UnlockAudio = Mac_UnlockAudio;
    this->free        = Audio_DeleteDevice;

#ifdef __MACOSX__	/* Mac OS X uses threaded audio, so normal thread code is okay */
    this->LockAudio   = NULL;
    this->UnlockAudio = NULL;
#endif
    return this;
}

AudioBootStrap SNDMGR_bootstrap = {
	"sndmgr", "MacOS SoundManager 3.0",
	Audio_Available, Audio_CreateDevice
};

/*#if defined(TARGET_API_MAC_CARBON) || defined(USE_RYANS_SOUNDCODE)*/
#if TARGET_API_MAC_CARBON || defined(USE_RYANS_SOUNDCODE)
/* This works correctly on Mac OS X */

#pragma options align=power

static volatile SInt32 audio_is_locked = 0;
static volatile SInt32 need_to_mix = 0;

static UInt8  *buffer[2];
static volatile UInt32 running = 0;
static CmpSoundHeader header;
static volatile Uint32 fill_me = 0;

/* TARGET_API_MAC_CARBON || USE_RYANS_SOUNDCODE */
static void mix_buffer(SDL_AudioDevice *audio, UInt8 *buffer)
{
#ifdef DEBUG_CALLBACK
   fprintf(stderr,"mix_buffer...\n"); fflush(stderr);
#endif
   if ( ! audio->paused ) {
#ifdef __MACOSX__
        SDL_mutexP(audio->mixer_lock);
#endif
        if ( audio->convert.needed ) {
#ifdef DEBUG_CALLBACK
            fprintf(stderr,"going to audio->convert...\n"); fflush(stderr);
#endif
            audio->spec.callback(audio->spec.userdata,
                    (Uint8 *)audio->convert.buf,audio->convert.len);
#ifdef DEBUG_CALLBACK
            fprintf(stderr,"going to SDL_ConvertAudio...\n"); fflush(stderr);
#endif
            SDL_ConvertAudio(&audio->convert);
#ifdef DEBUG_CALLBACK
            fprintf(stderr,"After SDL_ConvertAudio...\n"); fflush(stderr);
#endif
            if ( audio->convert.len_cvt != audio->spec.size ) {
                /* Uh oh... probably crashes here */;
#ifdef DEBUG_CALLBACK
                fprintf(stderr,"Uh oh... probably crashes here...\n"); fflush(stderr);
#endif
            }
#ifdef DEBUG_CALLBACK
            fprintf(stderr,"After SDL_memcpy...\n"); fflush(stderr);
#endif
            SDL_memcpy(buffer, audio->convert.buf, audio->convert.len_cvt);
        } else {
#ifdef DEBUG_CALLBACK
            fprintf(stderr,"After audio->spec.callback...\n"); fflush(stderr);
#endif
            audio->spec.callback(audio->spec.userdata, buffer, audio->spec.size);
        }
#ifdef __MACOSX__
        SDL_mutexV(audio->mixer_lock);
#endif
    }

#ifdef DEBUG_CALLBACK
    fprintf(stderr,"Going to DecrementAtomic...\n"); fflush(stderr);
#endif
    DecrementAtomic((SInt32 *) &need_to_mix);
#ifdef DEBUG_CALLBACK
    fprintf(stderr,"mix_buffer done...\n"); fflush(stderr);
#endif
}

/* TARGET_API_MAC_CARBON || USE_RYANS_SOUNDCODE */
static void Mac_LockAudio(_THIS)
{
#ifdef DEBUG_CALLBACK
    fprintf(stderr,"Mac_LockAudio (RYAN)...\n"); fflush(stderr);
#endif
    IncrementAtomic((SInt32 *) &audio_is_locked);
}

/* TARGET_API_MAC_CARBON || USE_RYANS_SOUNDCODE */
static void Mac_UnlockAudio(_THIS)
{
    SInt32 oldval;

#ifdef DEBUG_CALLBACK
    fprintf(stderr,"Mac_UnlockAudio (RYAN)...\n"); fflush(stderr);
#endif
         
    oldval = DecrementAtomic((SInt32 *) &audio_is_locked);
    if ( oldval != 1 )  /* != 1 means audio is still locked. */
        return;

    /* Did we miss the chance to mix in an interrupt? Do it now. */
    if ( BitAndAtomic (0xFFFFFFFF, (UInt32 *) &need_to_mix) ) {
        /*
         * Note that this could be a problem if you missed an interrupt
         *  while the audio was locked, and get preempted by a second
         *  interrupt here, but that means you locked for way too long anyhow.
         */
        mix_buffer (this, buffer[fill_me]);
    }
}

/* TARGET_API_MAC_CARBON || USE_RYANS_SOUNDCODE */
static void callBackProc (SndChannel *chan, SndCommand *cmd_passed ) {
   UInt32 play_me;
   SndCommand cmd; 
   SDL_AudioDevice *audio = (SDL_AudioDevice *)chan->userInfo;

#ifdef AUDIO_HACK
  chan=lchan; cmd_passed=lcmd_passed;
#endif

#ifdef DEBUG_CALLBACK
   fprintf(stderr,"In callBackProc chan=%08x cmd_passed=%08x...\n",(long)chan,(long)cmd_passed); fflush(stderr);
   fprintf(stderr,"cmd->cmd=%08x param1=%08x param2=%08x\n",cmd_passed->cmd,cmd_passed->param1,cmd_passed->param2); fflush(stderr);
#endif
   if(cmd_passed->cmd!=0xd) {
     fprintf(stderr,"cmd wasn't 0xd!\n"); fflush(stderr);
     exit(0);
   }
#ifdef DEBUG_CALLBACK
   fprintf(stderr,"Going to IncrementAtomic...\n"); fflush(stderr);
#endif
   IncrementAtomic((SInt32 *) &need_to_mix);

   fill_me = cmd_passed->param2;  /* buffer that has just finished playing, so fill it */      
   play_me = ! fill_me;           /* filled buffer to play _now_ */

   if ( ! audio->enabled ) {
      return;
   }
   
   /* queue previously mixed buffer for playback. */
   header.samplePtr = (Ptr)buffer[play_me];
   cmd.cmd = bufferCmd;
   cmd.param1 = 0; 
   cmd.param2 = (long)&header;
#ifdef DEBUG_CALLBACK
   fprintf(stderr,"Going to SndDoCommand #1...\n"); fflush(stderr);
   fprintf(stderr,"buffer is %08x, header is %08x\n",(long)buffer,(long)&header); fflush(stderr);
#endif
   SndDoCommand (chan, &cmd, 0);

#ifdef DEBUG_CALLBACK
   fprintf(stderr,"Going to memset...\n"); fflush(stderr);
   fprintf(stderr,"fill_me is %08x, audio->spec.size is %08x\n",(long)fill_me,(long)audio->spec.size); fflush(stderr);
#endif
   memset (buffer[fill_me], 0, audio->spec.size);

   /*
    * if audio device isn't locked, mix the next buffer to be queued in
    *  the memory block that just finished playing.
    */
#ifdef DEBUG_CALLBACK
   fprintf(stderr,"Going to BitAndAtomic...\n"); fflush(stderr);
#endif
   if ( ! BitAndAtomic(0xFFFFFFFF, (UInt32 *) &audio_is_locked) ) {
#ifdef DEBUG_CALLBACK
      fprintf(stderr,"Going to mix_buffer...\n"); fflush(stderr);
#endif
#ifdef DEBUG_CALLBACK
      fprintf(stderr,"audio is %08x, buffer[fill_me] is %08x\n",(long)audio,(long)&buffer[fill_me]); fflush(stderr);
#endif
      mix_buffer (audio, buffer[fill_me]);
   } 
#ifdef DEBUG_CALLBACK
   fprintf(stderr,"After BitAndAtomic...\n"); fflush(stderr);
#endif

   /* set this callback to run again when current buffer drains. */
   if ( running ) {
      cmd.cmd = callBackCmd;
      cmd.param1 = 0;
      cmd.param2 = play_me;
   
#ifdef DEBUG_CALLBACK
      fprintf(stderr,"Going to SndDoCommand #2...\n"); fflush(stderr);
#endif
      SndDoCommand (chan, &cmd, 0);
   }
#ifdef DEBUG_CALLBACK
   fprintf(stderr,"callBackProc done.\n"); fflush(stderr);
#endif
}

/* TARGET_API_MAC_CARBON || USE_RYANS_SOUNDCODE */
static int Mac_OpenAudio(_THIS, SDL_AudioSpec *spec) {

   SndCallBackUPP callback;
   int sample_bits;
   int i;
   long initOptions;
      
#ifdef DEBUG_AUDIO
   fprintf(stderr,"Mac_OpenAudio (RYANS) this=%08x spec=%08x...\n",(long)this,(long)spec); fflush(stderr);
#endif
   /* Very few conversions are required, but... */
    switch (spec->format) {
        case AUDIO_S8:
        spec->format = AUDIO_U8;
        break;
        case AUDIO_U16LSB:
        spec->format = AUDIO_S16LSB;
        break;
        case AUDIO_U16MSB:
        spec->format = AUDIO_S16MSB;
        break;
    }
    SDL_CalculateAudioSpec(spec);
    
    /* initialize bufferCmd header */
    memset (&header, 0, sizeof(header));
#ifdef DEBUG_AUDIO
    fprintf(stderr,"Going to NewSndCallBackUPP callBackProc=%08x...\n",(long)callBackProc); fflush(stderr);
#endif
    callback = (SndCallBackUPP) NewSndCallBackUPP (callBackProc);
#ifdef DEBUG_AUDIO
    fprintf(stderr,"callback is %08x...\n",(long)callback); fflush(stderr);
#endif
    sample_bits = spec->size / spec->samples / spec->channels * 8;

#ifdef DEBUG_AUDIO
    fprintf(stderr,
	"Audio format 0x%x, channels = %d, sample_bits = %d, frequency = %d\n",
	spec->format, spec->channels, sample_bits, spec->freq);
#endif /* DEBUG_AUDIO */
    
    header.numChannels = spec->channels;
    header.sampleSize  = sample_bits;
    header.sampleRate  = spec->freq << 16;
    header.numFrames   = spec->samples;
    header.encode      = cmpSH;
    
    /* Note that we install the 16bitLittleEndian Converter if needed. */
    if ( spec->format == 0x8010 ) {
        header.compressionID = fixedCompression;
        header.format = k16BitLittleEndianFormat;
    }
    
#ifdef DEBUG_AUDIO
    fprintf(stderr,"Going to alloc buffers...\n"); fflush(stderr);
#endif
    /* allocate 2 buffers */
    for (i=0; i<2; i++) {
      buffer[i] = (UInt8*)malloc (sizeof(UInt8) * spec->size);
#ifdef DEBUG_AUDIO
      fprintf(stderr,"buffer[%d]=%08x\n",i,(long)buffer[i]); 
#endif
      if (buffer[i] == NULL) {
         SDL_OutOfMemory();
         return (-1);
      }
     memset (buffer[i], 0, spec->size);
   }
   
#ifdef DEBUG_AUDIO
    fprintf(stderr,"Going to alloc th_channel (%d bytes)...\n",sizeof(*th_channel)); fflush(stderr);
#endif
   /* Create the sound manager channel */
    th_channel = (SndChannelPtr)SDL_malloc(sizeof(*th_channel));
    if ( th_channel == NULL ) {
        SDL_OutOfMemory();
        return(-1);
    }
    /*memset(channel,0,sizeof(*channel));*/
#ifdef DEBUG_AUDIO
      fprintf(stderr,"th_channel=%08x &th_channel=%08x\n",(long)th_channel,(long)&th_channel); 
#endif
    if ( spec->channels >= 2 ) {
        initOptions = initStereo;
    } else {
        initOptions = initMono;
    }
    th_channel->userInfo = (long)this;
    th_channel->qLength = 128;
#ifdef DEBUG_AUDIO
    fprintf(stderr,"Going to SndNewChannel...\n"); fflush(stderr);
#endif
    if ( SndNewChannel(&th_channel, sampledSynth, initOptions, callback) != noErr ) {
#ifdef DEBUG_AUDIO
        fprintf(stderr,"Unable to create audio channel\n"); fflush(stderr);
#endif
        SDL_SetError("Unable to create audio channel");
        SDL_free(th_channel);
        th_channel = NULL;
        return(-1);
    }
   
   /* start playback */
   {
      SndCommand cmd;
      cmd.cmd = callBackCmd;
      cmd.param1 = 0;
      cmd.param2 = 0;
      running = 1;
#ifdef DEBUG_AUDIO
   fprintf(stderr,"Going to SndDoCommand &cmd=%08x...\n",&cmd); fflush(stderr);
#endif
      SndDoCommand (th_channel, &cmd, 0);
   }

#ifdef DEBUG_AUDIO
   fprintf(stderr,"Mac_OpenAudio done.\n"); fflush(stderr);
#endif
   return 1;
}

/* TARGET_API_MAC_CARBON || USE_RYANS_SOUNDCODE */
static void Mac_CloseAudio(_THIS) {
   
   int i;
   
   running = 0;
   
   if (th_channel) {
      SndDisposeChannel (th_channel, true);
      th_channel = NULL;
   }
   
    for ( i=0; i<2; ++i ) {
        if ( buffer[i] ) {
            SDL_free(buffer[i]);
            buffer[i] = NULL;
        }
    }
}

#else /* !TARGET_API_MAC_CARBON && !USE_RYANS_SOUNDCODE */

/* !TARGET_API_MAC_CARBON && !USE_RYANS_SOUNDCODE */
static void Mac_LockAudio(_THIS)
{
#ifdef DEBUG_CALLBACK
    fprintf(stderr,"Mac_LockAudio (!RYAN)...\n"); fflush(stderr);
#endif
    /* no-op. */
}

/* !TARGET_API_MAC_CARBON && !USE_RYANS_SOUNDCODE */
static void Mac_UnlockAudio(_THIS)
{
#ifdef DEBUG_CALLBACK
    fprintf(stderr,"Mac_UnlockAudio (!RYAN)...\n"); fflush(stderr);
#endif
    /* no-op. */
}


/* !TARGET_API_MAC_CARBON && !USE_RYANS_SOUNDCODE */
/* This function is called by Sound Manager when it has exhausted one of
   the buffers, so we'll zero it to silence and fill it with audio if
   we're not paused.
*/
static pascal
void sndDoubleBackProc (SndChannelPtr chan, SndDoubleBufferPtr newbuf)
{
    SDL_AudioDevice *audio = (SDL_AudioDevice *)newbuf->dbUserInfo[0];

#ifdef DEBUG_CALLBACK
    fprintf(stderr,"In sndDoubleBackProc...\n"); fflush(stderr);
#endif

    /* If audio is quitting, don't do anything */
    if ( ! audio->enabled ) {
        return;
    }
    memset (newbuf->dbSoundData, 0, audio->spec.size);
    newbuf->dbNumFrames = audio->spec.samples;
    if ( ! audio->paused ) {
#ifdef DEBUG_CALLBACK
    fprintf(stderr,"(audio not paused)\n"); fflush(stderr);
#endif
        if ( audio->convert.needed ) {
#ifdef DEBUG_CALLBACK
    fprintf(stderr,"(audio->convert.needed)\n"); fflush(stderr);
#endif
#ifdef DEBUG_CALLBACK
    fprintf(stderr,"Going to audio->spec.callback()...\n"); fflush(stderr);
#endif
            audio->spec.callback(audio->spec.userdata,
                (Uint8 *)audio->convert.buf,audio->convert.len);
#ifdef DEBUG_CALLBACK
    fprintf(stderr,"Going to SDL_ConvertAudio()...\n"); fflush(stderr);
#endif
            SDL_ConvertAudio(&audio->convert);
            if ( audio->convert.len_cvt != audio->spec.size ) {
                /* Uh oh... probably crashes here */
                fprintf(stderr,"/* Uh oh... probably crashes here */\n");
                exit(0);
            }
#ifdef DEBUG_CALLBACK
    fprintf(stderr,"Going to SDL_memcpy()...\n"); fflush(stderr);
#endif
            SDL_memcpy(newbuf->dbSoundData, audio->convert.buf,
                            audio->convert.len_cvt);
        } else {
#ifdef DEBUG_CALLBACK
    fprintf(stderr,"Going to audio->spec.callback()...\n"); fflush(stderr);
#endif
            audio->spec.callback(audio->spec.userdata,
                (Uint8 *)newbuf->dbSoundData, audio->spec.size);
        }
    }
    else {
#ifdef DEBUG_CALLBACK
    fprintf(stderr,"(audio paused)\n"); fflush(stderr);
#endif
    }
    newbuf->dbFlags    |= dbBufferReady;
#ifdef DEBUG_CALLBACK
    fprintf(stderr,"sndDoubleBackProc done.\n"); fflush(stderr);
#endif
}

/* !TARGET_API_MAC_CARBON && !USE_RYANS_SOUNDCODE */
static int DoubleBufferAudio_Available(void)
{
    int available;
    NumVersion sndversion;
    long response;

#ifdef DEBUG_AUDIO
    fprintf(stderr,"DoubleBufferAudio_Available...\n"); fflush(stderr);
#endif

    available = 0;
    sndversion = SndSoundManagerVersion();
    if ( sndversion.majorRev >= 3 ) {
        if ( Gestalt(gestaltSoundAttr, &response) == noErr ) {
            if ( (response & (1 << gestaltSndPlayDoubleBuffer)) ) {
                available = 1;
#ifdef DEBUG_AUDIO
    fprintf(stderr,"Yes, can doubleBuffer.\n"); fflush(stderr);
#endif
            }
#ifdef DEBUG_AUDIO
            else { fprintf(stderr,"No gestaltSndPlayDoubleBuffer.\n"); fflush(stderr); }
#endif
        }
    } else {
        if ( Gestalt(gestaltSoundAttr, &response) == noErr ) {
            if ( (response & (1 << gestaltHasASC)) ) {
                available = 1;
#ifdef DEBUG_AUDIO
    fprintf(stderr,"Yes, have ASC.\n"); fflush(stderr);
#endif
            }
#ifdef DEBUG_AUDIO
            else { fprintf(stderr,"No gestaltHasASC.\n");  fflush(stderr); }
#endif
        }
    }
    return(available);
}

/* !TARGET_API_MAC_CARBON && !USE_RYANS_SOUNDCODE */
static void Mac_CloseAudio(_THIS)
{
    int i;

    if ( th_channel != NULL ) {
        /* Clean up the audio channel */
        SndDisposeChannel(th_channel, true);
        th_channel = NULL;
    }
    for ( i=0; i<2; ++i ) {
        if ( th_audio_buf[i] ) {
            SDL_free(th_audio_buf[i]);
            th_audio_buf[i] = NULL;
        }
    }
}

/* !TARGET_API_MAC_CARBON && !USE_RYANS_SOUNDCODE */
static int Mac_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
    SndDoubleBufferHeader2 audio_dbh;
    int i;
    long initOptions;
    int sample_bits;
    SndDoubleBackUPP doubleBackProc;
    
#ifdef DEBUG_AUDIO
   fprintf(stderr,"Mac_OpenAudio (!RYANS) this=%08x spec=%08x...\n",(long)this,(long)spec); fflush(stderr);
#endif

    /* Check to make sure double-buffered audio is available */
    if ( ! DoubleBufferAudio_Available() ) {
        SDL_SetError("Sound manager doesn't support double-buffering");
        return(-1);
    }

    /* Very few conversions are required, but... */
    switch (spec->format) {
        case AUDIO_S8:
        spec->format = AUDIO_U8;
        break;
        case AUDIO_U16LSB:
        spec->format = AUDIO_S16LSB;
        break;
        case AUDIO_U16MSB:
        spec->format = AUDIO_S16MSB;
        break;
    }
    SDL_CalculateAudioSpec(spec);

    /* initialize the double-back header */
    SDL_memset(&audio_dbh, 0, sizeof(audio_dbh));
#ifdef DEBUG_AUDIO
    fprintf(stderr,"Going to NewSndDoubleBackProc sndDoubleBackProc=%08x...\n",(long)sndDoubleBackProc); fflush(stderr);
#endif
    doubleBackProc = NewSndDoubleBackProc (sndDoubleBackProc);
    sample_bits = spec->size / spec->samples / spec->channels * 8;
    
    audio_dbh.dbhNumChannels = spec->channels;
    audio_dbh.dbhSampleSize    = sample_bits;
    audio_dbh.dbhCompressionID = 0;
    audio_dbh.dbhPacketSize    = 0;
    audio_dbh.dbhSampleRate    = spec->freq << 16;
    audio_dbh.dbhDoubleBack    = doubleBackProc;
    audio_dbh.dbhFormat    = 0;

    /* Note that we install the 16bitLittleEndian Converter if needed. */
    if ( spec->format == 0x8010 ) {
        audio_dbh.dbhCompressionID = fixedCompression;
        audio_dbh.dbhFormat = k16BitLittleEndianFormat;
    }

#ifdef DEBUG_AUDIO
    fprintf(stderr,"Going to alloc buffers...\n"); fflush(stderr);
#endif
    /* allocate the 2 double-back buffers */
    for ( i=0; i<2; ++i ) {
        th_audio_buf[i] = SDL_calloc(1, sizeof(SndDoubleBuffer)+spec->size);
        if ( th_audio_buf[i] == NULL ) {
            SDL_OutOfMemory();
            return(-1);
        }
        th_audio_buf[i]->dbNumFrames = spec->samples;
        th_audio_buf[i]->dbFlags = dbBufferReady;
        th_audio_buf[i]->dbUserInfo[0] = (long)this;
#ifdef DEBUG_AUDIO
      fprintf(stderr,"th_audio_buf[%d]=%08x\n",i,(long)th_audio_buf[i]); 
#endif
        audio_dbh.dbhBufferPtr[i] = th_audio_buf[i];
    }

#ifdef DEBUG_AUDIO
    fprintf(stderr,"Going to alloc th_channel (%d bytes)...\n",sizeof(*th_channel)); fflush(stderr);
#endif
    /* Create the sound manager channel */
    th_channel = (SndChannelPtr)SDL_malloc(sizeof(*th_channel));
    if ( th_channel == NULL ) {
        SDL_OutOfMemory();
        return(-1);
    }
#ifdef DEBUG_AUDIO
      fprintf(stderr,"th_channel=%08x &th_channel=%08x\n",(long)th_channel,(long)&th_channel); 
#endif
    if ( spec->channels >= 2 ) {
        initOptions = initStereo;
    } else {
        initOptions = initMono;
    }
    th_channel->userInfo = 0;
    th_channel->qLength = 128;
#ifdef DEBUG_AUDIO
    fprintf(stderr,"Going to SndNewChannel...\n"); fflush(stderr);
#endif
    if ( SndNewChannel(&th_channel, sampledSynth, initOptions, 0L) != noErr ) {
#ifdef DEBUG_AUDIO
        fprintf(stderr,"Unable to create audio channel\n"); fflush(stderr);
#endif
        SDL_SetError("Unable to create audio channel");
        SDL_free(th_channel);
        th_channel = NULL;
        return(-1);
    }
 
#ifdef DEBUG_AUDIO
   fprintf(stderr,"Going to SndPlayDoubleBuffer...\n"); fflush(stderr);
#endif
    /* Start playback */
    if ( SndPlayDoubleBuffer(th_channel, (SndDoubleBufferHeaderPtr)&audio_dbh)
                                != noErr ) {
        SDL_SetError("Unable to play double buffered audio");
        return(-1);
    }

#ifdef DEBUG_AUDIO
   fprintf(stderr,"Mac_OpenAudio done.\n"); fflush(stderr);
#endif
    
    return 1;
}

#endif /* TARGET_API_MAC_CARBON || USE_RYANS_SOUNDCODE */

