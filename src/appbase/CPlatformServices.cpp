#include "CPlatformServices.h"


int CPlatformServices::GetPlatformBits()
{
    // Check windows
    #if _WIN32 || _WIN64
    #if _WIN64
        #define SYSTEM64
        return 64;
    #else
        #define SYSTEM32
        return 32;
    #endif
    #endif

    // Check GCC
    #if __GNUC__
    #if __x86_64__ || __ppc64__
        #define SYSTEM64
        return 64;
    #else
        #define SYSTEM32
        return 32;
    #endif
    #endif

    // not detected
    return 0;
}


#if defined (Q_OS_WIN32) && !defined(Q_OS_CYGWIN)

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


bool CPlatformServices::CloseWindow(uint id)
{
	HWND hWnd = (HWND)id;
	if (!hWnd)
		return false;

	int result = SendMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);

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


quint64 CPlatformServices::GetTotalRAMBytes()
{
	MEMORYSTATUSEX memory_status;
	ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
	memory_status.dwLength = sizeof(MEMORYSTATUSEX);
	if (GlobalMemoryStatusEx(&memory_status))
	{
		return memory_status.ullTotalPhys;
	}
	else
		return 0;
}


#endif // windows

#if (defined(Q_OS_LINUX) || defined (Q_OS_UNIX) || defined (Q_OS_CYGWIN)) && (!defined (Q_OS_HAIKU)) && (!defined (Q_OS_DARWIN))

#include <QProcessInfo>

#include <QX11Info>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#ifdef Q_OS_FREEBSD
#include <sys/sysctl.h>
#else
#include <sys/sysinfo.h>
#endif


bool CPlatformServices::SetActiveWindow(uint id)
{
    Display *display = QX11Info::display();

    XEvent event = { 0 };
    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    event.xclient.window = id;
    event.xclient.format = 32;

    XSendEvent( display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &event );
    XMapRaised( display, id );

    return true;
}


bool CPlatformServices::CloseWindow(uint id)
{
    Display *display = QX11Info::display();

    XEvent event = { 0 };
    event.xclient.type = ClientMessage;
    event.xclient.window = id;
    event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", True);
    event.xclient.format = 32;
    event.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
    event.xclient.data.l[1] = CurrentTime;
    XSendEvent(display, id, False, NoEventMask, &event);

    return true;
}


CPlatformServices::PIDs CPlatformServices::GetRunningPIDs()
{
    PIDs result;

    auto procList = QProcessInfo::enumerate(false);
    for (auto &procInfo: procList)
    {
        result << procInfo.pid();
    }

    return result;
}


quint64 CPlatformServices::GetTotalRAMBytes()
{
#ifdef Q_OS_FREEBSD

    int mib[2];
    mib[0] = CTL_HW;
    mib[1] = HW_REALMEM;
    unsigned int size = 0;		/* 32-bit */
    size_t len = sizeof( size );
    if ( sysctl( mib, 2, &size, &len, NULL, 0 ) == 0 )
        return (size_t)size;
    else
    return 0L;

#else   // linux etc.

	struct sysinfo info;

	if (sysinfo(&info) == 0)
	{
		return info.totalram;
	}
	else
		return 0;

#endif
}


#endif // linux


// haiku
#if (defined(Q_OS_HAIKU) || defined(Q_OS_DARWIN))

quint64 CPlatformServices::GetTotalRAMBytes()
{
    return 0;
}

CPlatformServices::PIDs CPlatformServices::GetRunningPIDs()
{
    PIDs result;

    return result;
}

bool CPlatformServices::CloseWindow(uint id)
{
    return true;
}

bool CPlatformServices::SetActiveWindow(uint id)
{
    return true;
}

#endif // haiku
