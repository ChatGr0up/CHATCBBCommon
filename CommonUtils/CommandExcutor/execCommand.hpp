#pragma once

#include <string>
#include <ostream>

namespace CommonUtils {

/**
 * Executes a shell command and captures its output.
 * @param command The command to execute.
 * @param output An output stream to capture the command's output.
 * @return true if the command executed successfully, false otherwise.
 */
bool execCommand(const std::string& command, std::ostream& output);
}