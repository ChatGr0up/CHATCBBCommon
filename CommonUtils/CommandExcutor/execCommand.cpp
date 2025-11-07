#include "execCommand.hpp"
#include "LogMacro.hpp"

namespace CommonUtils {
class PipeGuard{
public:
    PipeGuard(FILE* & pipe) : pipe_(pipe) {}

    ~PipeGuard(){
        if (pipe_) {
            pclose(pipe_);
        }
    }
private:
    FILE* & pipe_;
};

bool execCommand(const std::string& command, std::ostream& output)
{
    const int BUFFER_SIZE = 1 << 10;
    char buffer[BUFFER_SIZE] = {0};
    
    FILE* pipe = nullptr;
    PipeGuard pipeGuard(pipe);
    try {
        if ((pipe = popen(command.c_str(), "r")) == nullptr) {
            LOG_ERROR("popen failed for command: " << command);
            return false;
        }
    } CATCH_AND_RETURN(false)

     while (fgets(buffer, BUFFER_SIZE, pipe) != nullptr) {
        output << buffer;
    }
    return true;
}
}