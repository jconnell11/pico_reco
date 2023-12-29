#!/usr/bin/env python3
# encoding: utf-8

# =========================================================================
#
# pico_wrap.py : Python wrapper for Picovoice Cheetah speech recognizer
#
# Written by Jonathan H. Connell, jconnell@alum.mit.edu
#
# =========================================================================
#
# Copyright 2023 Etaoin Systems
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 
# =========================================================================

import time

from ctypes import *

import os
here = os.path.dirname(__file__)       
lib = CDLL(here + '/libpico_reco.so')   
       

# Python wrapper for Picovoice Cheetah speech recognizer
# needs key file = "picovoice.key" and model file = "cheetah_params.pv"
# assumes files are in subdirectory "config" wrt script directory
# returns 1 if okay, 0 or negative for problem

class PicoReco:

  # connect to speech recognition engine using stored credentials
  # can optionally print out partial results as they become available
  def Start(self, partial):
    return lib.pico_start(here.encode(), partial)

  # check to see if a new utterance is available
  # return: 2 new result, 1 speaking, 0 silence, negative for error
  def Status(self):
    return lib.pico_status()

  # get last complete utterance head by the speech recognizer
  def Heard(self):
    msg = c_char_p(lib.pico_heard()).value
    return msg.decode()


# =========================================================================

# simple test program runs using default microphone for 20 seconds
if __name__ == "__main__":
  r = PicoReco();
  print('Configuring Picovoice and connecting to microphone ...')
  if r.Start(1) <= 0:
    print('  >>> Failed!')
  else:
    print('--- Transcribing for 20 secs ---')
    for i in range(600):
      st = r.Status()
      if st < 0:
        break
      if st == 2:
        print('  ' + r.Heard())
      time.sleep(0.033) 
  print('--- Done ---')

