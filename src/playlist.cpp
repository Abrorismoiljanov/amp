#include "playlist.h"
#include <algorithm>
#include <filesystem>


std::vector<std::filesystem::path> loadplaylist(const std::string& musicDir){
   std::vector<std::filesystem::path> playlist;

    for (const auto& entry : std::filesystem::directory_iterator(musicDir)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            if (ext == ".mp3" || ext == ".wav" || ext == ".ogg") {
                playlist.push_back(entry.path());
            }
        }
    }
    std::sort(playlist.begin(), playlist.end());

    return playlist;
};
