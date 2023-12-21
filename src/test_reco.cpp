// test_reco.cpp : shell to test Picovoice Cheetah speech recognizer
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

#include <stdio.h>
#include <time.h>

#include <jhcPicoReco.h>


//= Print each verbal utterance as detected by endpointing.
// command line argument is loop time in seconds

int main (int argc, char *argv[])
{
  struct timespec ts = {0, 50 * 1000000};         // 50 ms
  jhcPicoReco sp;
  char sent[1000];
  int i, n, rc, sec = 20;

  // get command line parameters
  if (argc > 1)
    sscanf(argv[1], "%d", &sec);
  sp.dbg = 1;

  // start microphone and speech engine
  printf("Configuring Picovoice and connecting to microphone ...\n");
  if (sp.Start() <= 0)
  {
    printf(">>> Failed to initialize!\n");
    return -1;
  }

  // show partial and final transcriptions 
  printf("--- Transcribing utterances for %d secs ---\n\n", sec);
  n = 20 * sec;
  for (i = 0; i < n; i++)
  {
    if ((rc = sp.Status()) < 0)
      break;
    if (rc == 2)
      printf("%s\n", sp.Heard(sent));
    nanosleep(&ts, NULL);
  }

  // no cleanup needed
  printf("\n--- Done ---\n");
  return 0;
}
