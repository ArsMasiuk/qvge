include(CheckCXXSourceCompiles)

check_cxx_source_compiles("
#include <sched.h>
int main() {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	return 0;
}" has_linux_cpu_macros)
