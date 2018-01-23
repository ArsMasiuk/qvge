#pragma once

#include <QCoreApplication>
#include <QSet>


class CPlatformServices
{
public:
	static bool SetActiveWindow(uint id);

	typedef QSet<uint> PIDs;
	static PIDs GetRunningPIDs();

    static int GetPlatformBits();   // 32 or 64 ... or more

	static quint64 GetTotalRAMBytes();	// total RAM in bytes, 0 if failed
};
