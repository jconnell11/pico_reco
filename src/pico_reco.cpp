// pico_reco.cpp : interface to Piocvoice speech recognition library
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

#include <jhcPicoReco.h>


///////////////////////////////////////////////////////////////////////////
//                          Global Variables                             //
///////////////////////////////////////////////////////////////////////////

//= String untouched by background thread for result.

static char last[1000];


//= Microphone sampler and speech recognition system.

static jhcPicoReco reco;


///////////////////////////////////////////////////////////////////////////
//                           Main Functions                              //
///////////////////////////////////////////////////////////////////////////

//= Connect to speech recognition engine using stored credentials.

extern "C" int pico_start (const char *path, int partial)
{
  reco.dbg = partial;
  return reco.Start(path);
}


//= Check to see if a new utterance is available.
// return: 2 new result, 1 speaking, 0 silence, negative for error

extern "C" int pico_status ()
{
  return reco.Status();
}


//= Get last complete utterance head by the speech recognizer.

extern "C" const char *pico_heard ()
{
  return reco.Heard(last);
}
