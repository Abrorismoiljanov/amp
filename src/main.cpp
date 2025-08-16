#include <SDL2/SDL_timer.h>
#include <SDL_events.h>
#include <SDL_keycode.h>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>
#include <sys/ioctl.h>
#include "playlist.h"
#include "filepath.h"

void clearScreen() {
    std::cout << "\033[2J\033[1;1H"; // ANSI escape code, faster than system("clear")
}

enum Playlistmode{
    Normal,
    Loop,
    Random
};

void cleanupAndExit() {
    std::cout << "\033[?25h";   // show cursor
    std::cout << "\033[2J\033[1;1H"; // clear screen
    SDL_Quit();
    Mix_CloseAudio();
    exit(0);
}

int getTrackLength(const std::filesystem::path &file) {
    TagLib::FileRef f(file.c_str());
    if (!f.isNull() && f.audioProperties()) {
        return f.audioProperties()->lengthInSeconds(); // length in seconds
    }
    return 0;
}

struct TrackMeta {
    std::string title;
    std::string artist;
    std::string album;
};

TrackMeta getMetadata(const std::filesystem::path &file) {
    TrackMeta meta;
    TagLib::FileRef f(file.c_str());
    if (!f.isNull() && f.tag()) {
        meta.title  = f.tag()->title().isEmpty()  ? file.filename().string() : f.tag()->title().to8Bit(true);
        meta.artist = f.tag()->artist().isEmpty() ? "Unknown Artist" : f.tag()->artist().to8Bit(true);
        meta.album  = f.tag()->album().isEmpty()  ? "Unknown Album"  : f.tag()->album().to8Bit(true);
    } else {
        meta.title  = file.filename().string();
        meta.artist = "Unknown Artist";
        meta.album  = "Unknown Album";
    }
    return meta;
}

int getTerminalRows() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        return 24; // fallback
    }
    return w.ws_row;
}

void renderUI(const std::string &trackName, int volume, int elapsed, int total, bool ispaused, const TrackMeta &meta, Playlistmode mode, std::vector<std::filesystem::path> playlist, size_t index) {
    clearScreen();

    int rows = getTerminalRows();

    int minutesElapsed = elapsed / 60;
    int secondsElapsed = elapsed % 60;
    int minutesTotal = total / 60;
    int secondsTotal = total % 60;

    int volumePercent = (volume * 100) / 128;
    int volumePercentbar = (volume * 10) / 128;

    std::cout << "\n\n" << " Now Playing: " << trackName << "\n\n";

    std::cout << " " << meta.title << "\n\n"<< " " << meta.artist << "\n\n" << " [" << meta.album << "]\n\n";
 
    const int barWidth = 100;
    int pos = (elapsed * barWidth) / total;

    std::cout << " ["; 
    if (ispaused) {
        std::cout << "  ";
    }else {
        std::cout << "  ";
    }
    std::cout << "] ";
 
    std::cout << " ["; 
    if (mode == Playlistmode::Normal) {
        std::cout << " 󰑖 ";
    }else if (mode ==  Playlistmode::Loop) {
        std::cout << " 󰑘 ";
    }else if(mode == Playlistmode::Random) {
        std::cout << "  ";
    }
    std::cout << "] ";

    std::cout << std::setw(2) << std::setfill('0') << minutesElapsed << ":" << std::setw(2) << secondsElapsed;
    std::cout << " [";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "\033[1;34m█\033[0m";
        else std::cout << " ";
    }
    std::cout << "] ";
    std::cout<< std::setw(2) << minutesTotal << ":" << std::setw(2) << secondsTotal << "\n\n\n";
 

    std::cout << " [";
    for (int i = 0; i < 10; ++i) {
        if (i < volumePercentbar) std::cout << "\033[1;34m█\033[0m";
        else std::cout << " ";
    }
    std::cout << "] ";
    std::cout << "Volume: " << volumePercent << " / 100\n\n";
    
    
    auto safeAccess = [&](size_t i) -> std::string {
        return (i < playlist.size()) ? playlist[i].filename().string() : "";
    };


    std::cout << " " << safeAccess(index - 3) << "\n";
    std::cout << " " << safeAccess(index - 2) << "\n";
    std::cout << " " << safeAccess(index - 1) << "\n";
    std::cout << "\033[47m \033[30m" << "  [" << safeAccess(index) << "] \033[0m \n";
    std::cout << " " << safeAccess(index + 1) << "\n";
    std::cout << " " << safeAccess(index + 2) << "\n";
    std::cout << " " << safeAccess(index + 3) << "\n";
 
    std::cout << "\033[" << rows <<";1H";
    std::cout << "[SPACE] Play/Pause [j] [r] Normal/Loop/Random mode decrease volume [k] increase volume [n] back 10s [m] skip 10s [l] Next [h] Previous [q] Quit\n";
    std::cout << "\033[" << rows << ";1H\033[K";
}


void SNBI(){
    termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO); // No buffering, no echo
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

char getkey(){
    char c = 0;
    read(STDIN_FILENO, &c, 1);
    return c;
}

int main (int argc, char *argv[]) {


    std::cout << "\033[?25l"; // hide cursor

    SDL_Init(SDL_INIT_AUDIO);

    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    auto playlist = loadplaylist(getMusicDirectory(argc ,argv));
    
    SNBI();

   // Decide starting index
    size_t startIndex = 0;
    if (argc >= 2 && std::filesystem::is_regular_file(argv[1])) {
        auto givenFile = std::filesystem::absolute(argv[1]);
        for (size_t i = 0; i < playlist.size(); ++i) {
            if (std::filesystem::absolute(playlist[i]) == givenFile) {
                startIndex = i;
                break;
            }
        }
    }

    size_t index = startIndex;
    Playlistmode mode = Playlistmode::Normal;

    while (true) {
    
        Mix_Music* music = Mix_LoadMUS(playlist[index].c_str());
        Mix_PlayMusic(music, 1);
        
        Uint32 startTime = SDL_GetTicks();
        Uint32 totalPaused = 0;
        Uint32 pauseStart = 0;
        bool isPaused = false;
        int elapsed = 0;
        double currentPos = 0;
        float SkipOffset = 0;

        const int frameDelay = 20; // target 50ms per frame ~20 FPS
        Uint32 lastFrameTime = SDL_GetTicks();

        while (Mix_PlayingMusic()) {

            Uint32 now = SDL_GetTicks();
            int elapsedSinceLastFrame = now - lastFrameTime;

            int total = getTrackLength(playlist[index]);
            int volume = Mix_VolumeMusic(-1);
            

            char key = getkey();
            if (key == 'q') { cleanupAndExit(); return 0;};
            if (key == 'h') {
                Mix_HaltMusic();
                index = (index == 0) ? playlist.size() -1: index -1;
                goto nt;
            };
            if (key == 'l') {
                Mix_HaltMusic();
                index = (index + 1) % playlist.size();
                goto nt;
            };
            if (key == 'r') {
                if (mode == Playlistmode::Normal) {
                    mode = Playlistmode::Loop;
                }else if (mode == Playlistmode::Loop) {
                    mode = Playlistmode::Random;
                }else if (Playlistmode::Random) {
                    mode = Playlistmode::Normal;
                }
            };

            if (key == ' ') {
                if(isPaused){
                    Mix_ResumeMusic();
                    totalPaused += SDL_GetTicks() - pauseStart;
                    isPaused = false;
                }else{
                    Mix_PauseMusic();
                    pauseStart = SDL_GetTicks();
                    isPaused = true;
                };
            };
        
            if (key == 'k') {
                int vol = Mix_VolumeMusic(-1); // Get current volume
                if (vol < 128) Mix_VolumeMusic(vol + 2);
            }
            if (key == 'j') {
                int vol = Mix_VolumeMusic(-1);
                if (vol > 0) Mix_VolumeMusic(vol - 2);
            }
            if (key == 'm') { // skip forward 5 seconds
            SkipOffset += 5;
            if (SkipOffset + (SDL_GetTicks() - startTime - totalPaused)/1000.0 > total) {
                SkipOffset = total - (SDL_GetTicks() - startTime - totalPaused/1000.0);
            }
            Mix_SetMusicPosition(currentPos + 5);
            }
            if (key == 'n') { // skip backward 5 seconds
            SkipOffset -= 5;
            if (SkipOffset < 0) SkipOffset = 0;
            Mix_SetMusicPosition(currentPos - 5);
            }

            if (!isPaused) {
                currentPos = (SDL_GetTicks() - startTime - totalPaused) / 1000.0 + SkipOffset; 
            }

            elapsed = static_cast<int>(currentPos);
            renderUI(playlist[index].filename().string(), volume, elapsed, total, isPaused, getMetadata(playlist[index]), mode, playlist, index);
            
            if (elapsedSinceLastFrame < frameDelay) {
                SDL_Delay(frameDelay - elapsedSinceLastFrame);
                now = SDL_GetTicks(); // update current time after delay
            }
            lastFrameTime = now;

        }
        clearScreen();
    
        if (mode  == Playlistmode::Normal) {
            index = (index + 1) % playlist.size();
        }else if (mode  == Playlistmode::Loop) {
            index = (index) % playlist.size();
        }else if (mode == Playlistmode::Random) {
            index = rand() % playlist.size();
        }
        nt:
            Mix_FreeMusic(music);

    }

    cleanupAndExit();

    return 0;
}
