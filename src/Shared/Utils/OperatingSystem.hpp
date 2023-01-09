#ifndef SRC_SHARED_UTILS_OPERATINGSYSTEM_HPP_
#define SRC_SHARED_UTILS_OPERATINGSYSTEM_HPP_

#include <string>
#include <vector>

namespace os
{

bool openWithSystemHandler(const std::string & path);
intptr_t spawnChildProcess(const std::vector<std::string> & args);
bool isChildProcessRunning(intptr_t process);
}

#endif
