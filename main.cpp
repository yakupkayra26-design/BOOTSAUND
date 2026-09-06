#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <cctype>

#ifndef BUILD_SYSMODULE
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif

static bool has_mp3_extension(const std::string& name) {
    if (name.size() < 4) return false;
    std::string extension = name.substr(name.size() - 4);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char character) { return (char)std::tolower(character); });
    return extension == ".mp3";
}

#ifdef BUILD_SYSMODULE

// ============================================================================
// SYSMODULE BÖLÜMÜ (Boot Anında Çalışır)
// ============================================================================

extern "C" {
    u32 __nx_applet_type = AppletType_None;
    size_t __nx_heap_size = 0x200000;
}

std::string get_active_track() {
    std::ifstream file("sdmc:/BOOTSOUND/config.ini");
    if (!file.is_open()) return "";
    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("active_track=", 0) == 0) {
            return line.substr(13);
        }
    }
    return "";
}

int main(int argc, char **argv) {
    if (R_FAILED(smInitialize())) return 0;
    if (R_FAILED(fsInitialize())) { smExit(); return 0; }

    FsFileSystem sdmcFs;
    if (R_SUCCEEDED(fsOpenSdCardFileSystem(&sdmcFs))) {
        fsdevMountDevice("sdmc", sdmcFs);
    } else {
        fsExit(); smExit(); return 0;
    }

    mkdir("sdmc:/BOOTSOUND", 0777);

    std::string track = get_active_track();
    if (track.empty() || track == "none") {
        fsdevUnmountDevice("sdmc");
        fsExit(); smExit(); return 0;
    }

    std::string mp3Path = "sdmc:/BOOTSOUND/" + track;

    drmp3 mp3;
    if (!drmp3_init_file(&mp3, mp3Path.c_str(), NULL)) {
        fsdevUnmountDevice("sdmc");
        fsExit(); smExit(); return 0;
    }

    if (mp3.channels < 1 || mp3.channels > 2) {
        drmp3_uninit(&mp3);
        fsdevUnmountDevice("sdmc");
        fsExit();
        smExit();
        return 0;
    }

    if (R_SUCCEEDED(audoutInitialize())) {
        char deviceName[0x100] = {};
        u32 sampleRate = 0;
        u32 channelCount = 0;
        PcmFormat format;
        AudioOutState state;
        Result openResult = audoutOpenAudioOut(
            NULL, deviceName, 48000, 2, &sampleRate, &channelCount, &format, &state);

        if (R_FAILED(openResult) || sampleRate == 0 || channelCount != 2) {
            drmp3_uninit(&mp3);
            audoutExit();
            fsdevUnmountDevice("sdmc");
            fsExit();
            smExit();
            return 0;
        }

        audoutStartAudioOut();
        const size_t FRAME_COUNT = 4096 * 4;
        const size_t SAMPLE_COUNT = FRAME_COUNT * 2;
        int16_t* pcmData = (int16_t*)aligned_alloc(0x1000, SAMPLE_COUNT * sizeof(int16_t));
        if (!pcmData) {
            audoutStopAudioOut();
            audoutExit();
            drmp3_uninit(&mp3);
            fsdevUnmountDevice("sdmc");
            fsExit();
            smExit();
            return 0;
        }

        AudioOutBuffer buf;
        memset(&buf, 0, sizeof(AudioOutBuffer));
        buf.buffer = pcmData;
        buf.buffer_size = SAMPLE_COUNT * sizeof(int16_t);

        drmp3_uint64 framesRead = 0;
        do {
            int16_t sourceData[FRAME_COUNT * 2];
            framesRead = drmp3_read_pcm_frames_s16(&mp3, FRAME_COUNT, sourceData);
            if (framesRead > 0) {
                for (drmp3_uint64 frame = 0; frame < framesRead; frame++) {
                    pcmData[frame * 2] = sourceData[frame * mp3.channels];
                    pcmData[frame * 2 + 1] = mp3.channels > 1
                        ? sourceData[frame * mp3.channels + 1]
                        : sourceData[frame * mp3.channels];
                }
                buf.data_size = framesRead * 2 * sizeof(int16_t);
                if (R_FAILED(audoutAppendAudioOutBuffer(&buf))) break;

                AudioOutBuffer* releasedBuf;
                u32 releasedCount;
                audoutWaitPlayFinish(&releasedBuf, &releasedCount, UINT64_MAX);
            }
        } while (framesRead > 0);

        free(pcmData);
        audoutStopAudioOut();
        audoutExit();
    }

    drmp3_uninit(&mp3);
    fsdevUnmountDevice("sdmc");
    fsExit();
    smExit();

    return 0;
}

#else

// ============================================================================
// GUI BÖLÜMÜ (Arayüz Uygulaması)
// ============================================================================

enum Language { LANG_TR, LANG_EN };
Language currentLang = LANG_TR;
enum Screen { SCREEN_TRACKS, SCREEN_ABOUT };
Screen currentScreen = SCREEN_TRACKS;
std::string selectedTrack = "";
std::vector<std::string> mp3Files;
int currentHover = 0;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

void create_default_config_if_missing() {
    mkdir("sdmc:/BOOTSOUND", 0777);
    std::ifstream check("sdmc:/BOOTSOUND/config.ini");
    if (!check.is_open()) {
        std::ofstream out("sdmc:/BOOTSOUND/config.ini");
        out << "language=TR\nactive_track=none\n";
    }
}

void load_config() {
    std::ifstream file("sdmc:/BOOTSOUND/config.ini");
    if (!file.is_open()) return;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("language=", 0) == 0) {
            std::string l = line.substr(9);
            currentLang = (l == "EN") ? LANG_EN : LANG_TR;
        } else if (line.rfind("active_track=", 0) == 0) {
            selectedTrack = line.substr(13);
        }
    }
}

void save_config() {
    std::ofstream out("sdmc:/BOOTSOUND/config.ini");
    out << "language=" << (currentLang == LANG_TR ? "TR" : "EN") << "\n";
    out << "active_track=" << selectedTrack << "\n";
}

void scan_mp3_files() {
    mp3Files.clear();
    DIR* dir = opendir("sdmc:/BOOTSOUND");
    if (dir) {
        struct dirent* ent;
        while ((ent = readdir(dir)) != NULL) {
            std::string name = ent->d_name;
            if (has_mp3_extension(name)) {
                mp3Files.push_back(name);
            }
        }
    }
    std::sort(mp3Files.begin(), mp3Files.end());
    if (mp3Files.empty()) currentHover = 0;
    else if (currentHover >= (int)mp3Files.size()) currentHover = (int)mp3Files.size() - 1;
}

void draw_text(const std::string& text, int x, int y, int size, SDL_Color color) {
    if (!font || text.empty()) return;
    TTF_SetFontSize(font, size);
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect destination = {x, y, surface->w, surface->h};
    SDL_FreeSurface(surface);
    if (texture) {
        SDL_RenderCopy(renderer, texture, NULL, &destination);
        SDL_DestroyTexture(texture);
    }
}

void draw_panel(const SDL_Rect& panel, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &panel);
}

void draw_ui() {
    const SDL_Color white = {235, 240, 245, 255};
    const SDL_Color muted = {145, 160, 175, 255};
    const SDL_Color cyan = {42, 196, 205, 255};
    const SDL_Color green = {80, 205, 130, 255};
    const SDL_Color background = {17, 24, 32, 255};
    SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, 255);
    SDL_RenderClear(renderer);
    draw_panel({0, 0, 1280, 86}, {25, 35, 46, 255});
    draw_text("BOOTSOUND", 44, 22, 34, white);
    draw_text("CROX", 1090, 28, 22, cyan);
    draw_text("Nintendo Switch boot audio", 44, 58, 14, muted);
    draw_panel({0, 86, 250, 634}, {22, 30, 40, 255});
    draw_panel({24, 132, 202, 58}, currentScreen == SCREEN_TRACKS ? SDL_Color{34, 125, 142, 255} : SDL_Color{22, 30, 40, 255});
    draw_panel({24, 202, 202, 58}, currentScreen == SCREEN_ABOUT ? SDL_Color{34, 125, 142, 255} : SDL_Color{22, 30, 40, 255});
    draw_text(currentLang == LANG_TR ? "Sesler" : "Sounds", 50, 149, 21, white);
    draw_text(currentLang == LANG_TR ? "Hakkinda" : "About", 50, 219, 21, white);
    draw_text("A Sec   B Geri", 44, 650, 15, muted);
    draw_text("X Dil   + Cikis", 44, 676, 15, muted);
    if (currentScreen == SCREEN_ABOUT) {
        draw_text(currentLang == LANG_TR ? "Hakkinda" : "About", 300, 136, 32, white);
        draw_text("BootSound", 300, 206, 26, cyan);
        draw_text("MP3 boot sound manager for Nintendo Switch", 300, 254, 19, muted);
        draw_text(currentLang == LANG_TR ? "Yapimci" : "Developer", 300, 330, 18, muted);
        draw_text("CROX", 300, 360, 28, green);
        draw_text("22.5.0 uyumlulugu icin Atmosphere sysmodule", 300, 430, 17, muted);
        return;
    }
    draw_text(currentLang == LANG_TR ? "Baslangic Sesi" : "Boot Sound", 300, 136, 32, white);
    draw_text(currentLang == LANG_TR ? "Aktif ses" : "Active sound", 300, 190, 16, muted);
    draw_text(selectedTrack.empty() ? (currentLang == LANG_TR ? "Secilmedi" : "Not selected") : selectedTrack, 300, 218, 24, green);
    draw_text(currentLang == LANG_TR ? "MP3 kutuphanesi" : "MP3 library", 300, 290, 20, white);
    if (mp3Files.empty()) {
        draw_panel({300, 340, 820, 88}, {25, 35, 46, 255});
        draw_text(currentLang == LANG_TR ? "BOOTSOUND klasorune MP3 ekleyin" : "Add MP3 files to BOOTSOUND", 330, 370, 18, muted);
    } else {
        for (size_t i = 0; i < mp3Files.size() && i < 7; i++) {
            int y = 330 + (int)i * 48;
            bool selected = (int)i == currentHover;
            draw_panel({300, y, 820, 40}, selected ? SDL_Color{34, 125, 142, 255} : SDL_Color{25, 35, 46, 255});
            draw_text(mp3Files[i], 324, y + 9, 17, selected ? white : muted);
            if (selected) draw_text("A", 1070, y + 9, 17, green);
        }
    }
}

int main(int argc, char **argv) {
    if (romfsInit() != 0 || SDL_Init(SDL_INIT_VIDEO) != 0 || TTF_Init() != 0) return 1;
    window = SDL_CreateWindow("BootSound", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_FULLSCREEN);
    renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED) : NULL;
    font = TTF_OpenFont("romfs:/font.ttf", 24);
    if (!window || !renderer || !font) {
        if (font) TTF_CloseFont(font);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeAny(&pad);

    create_default_config_if_missing();
    load_config();
    scan_mp3_files();

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break;

        if (kDown & HidNpadButton_Y) {
            currentScreen = SCREEN_ABOUT;
        }
        if (kDown & HidNpadButton_B) {
            currentScreen = SCREEN_TRACKS;
        }

        if (currentScreen == SCREEN_ABOUT) {
            draw_ui();
            SDL_RenderPresent(renderer);
            continue;
        }

        if (kDown & HidNpadButton_X) {
            currentLang = (currentLang == LANG_TR) ? LANG_EN : LANG_TR;
            save_config();
        }

        if (!mp3Files.empty()) {
            if (kDown & HidNpadButton_Down) {
                currentHover = (currentHover + 1) % mp3Files.size();
            }
            if (kDown & HidNpadButton_Up) {
                currentHover = (currentHover - 1 + mp3Files.size()) % mp3Files.size();
            }
            if (kDown & HidNpadButton_A) {
                selectedTrack = mp3Files[currentHover];
                save_config();
            }
        }

        draw_ui();
        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    romfsExit();
    return 0;
}

#endif