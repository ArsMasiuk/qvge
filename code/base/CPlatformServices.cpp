#include "CPlatformServices.h"


#ifdef Q_OS_WIN32

#include <Windows.h>
#include <Psapi.h>


bool CPlatformServices::SetActiveWindow(uint id)
{
	HWND hWnd = (HWND)id;
	if (!hWnd)
		return false;

	BringWindowToTop(hWnd);

	HWND   hCurrWnd = GetForegroundWindow();
	DWORD  myThreadID = GetCurrentThreadId();
	DWORD  currThreadID = GetWindowThreadProcessId(hCurrWnd, NULL);
	AttachThreadInput(myThreadID, currThreadID, TRUE);
	SetForegroundWindow(hWnd);
	SetFocus(hWnd);
	AttachThreadInput(myThreadID, currThreadID, FALSE);

	if (IsIconic(hWnd))
		ShowWindow(hWnd, SW_RESTORE);
	else
		ShowWindow(hWnd, SW_SHOW);

	return true;
}


CPlatformServices::PIDs CPlatformServices::GetRunningPIDs()
{
	PIDs result;

	DWORD pids[10240];
	DWORD bytesReturned;
	if (EnumProcesses(pids, 10240 * sizeof(DWORD), &bytesReturned))
	{
		int count = bytesReturned / sizeof(DWORD);
		for (int i = 0; i < count; ++i)
		{
			result << pids[i];
		}
	}

	return result;
}


#endif // windows


#ifdef Q_OS_LINUX

#include <QX11Info>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

extern "C" {
#include <read_proc.h>
}

bool CPlatformServices::SetActiveWindow(uint id)
{
    Display * display = QX11Info::display();

    XEvent event = { 0 };
    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = XInternAtom( display, "_NET_ACTIVE_WINDOW", False);
    event.xclient.window = id;
    event.xclient.format = 32;

    XSendEvent( display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &event );
    XMapRaised( display, id );

    return true;
}


CPlatformServices::PIDs CPlatformServices::GetRunningPIDs()
{
    PIDs result;

    struct Root *root = read_proc();
    struct Job *buffer = NULL;

    for(int i=0; i<root->len; i++)
    {
        buffer = get_from_place(root,i);
        //printf("%s\t%u\n",buffer->name,buffer->pid);

        result << buffer->pid;
    }

    return result;
}

#endif // linux
