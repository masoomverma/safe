#include "core/cli.hpp"
#include "core/types.hpp"
#include <iostream>

namespace safe::core
{
    bool CLI::ParseArguments(int argc, char* argv[], const std::vector<Command>& commands) {
        (void)argc;     // stub implementation
        (void)argv;     // stub implementation
        (void)commands; // stub implementation
        return true;
    }

    void CLI::ShowHelp() {
        std::cout << "Safe v" << APP_VERSION.major << "." << APP_VERSION.minor << "." << APP_VERSION.patch << "\n";
        std::cout << "Usage: Safe [options]\n";
        std::cout << "Options:\n";
        std::cout << "  --help     Show this help message\n";
        std::cout << "  --version  Show version information\n";
    }

    void CLI::ShowVersion() {
        std::cout << "Safe version " << APP_VERSION.ToString() << "\n";
    }

} // namespace safe::core
