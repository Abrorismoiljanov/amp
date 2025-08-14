#include <SDL2/SDL.h>
#include <filesystem>
#include "filepath.h"

std::filesystem::path getMusicDirectory(int argc, char **argv){
    if (argc >= 2) {
        std::filesystem::path argPath = argv[1];

        if (std::filesystem::is_regular_file(argPath)) {
            // Case 1: Given a file → return its parent folder
            return argPath.parent_path();
        }
        else if (std::filesystem::is_directory(argPath)) {
            // Case 2: Given a folder → return that folder
            return argPath;
        }
        else if (std::filesystem::exists(std::filesystem::current_path() / argPath)) {
            // Case 3: Just a filename in current working dir
            return std::filesystem::current_path();
        }
    }

    return std::filesystem::current_path();
}
