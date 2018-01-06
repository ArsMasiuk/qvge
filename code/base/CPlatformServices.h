#pragma once

#include <QCoreApplication>
#include <QSet>


class CPlatformServices
{
public:
	static bool SetActiveWindow(uint id);

	typedef QSet<uint> PIDs;
	static PIDs GetRunningPIDs();
};
