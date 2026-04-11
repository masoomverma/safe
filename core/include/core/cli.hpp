#ifndef CLI_HPP
#define CLI_HPP

#include <string>
#include <vector>

namespace safe::core
{

    class CLI {
    public:
        struct Command {
            std::string name;
            std::string description;
            std::vector<std::string> args;
        };

        // Parse command line arguments
        static bool ParseArguments(int argc, char* argv[], const std::vector<Command>& commands);

        // Help and usage
        static void ShowHelp();
        static void ShowVersion();
    };

} // namespace safe::core

#endif
