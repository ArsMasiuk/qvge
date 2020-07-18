#pragma once

#include <QCoreApplication>
#include <QSet>


class CPlatformServices
{
public:
	static bool SetActiveWindow(uint id);

	static bool CloseWindow(uint id);

	typedef QSet<uint> PIDs;
	static PIDs GetRunningPIDs();

    static int GetPlatformBits();   // 32 or 64 ... or more

	static quint64 GetTotalRAMBytes();	// total RAM in bytes, 0 if failed
};


#if defined (Q_OS_WIN32) && !defined(Q_OS_CYGWIN)
#include "CPlatformWin32.h"
#endif
