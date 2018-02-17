Buncha old avisynth junk.  Unlikely to be of any value to anyone.
Dlls compiled for avisynth 2.6.0a3 (32 bit)

Unless otherwise noted, copyright for all things in this is included below.

1. dolphinmagic - Used to fix up some old dolphin dump.  No idea what it does.  Probably not any value.
2. framercache - forcibly caches a single frame of video from source.  It was used to cache expensive-to-generate high res subtitles, when AviSynth's frame cache heuristics were dropping it sometimes.  No value unless a single frame from its source will be reused.
3. freesub - subtitle with bdf fonts.  Example font files (no copyright, per US law) in /font subfolder.  Uses a library (SDL_bdf) that's included with its own separate copyright notice.
4. gbrptorgb - converts gbrp that's masquerading as YV24 to RGB losslessly.
5. timecodefps - Converts vfr to cfr using mkv timecodes as input, or something.  It has its own readme.  Originally available under BSD 3-clause; also available under the license reproduced below.
6. toyv24 - Point resize of yv12 to yv24.  Avisynth built in one was junk I think.

Copyright 2012 Nicholai Main

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


