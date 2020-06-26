/** \file
 * \brief Implementation of System class.
 *
 * \author Carsten Gutwenger
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <ogdf/basic/exceptions.h>
#include <ogdf/basic/memory.h>
#include <ogdf/basic/System.h>

#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
#define WIN32_EXTRA_LEAN
#define WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#define NOMINMAX

#include <windows.h>
#include <Psapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "psapi.lib")
#endif
#endif


#ifdef __APPLE__
#include <stdlib.h>
#include <malloc/malloc.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <mach/vm_statistics.h>
#include <mach/mach.h>
#include <mach/machine.h>
#elif defined(OGDF_SYSTEM_UNIX)
#include <malloc.h>
#endif

#if defined(_MSC_VER)
# include <intrin.h>
#elif defined(OGDF_SYSTEM_UNIX) || (defined(__MINGW32__) && !defined(__MINGW64__))
# include <unistd.h>
# include <fcntl.h>
# include <sys/time.h>
#endif
#ifdef __GNUC__
# include <cpuid.h>
#endif

static inline void cpuid(int CPUInfo[4], int infoType)
{
#if defined(OGDF_SYSTEM_WINDOWS) && !defined(__GNUC__)
	__cpuid(CPUInfo, infoType);
#else
	uint32_t a = 0;
	uint32_t b = 0;
	uint32_t c = 0;
	uint32_t d = 0;

# ifdef __GNUC__
	__get_cpuid(infoType, &a, &b, &c, &d);
# endif

	CPUInfo[0] = a;
	CPUInfo[1] = b;
	CPUInfo[2] = c;
	CPUInfo[3] = d;
#endif
}


namespace ogdf {

unsigned int System::s_cpuFeatures = 0;
int System::s_cacheSize = 0;
int System::s_cacheLine = 0;
int System::s_pageSize = 0;
int System::s_numberOfProcessors = 1;

#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
int64_t System::s_HPCounterFrequency;
#endif

unsigned int operator|=(unsigned int &i, CPUFeatureMask fm){
	i |= static_cast<unsigned int>(fm);
	return i;
}

void System::init()
{
	int CPUInfo[4] = {-1};
	cpuid(CPUInfo, 0);

	unsigned int nIds = CPUInfo[0];
	if(nIds >= 1)
	{
		cpuid(CPUInfo, 1);

		int featureInfoECX = CPUInfo[2];
		int featureInfoEDX = CPUInfo[3];

		if(featureInfoEDX & (1 << 23)) s_cpuFeatures |= CPUFeatureMask::MMX;
		if(featureInfoEDX & (1 << 25)) s_cpuFeatures |= CPUFeatureMask::SSE;
		if(featureInfoEDX & (1 << 26)) s_cpuFeatures |= CPUFeatureMask::SSE2;
		if(featureInfoECX & (1 <<  0)) s_cpuFeatures |= CPUFeatureMask::SSE3;
		if(featureInfoECX & (1 <<  9)) s_cpuFeatures |= CPUFeatureMask::SSSE3;
		if(featureInfoECX & (1 << 19)) s_cpuFeatures |= CPUFeatureMask::SSE4_1;
		if(featureInfoECX & (1 << 20)) s_cpuFeatures |= CPUFeatureMask::SSE4_2;
		if(featureInfoECX & (1 <<  5)) s_cpuFeatures |= CPUFeatureMask::VMX;
		if(featureInfoECX & (1 <<  6)) s_cpuFeatures |= CPUFeatureMask::SMX;
		if(featureInfoECX & (1 <<  7)) s_cpuFeatures |= CPUFeatureMask::EST;
		if(featureInfoECX & (1 <<  3)) s_cpuFeatures |= CPUFeatureMask::MONITOR;
	}

	cpuid(CPUInfo, 0x80000000);
	unsigned int nExIds = CPUInfo[0];

	if(nExIds >= 0x80000006) {
		cpuid(CPUInfo, 0x80000006);
		s_cacheLine = CPUInfo[2] & 0xff;
		s_cacheSize = (CPUInfo[2] >> 16) & 0xffff;
	}

#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&s_HPCounterFrequency));

	SYSTEM_INFO siSysInfo;
	GetSystemInfo(&siSysInfo);
	s_pageSize = siSysInfo.dwPageSize;
	s_numberOfProcessors = siSysInfo.dwNumberOfProcessors;
#elif defined(OGDF_SYSTEM_UNIX)
# if defined(__APPLE__)
	unsigned long long value;
	size_t size = sizeof(value);
	if (sysctlbyname("hw.pagesize", &value, &size, nullptr, 0) != -1) {
		s_pageSize = (int)value;
	}
	if (sysctlbyname("hw.ncpu", &value, &size, nullptr, 0) != -1) {
		s_numberOfProcessors = (int)value;
	}
# else
	s_pageSize = (int)sysconf(_SC_PAGESIZE);
	s_numberOfProcessors = (int)sysconf(_SC_NPROCESSORS_CONF);
# endif
#endif
}


#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
void System::getHPCounter(int64_t &counter)
{
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&counter));
}


double System::elapsedSeconds(
	const int64_t &startCounter,
	const int64_t &endCounter)
{
	return double(endCounter - startCounter) / s_HPCounterFrequency;
}


int64_t System::usedRealTime(int64_t &t)
{
	int64_t tStart = t;
#if _WIN32_WINNT >= 0x0600
	t = GetTickCount64();
#else
	t = GetTickCount();
#endif
	return t - tStart;
}

int64_t System::realTime()
{
#if _WIN32_WINNT >= 0x0600
	return GetTickCount64();
#else
	return GetTickCount();
#endif
}


long long System::physicalMemory()
{
#if !defined(__CYGWIN__) || (_WIN32_WINNT >= 0x0500)
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);

	GlobalMemoryStatusEx (&statex);
	return statex.ullTotalPhys;
#else
	MEMORYSTATUS stat;
	stat.dwLength = sizeof (stat);

	GlobalMemoryStatus (&stat);
	return stat.dwTotalPhys;
#endif
}

long long System::availablePhysicalMemory()
{
#if !defined(__CYGWIN__) || (_WIN32_WINNT >= 0x0500)
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);

	GlobalMemoryStatusEx (&statex);
	return statex.ullAvailPhys;
#else
	MEMORYSTATUS stat;
	stat.dwLength = sizeof (stat);

	GlobalMemoryStatus (&stat);
	return stat.dwAvailPhys;
#endif
}

size_t System::memoryUsedByProcess()
{
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

	return pmc.WorkingSetSize;
}

size_t System::peakMemoryUsedByProcess()
{
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

	return pmc.PeakWorkingSetSize;
}

#elif __APPLE__

long long System::physicalMemory()
{
	unsigned long long value;
	size_t  size = sizeof( value );
	if (sysctlbyname("hw.memsize", &value, &size, nullptr, 0) !=-1)
		return value;
	else
		return 0;
}

long long System::availablePhysicalMemory()
{
	unsigned long long pageSize;
	long long result;
	size_t  size = sizeof( pageSize );
	sysctlbyname("hw.pagesize", &pageSize, &size, nullptr, 0);

	vm_statistics_data_t vm_stat;
	int count = ((mach_msg_type_number_t) (sizeof(vm_statistics_data_t)/sizeof(integer_t)));
	host_statistics(mach_host_self(), HOST_VM_INFO, (integer_t*)&vm_stat, (mach_msg_type_number_t*)&count);
	result = (unsigned long long)(vm_stat.free_count + vm_stat.inactive_count) * pageSize;
	return result;
}


size_t System::memoryUsedByProcess()
{
	// not implemented
	return 0;
}

#else
// LINUX, NOT MAC OS
long long System::physicalMemory()
{
	return (long long)(sysconf(_SC_PHYS_PAGES)) * sysconf(_SC_PAGESIZE);
}

long long System::availablePhysicalMemory()
{
	return (long long)(sysconf(_SC_AVPHYS_PAGES)) * sysconf(_SC_PAGESIZE);
}

size_t System::memoryUsedByProcess()
{
	int pid = getpid();
	string filename = string("/proc/") + to_string(pid) + "/statm";

	std::ifstream is(filename.c_str());
	if(!is) OGDF_THROW(Exception);

	//	size:      total program size (in pages)
	//	resident:  number of resident set (non-swapped) pages (4k)
	//	share:     number of pages of shared (mmap'd) memory
	//	trs:       text resident set size
	//	lrs:       shared-lib resident set size
	//	drs:       data resident set size
	//	dt:        dirty pages
	long size, resident, share, trs, lrs, drs, dt;
	is >> size >> resident >> share >> trs >> lrs >> drs >> dt;

	return size*4*1024;
}

#endif


#ifdef OGDF_SYSTEM_WINDOWS

size_t System::memoryAllocatedByMalloc()
{
	_HEAPINFO hinfo;
	int heapstatus;
	hinfo._pentry = nullptr;

	size_t allocMem = 0;
	while((heapstatus = _heapwalk(&hinfo)) == _HEAPOK)
	{
		if(hinfo._useflag == _USEDENTRY)
			allocMem += hinfo._size;
	}

	return allocMem;
}

size_t System::memoryInFreelistOfMalloc()
{
	_HEAPINFO hinfo;
	int heapstatus;
	hinfo._pentry = nullptr;

	size_t allocMem = 0;
	while((heapstatus = _heapwalk(&hinfo)) == _HEAPOK)
	{
		if(hinfo._useflag == _FREEENTRY)
			allocMem += hinfo._size;
	}

	return allocMem;
}

#elif __APPLE__

size_t System::memoryAllocatedByMalloc()
{
	return mstats().bytes_used;
}

size_t System::memoryInFreelistOfMalloc()
{
	return mstats().bytes_free;
}
#else

size_t System::memoryAllocatedByMalloc()
{
	return mallinfo().uordblks;
}

size_t System::memoryInFreelistOfMalloc()
{
	return mallinfo().fordblks;
}

#endif

#if !defined(OGDF_SYSTEM_WINDOWS) && !defined(__CYGWIN__)
int64_t System::usedRealTime(int64_t &t)
{
	int64_t tStart = t;
	timeval tv;
	gettimeofday(&tv, nullptr);
	t = int64_t(tv.tv_sec) * 1000 + tv.tv_usec/1000;
	return t - tStart;
}

int64_t System::realTime()
{
	timeval tv;
	gettimeofday(&tv, nullptr);
	return int64_t(tv.tv_sec) * 1000 + tv.tv_usec/1000;
}
#endif


size_t System::memoryAllocatedByMemoryManager()
{
	return OGDF_ALLOCATOR::memoryAllocatedInBlocks();
}

size_t System::memoryInGlobalFreeListOfMemoryManager()
{
	return OGDF_ALLOCATOR::memoryInGlobalFreeList();
}

size_t System::memoryInThreadFreeListOfMemoryManager()
{
	return OGDF_ALLOCATOR::memoryInThreadFreeList();
}


// TODO: Untested for cygwin, mingw!
#ifdef OGDF_SYSTEM_WINDOWS

int System::getProcessID()
{
	return GetCurrentProcessId();
}

#else

int System::getProcessID()
{
	return getpid();
}

#endif

}
