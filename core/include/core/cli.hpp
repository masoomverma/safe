#pragma once

#include <string>
#include <vector>

namespace safe {
namespace core {

class CLI {
public:
    struct Command {
        std::string name;
        std::string description;
        std::vector<std::string> args;
    };

    // Parse command line arguments
    static bool ParseArguments(int argc, char* argv[], std::vector<Command>& commands);
    
    // Help and usage
    static void ShowHelp();
    static void ShowVersion();
};

} // namespace core
} // namespace safe
