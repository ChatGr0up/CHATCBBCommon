#include "LockFreeMPSCLogger.hpp"

namespace LOG {
thread_local char LockFreeMPSCLogger::time_buf[32];
}