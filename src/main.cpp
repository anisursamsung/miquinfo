#include "info_app.hpp"
#include "sys_info.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--print" || arg == "-p" || arg == "--summary" || arg == "-s") {
            miquinfo::SystemInfo info = miquinfo::SysInfoReader::gather();
            std::cout << info.to_plain_text();
            return 0;
        } else if (arg == "--markdown" || arg == "-m") {
            miquinfo::SystemInfo info = miquinfo::SysInfoReader::gather();
            std::cout << info.to_markdown();
            return 0;
        }
    }

    auto engine = miqu::AppEngine::create();
    if (!engine) {
        std::cerr << "[miquinfo] Failed to initialize AppEngine.\n";
        return 1;
    }

    miquinfo::InfoApp app(engine);
    if (!app.init()) {
        std::cerr << "[miquinfo] Failed to initialize UI window.\n";
        return 1;
    }

    return app.run();
}
