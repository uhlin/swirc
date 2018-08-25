/* SpawnMessageLoop.cpp
   Copyright (C) 2018 Markus Uhlin. All rights reserved.

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

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#ifdef TOAST_NOTIFICATIONS
#include "common.h"

#include <process.h>

#include "DesktopNotificationManagerCompat.hpp"
#include "DesktopToastsApp.hpp"
#include "SpawnMessageLoop.h"
#include "errHand.h"

static DesktopToastsApp app;
static HINSTANCE hInstance;
static uintptr_t thread_id;

void
JoinMessageLoop(void)
{
    (void) WaitForSingleObject((HANDLE) thread_id, 10000);
}

static void __cdecl
MessageLoop(void *arg)
{
    if ((hInstance = GetModuleHandle(NULL)) == NULL)
	err_log(0, "MessageLoop: Failed to get module handle");

    app.SetHInstance(hInstance);

    if (app.Initialize(hInstance) != S_OK)
	err_log(0, "MessageLoop: Failed to prepare the main window");

    app.RunMessageLoop();
}

void
SpawnMessageLoop(void)
{
    static const uintptr_t UNSUCCESSFUL = (uintptr_t) -1L;

    if ((thread_id = _beginthread(MessageLoop, 0, NULL)) == UNSUCCESSFUL)
	err_sys("SpawnMessageLoop: Failed to create a thread");
}
#endif /* -----TOAST_NOTIFICATIONS----- */
