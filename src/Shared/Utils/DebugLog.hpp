#ifndef SRC_SHARED_UTILS_DEBUGLOG_HPP_
#define SRC_SHARED_UTILS_DEBUGLOG_HPP_

#include <string>

#define WOS_DEBUG_LOCATION writeDebugLocation(__FILE__, __LINE__, __PRETTY_FUNCTION__);

void writeDebugOutput(std::string log);
void writeDebugLocation(const char * fileName, unsigned int lineNumber, const char * functionName);

#endif
