#include <ogdf/basic/basic.h>
#include <ogdf/basic/System.h>

using namespace ogdf;

const char *yn(bool b)
{
	return b ? "yes" : "no";
}

int main()
{
	std::cout
	  << "---------------------------------------" << std::endl
	  << "      System-specific information      " << std::endl
	  << "---------------------------------------" << std::endl
	  << std::endl

	  << "Cache / processors:" << std::endl
	  << "-------------------" << std::endl
	  << "Processors: " << System::numberOfProcessors() << std::endl
	  << "L2-Cache:   " << System::cacheSizeKBytes() << " KBytes" << std::endl
	  << "Cache-Line: " << System::cacheLineBytes() << " Bytes" << std::endl
	  << std::endl

	  << "Supported technologies:" << std::endl
	  << "-----------------------" << std::endl
	  << "MMX:    " << yn(System::cpuSupports(CPUFeature::MMX))    << std::endl
	  << "SSE:    " << yn(System::cpuSupports(CPUFeature::SSE))    << std::endl
	  << "SSE2:   " << yn(System::cpuSupports(CPUFeature::SSE2))   << std::endl
	  << "SSE3:   " << yn(System::cpuSupports(CPUFeature::SSE3))   << std::endl
	  << "SSSE3:  " << yn(System::cpuSupports(CPUFeature::SSSE3))  << std::endl
	  << "SSE4.1: " << yn(System::cpuSupports(CPUFeature::SSE4_1)) << std::endl
	  << "SSE4.2: " << yn(System::cpuSupports(CPUFeature::SSE4_2)) << std::endl
	  << "VMX:    " << yn(System::cpuSupports(CPUFeature::VMX))    << std::endl
	  << "SMX:    " << yn(System::cpuSupports(CPUFeature::SMX))    << std::endl
	  << "EST:    " << yn(System::cpuSupports(CPUFeature::EST))    << std::endl
	  << std::endl

	  << "Memory management:" << std::endl
	  << "------------------" << std::endl
	  << "Total physical memory: " << System::physicalMemory() / 1024 / 1024 << " MBytes" << std::endl
	  << "  available:           " << System::availablePhysicalMemory() / 1024 / 1024 << " MBytes" << std::endl
	  << "  used by process:     " << System::memoryUsedByProcess() / 1024 << " KBytes" << std::endl
#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
	  << "  peak amount:         " << System::peakMemoryUsedByProcess() / 1024 << " KBytes" << std::endl
#endif
	  << std::endl
	  << "allocated by malloc:   " << System::memoryAllocatedByMalloc() / 1024 << " KBytes" << std::endl
	  << "  in freelist:         " << System::memoryInFreelistOfMalloc() / 1024 << " KBytes" << std::endl
	  << std::endl
	  << "allocated by OGDF:     " << System::memoryAllocatedByMemoryManager() / 1024 << " KBytes" << std::endl
	  << "  in global freelist:  " << System::memoryInGlobalFreeListOfMemoryManager() / 1024 << " KBytes" << std::endl
	  << "  in thread freelist:  " << System::memoryInThreadFreeListOfMemoryManager() / 1024 << " KBytes" << std::endl;

	return 0;
}
