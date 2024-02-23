#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>

#include "runtime/engine.h"

#include "editor/include/editor.h"

// https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html
#define PICCOLO_XSTR(s) PICCOLO_STR(s)
#define PICCOLO_STR(s) #s

int main(int argc, char** argv)
{
    std::filesystem::path executable_path(argv[0]);
    std::filesystem::path config_file_path = executable_path.parent_path() / "CompassEditor.ini";

    Compass::CompassEngine* engine = new Compass::CompassEngine();

    engine->startEngine(config_file_path.generic_string());
    engine->initialize();

    Compass::CompassEditor* editor = new Compass::CompassEditor();
    editor->initialize(engine);

    editor->run();

    editor->clear();

    engine->clear();
    engine->shutdownEngine();

    return 0;
}
