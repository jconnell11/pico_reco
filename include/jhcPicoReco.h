// jhcPicoReco.h : Picovoice Cheetah speech recognizer using microphone
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

#include <pthread.h>

#include <alsa/asoundlib.h>
#include <pv_cheetah.h>


//= Picovoice Cheetah speech recognizer using microphone.
// handles opening audio source and feeding samples to Pico 
// uses background thread so all main calls are non-blocking
// NOTE: needs ALSA so do "sudo apt-get install libasound2-dev"

class jhcPicoReco
{
// PRIVATE MEMBER VARIABLES
private:
  // audio buffer
  int16_t *frame;
  int nsamp;

  // background thread
  pthread_mutex_t vars;
  pthread_t bg;
  int running;

  // components
  snd_pcm_t *mic;
  pv_cheetah_t *reco;

  // audio noise model
  float avg, var;

  // current state
  char partial[1000], result[1000];
  int active, ready;


// PUBLIC MEMBER VARIABLES
public:
  int dbg;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcPicoReco ();
  jhcPicoReco ();
  int Start (const char *path =NULL);

  // main functions
  int Status ();
  const char *Heard (char *txt);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  int cheetah_cfg (const char *path);
  int open_mic ();
  static void *pcm_reco (void *shell);
  int voice ();

};
