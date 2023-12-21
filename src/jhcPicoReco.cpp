// jhcPicoReco.cpp : Picovoice Cheetah speech recognizer using microphone
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2023 Etaoin Systems
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 
///////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <jhcPicoReco.h>


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcPicoReco::~jhcPicoReco ()
{
  if (running > 0)
  {
    pthread_cancel(bg);
    pthread_join(bg, 0);
  }
  if (reco != NULL)
    pv_cheetah_delete(reco);
  if (mic != NULL)
    snd_pcm_close(mic);
  pthread_mutex_destroy(&vars);
  delete [] frame;
}


//= Default constructor initializes certain values.
// reads access key from environment variable
// model assumed to be in same directory as executable

jhcPicoReco::jhcPicoReco ()
{
  // make up audio sample buffer (16 bit mono)
  nsamp = pv_cheetah_frame_length();
  frame = new int16_t[nsamp];      

  // components
  mic = NULL;
  reco = NULL;

  // background thread
  pthread_mutex_init(&vars, NULL);
  running = 0;

  // noise model
  avg = 400.0;
  var = 50.0 * 50.0;

  // initial state 
  *partial = '\0';
  *result = '\0';
  active = 0;
  ready = 0;

  // parameters
  dbg = 0;
}


//= Connect to speech recognition engine using stored credentials.
// needs key file = "picovoice.key" and model file = "cheetah_params.pv"
// assumes files are in subdirectory "config" wrt given path
// returns 1 if okay, 0 or negative for problem

int jhcPicoReco::Start (const char *path)
{
  // open microphone then start up speech recognition
  if (cheetah_cfg(path) <= 0)
    return -2;
  if (open_mic() <= 0)
    return -1;

  // start background thread
  running = 1;
  pthread_create(&bg, NULL, pcm_reco, (void *) this);
  return 1;
}


//= Configure local Cheetah speech-to-text based on shared library.
// returns 1 if okay, 0 or negative for problem
// Note: this can take a while (almost 5 secs!)

int jhcPicoReco::cheetah_cfg (const char *path)
{
  char kfile[80], key[80], mfile[80];
  const char *rc, *dir = path;
  FILE *in;
  pv_status_t st;
  int n;

  // already configured
  if (reco != NULL)
    return -3;
  if (dir == NULL)
    dir = get_current_dir_name();

  // read user key from file (reuses filename string)
  sprintf(kfile, "%s/config/picovoice.key", dir);
  if ((in = fopen(kfile, "r")) == NULL)
  {
    printf(">>> Picovoice missing key: %s !\n", kfile);
    return -2;
  }
  rc = fgets(key, 80, in);
  fclose(in);

  // trim end and sanity check
  if ((n = (int) strlen(key)) > 0)
    if (strchr("\r\n", key[n - 1]) != NULL)
      if (--n > 0)
        if (strchr("\r\n", key[n - 1]) != NULL)
          n--;
  key[n] = '\0';
  if (n != 56)
  {
    printf(">>> Picovoice bad key: %s !\n", kfile);
    return -1;
  }

  // try credentials and model
  sprintf(mfile, "%s/config/cheetah_params.pv", dir);
  if ((st = pv_cheetah_init(key, mfile, 0.5f, false, &reco)) 
      == PV_STATUS_SUCCESS)
    return 1;
  printf(">>> Picovoice likely bad model: %s !\n", mfile);
  return 0;
}
 

//= Opens default audio device for streaming input to speech recognizer.
// returns 1 if okay, 0 or negative for problem

int jhcPicoReco::open_mic ()
{
  snd_pcm_hw_params_t *params;
  int rc, ok = 1;

  // can only open audio source once
  if (mic != NULL)
    return -2;

  // try to open default audio device and set parameters
  // cannot use "plughw:2,0" since pulseaudio suspends idle streams
  if ((rc = snd_pcm_open(&mic, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0) 
  {
    printf(">>> Pulseaudio error: %s !\n", strerror(-rc));
    return -1;
  }
  if (snd_pcm_hw_params_malloc(&params) >= 0)
  {
    // set for 16 bit capture
    snd_pcm_hw_params_any(mic, params); 
    snd_pcm_hw_params_set_access(mic, params, SND_PCM_ACCESS_RW_INTERLEAVED); 
    snd_pcm_hw_params_set_format(mic, params, SND_PCM_FORMAT_S16_LE); 

    // set for single channel at Picovoice specified rate
    snd_pcm_hw_params_set_rate(mic, params, pv_sample_rate(), 0); 
    snd_pcm_hw_params_set_channels(mic, params, 1);
    if ((rc = snd_pcm_hw_params(mic, params)) < 0)
    {
      printf(">>> Pulseaudio error: %s !\n", strerror(-rc));
      ok = 0;
    }

    // cleanup
    snd_pcm_hw_params_free(params);
  }

  // make sure successful
  if (ok > 0)
    return 1;
  snd_pcm_close(mic);
  mic = NULL;
  return 0;
}


//= Main background loop reads a frame of audio and sends to recognition.
// recognition results typically available 0.5-1.0 secs after end of utterance
// actual loop is not time synchronous: sometimes 0 ms, sometimes 1300 ms
// this means it cannot be used for real-time volume sensing or muting
// Note: foreground never touches "nsamp", "frame", "avg", or "var" variables

void *jhcPicoReco::pcm_reco (void *shell)
{
  jhcPicoReco *sh = (jhcPicoReco *) shell;
  char *txt = NULL;
  pv_status_t st;
  bool endp;
  int rc, len, quiet = 0; 

  while (1)
  {
    // allow clean exit
    pthread_testcancel();

    // wait for next audio frame samples and check if muted 
    if ((rc = snd_pcm_readi(sh->mic, sh->frame, sh->nsamp)) != sh->nsamp)
    {
      printf(">>> Pulseaudio error: %s !\n", strerror(-rc));
      break;
    }

    // skip recognizer when input largely silent (1.6 sec)
    // gives system time to catch up if lagging behind audio
    quiet++;
    if (sh->voice() > 0)
      quiet = 0;
    if (quiet >= 50)   
      continue;

    // pass audio to Cheetah and get next transcription chunk (if any)
    // sometimes this is immediate, other times it blocks for 1.3 sec!
    if ((st = pv_cheetah_process(sh->reco, sh->frame, &txt, &endp)) 
        != PV_STATUS_SUCCESS)
    {
      printf(">>> Picovoice error: %s !\n", pv_status_to_string(st));
      break;
    }

    // force flush at endpoint or long silence (0.8 sec)
    if (quiet == 25)
      endp = true;
    if (endp)       
    {
      free(txt);
      if ((st = pv_cheetah_flush(sh->reco, &txt)) != PV_STATUS_SUCCESS)
      {
        printf(">>> Picovoice error: %s !\n", pv_status_to_string(st));
        break;
      }
    }  

    // see if any new piece of transcription was received
    if (*txt == '\0')
    {
      free(txt);
      txt = NULL;
      continue;
    }

    // append any fragment to accumulated text (uses safe strcat_s)
    pthread_mutex_lock(&(sh->vars));
    len = (int) strlen(sh->partial);
    snprintf(sh->partial + len, 1000 - len, "%s", txt);
    free(txt);
    txt = NULL;
    if (!endp && (sh->dbg > 0))
      printf("  %s ...\n", sh->partial);
       
    // mark new result as ready and register endpoint silence 
    sh->active = 1;
    if (endp)
    { 
      strcpy(sh->result, sh->partial);
      sh->partial[0] = '\0';
      sh->ready = 1;
      sh->active = 0;
    } 

    // let foreground thread read status
    pthread_mutex_unlock(&(sh->vars));
  }
  return NULL;
}


//= Simple voice activity dectector based on noise model.
// checks if any audio sample in current frame is above threshold

int jhcPicoReco::voice () 
{
  const int16_t *b = frame;
  float diff, mix = 0.02, f = 2.0;
  int i, th, ans = 0, vol = 0;

  // estimate current volume as max in frame
  for (i = 0; i < nsamp; i++)
    if (b[i] > vol)
      vol = b[i];
  if (vol < 10)              // muted
    return 0;

  // see if volume unlikely given noise model
  th = (int)(avg + f * sqrt(var) + 0.5);
  if (vol > th)
  {
    vol = th;
    ans = 1;
  }
  
  // udpate IIR noise model
  diff = vol - avg;
  avg += mix * diff;
  var += mix * (diff * diff - var);
  return ans;
}


///////////////////////////////////////////////////////////////////////////
//                            Main Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Check to see if a new utterance is available.
// return: 2 new result, 1 speaking, 0 silence, negative for error

int jhcPicoReco::Status ()
{
  int ans;

  // sanity check
  if ((reco == NULL) || (mic == NULL) || (running <= 0))
    return -2;
  if (pthread_tryjoin_np(bg, NULL) != EBUSY) 
  {
    running = 0;
    return -1;               // background thread crashed
  }

  // determine answer while blocking background thread
  pthread_mutex_lock(&vars);
  if (ready > 0)
  {
    ready = 0;               // once
    ans = 2;
  }
  else
    ans = active;
  pthread_mutex_unlock(&vars);
  return ans; 
}


//= Get last complete utterance head by the speech recognizer.
// copies result since this may get overwritten in subsequent cycles
// returns up to 1000 chars, empty string if nothing ever heard yet

const char *jhcPicoReco::Heard (char *txt)
{
  if (txt == NULL)
    return NULL;
  pthread_mutex_lock(&vars);
  strcpy(txt, result);
  pthread_mutex_unlock(&vars);
  return txt;
}
