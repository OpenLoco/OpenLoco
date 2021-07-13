#ifdef _WIN32
// Ignore warnings generated from yaml-cpp
#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4251) // 'identifier': 'object_type1' 'identifier1' needs to have dll-interface to be used by clients of 'object_type' 'identfier2'
#pragma warning(disable : 4275) // non dll-interface 'classkey' 'identifier1' used as base for dll-interface 'classkey' 'identifier2'
#pragma warning(disable : 4996) // declaration deprecated
#include <yaml-cpp/yaml.h>
#pragma warning(pop)
#else
#include <yaml-cpp/yaml.h>
#endif
