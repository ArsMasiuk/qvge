include(CheckCXXSourceCompiles)

check_cxx_source_compiles("
#include <string>
int main() {
	std::string copy(__PRETTY_FUNCTION__);
	return 0;
}" compiler_has_pretty_function)
