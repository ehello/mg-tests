///////////////////////////////////////////////////////////////////////////////
//
//                          IMPORTANT NOTICE
//
// The following open source license statement does not apply to any
// entity in the Exception List published by FMSoft.
//
// For more information, please visit:
//
// https://www.fmsoft.cn/exception-list
//
//////////////////////////////////////////////////////////////////////////////
/*
** $Id: mginit.c 322 2007-08-30 01:20:10Z xwyan $
**
** Listing 31.1
**
** mginit.c: Sample program for MiniGUI Programming Guide
**      A simple mginit program.
**
** Copyright (C) 2003 ~ 2017 FMSoft (http://www.fmsoft.cn).
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#ifdef _MGRM_PROCESSES

static BOOL quit = FALSE;
static int nr_clients = 0;
#ifdef _MGSCHEMA_COMPOSITING
static pid_t pid_welcome = 0;
static pid_t pid_dynamic = 0;
static char* exe_cmd;
#endif

static pid_t exec_app (const char* file_name, const char* app_name)
{
    pid_t pid = 0;

    if ((pid = vfork ()) > 0) {
        _MG_PRINTF ("new child, pid: %d.\n", pid);
    }
    else if (pid == 0) {
        execl (file_name, app_name, NULL);
        perror ("execl");
        _exit (1);
    }
    else {
        perror ("vfork");
    }

    return pid;
}

static void on_new_del_client (int op, int cli)
{
    if (op == LCO_NEW_CLIENT) {
        nr_clients ++;
        _MG_PRINTF ("A new client: %d.\n", mgClients[cli].pid);
    }
    else if (op == LCO_DEL_CLIENT) {
        _MG_PRINTF ("A client left: %d.\n", mgClients[cli].pid);
        nr_clients --;
        if (nr_clients == 0) {
            _MG_PRINTF ("There is no any client.\n");
        }
        else if (nr_clients < 0) {
            _ERR_PRINTF ("Serious error: nr_clients less than zero.\n");
        }

#ifdef _MGSCHEMA_COMPOSITING
        if (pid_welcome == mgClients[cli].pid) {
            pid_welcome = 0;
            if (exe_cmd) {
                exec_app (exe_cmd, exe_cmd);
                free (exe_cmd);
                exe_cmd = NULL;
            }
        }
#endif
    }
    else
        _ERR_PRINTF ("Serious error: incorrect operations.\n");
}

static unsigned int old_tick_count;

static pid_t pid_scrnsaver = 0;

static inline void dump_key_messages (PMSG msg)
{
    if (msg->message == MSG_KEYDOWN || msg->message == MSG_KEYUP) {
        _WRN_PRINTF ("%s (%d) %s KS_REPEATED\n",
                (msg->message == MSG_KEYDOWN)?"MSG_KEYDOWN":"MSG_KEYUP",
                (int)msg->wParam,
                (msg->lParam & KS_REPEATED)?"with":"without");
    }
}

static int my_event_hook (PMSG msg)
{
    old_tick_count = GetTickCount ();

    if (pid_scrnsaver) {
        kill (pid_scrnsaver, SIGINT);
        ShowCursor (TRUE);
        pid_scrnsaver = 0;
    }

    dump_key_messages(msg);

    if (msg->message == MSG_KEYDOWN) {
        switch (msg->wParam) {
        case SCANCODE_ESCAPE:
            if (nr_clients == 0) {
                quit = TRUE;
            }
            break;

        case SCANCODE_SPACE:
#ifdef _MGSCHEMA_COMPOSITING
            if (pid_welcome == 0 && pid_dynamic == 0)
                pid_dynamic = exec_app ("./wallpaper-dynamic", "wallpaper-dynamic");
#endif
           break;

        case SCANCODE_F1:
           exec_app ("./edit", "edit");
           break;
        case SCANCODE_F2:
           exec_app ("./menubutton", "menubutton");
           break;
        case SCANCODE_F3:
           exec_app ("./combobox", "combobox");
           break;
        case SCANCODE_F4:
           exec_app ("./eventdumper", "eventdumper");
           break;
        case SCANCODE_F5:
           exec_app ("./helloworld", "helloworld");
           break;
    }
    }

    return HOOK_GOON;
}

static void child_wait (int sig)
{
    int pid;
    int status;

    while ((pid = waitpid (-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED (status))
            _MG_PRINTF ("--pid=%d--status=%x--rc=%d---\n", pid, status, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            _MG_PRINTF ("--pid=%d--signal=%d--\n", pid, WTERMSIG (status));
    }
}

int MiniGUIMain (int argc, const char* argv[])
{
    MSG msg;
    struct sigaction siga;

    siga.sa_handler = child_wait;
    siga.sa_flags  = 0;
    memset (&siga.sa_mask, 0, sizeof(sigset_t));
    sigaction (SIGCHLD, &siga, NULL);

    OnNewDelClient = on_new_del_client;

    if (!ServerStartup (0 , 0 , 0)) {
        _ERR_PRINTF("Can not start the server of MiniGUI-Processes: mginit.\n");
        return 1;
    }

#ifdef _MGSCHEMA_COMPOSITING
    if (argc > 1 && strcasecmp (argv[1], "auto") == 0) {
        pid_dynamic = exec_app ("./wallpaper-dynamic", "wallpaper-dynamic");
        exec_app ("./static", "static");
        exec_app ("./edit", "edit");
        exec_app ("./eventdumper", "eventdumper");
    }
    else if (argc > 1 && strcasecmp (argv[1], "none") == 0) {
        // do not start any child
    }
    else {
        pid_welcome = exec_app ("./wallpaper-welcome", "wallpaper-welcome");
        if (argc > 1)
            exe_cmd = strdup (argv[1]);
    }
#endif

    SetServerEventHook (my_event_hook);

    old_tick_count = GetTickCount ();

    while (!quit && GetMessage (&msg, HWND_DESKTOP)) {
        DispatchMessage (&msg);
    }

    return 0;
}

#else   /* defined _MGRM_PROCESSES */

int main (int argc, const char* argv[])
{
    _WRN_PRINTF ("This test program is the server for MiniGUI-Prcesses "
           "runtime mode. But your MiniGUI was not configured as "
           "MiniGUI-Processes\n");
    return 0;
}

#endif  /* not defined _MGRM_PROCESSES */

