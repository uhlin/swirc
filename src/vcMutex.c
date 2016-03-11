/* Wrappers that deals with WIN32 mutexes
   Copyright (C) 2012, 2013 Markus Uhlin. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   - Neither the name of the author nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include "common.h"
#include "errHand.h"		/* err_quit() */
#include "vcMutex.h"

void
mutex_lock(HANDLE *mutex)
{
    if (WaitForSingleObject(*mutex, INFINITE) != WAIT_OBJECT_0) {
	err_quit("WaitForSingleObject error 0x%lx", (unsigned long int) GetLastError());
    }
}

void
mutex_unlock(HANDLE *mutex)
{
    if (!ReleaseMutex(*mutex)) {
	err_quit("ReleaseMutex error 0x%lx", (unsigned long int) GetLastError());
    }
}

void
mutex_destroy(HANDLE *mutex)
{
    if (!CloseHandle(*mutex)) {
	err_quit("CloseHandle error 0x%lx", (unsigned long int) GetLastError());
    }
}

void
mutex_new(HANDLE *mutex)
{
    if ((*mutex = CreateMutex(NULL, FALSE, NULL)) == NULL) {
	err_quit("CreateMutex error 0x%lx", (unsigned long int) GetLastError());
    }
}
