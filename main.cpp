#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <cctype>

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

static bool has_mp3_extension(const std::string& name) {
    if (name.size() < 4) return false;
    std::string extension = name.substr(name.size() - 4);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char character) { return (char)std::tolower(character); });
    return extension == ".mp3";
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

    if (R_SUCCEEDED(audoutInitialize())) {
        audoutStartAudioOut();

        uint32_t channels = mp3.channels;
        if (channels == 0) {
            drmp3_uninit(&mp3);
            audoutExit();
            fsdevUnmountDevice("sdmc");
            fsExit();
            smExit();
            return 0;
        }
        const size_t BUFFER_SIZE = 4096 * 4;
        int16_t* pcmData = (int16_t*)aligned_alloc(0x1000, BUFFER_SIZE * sizeof(int16_t));
        
        AudioOutBuffer buf;
        memset(&buf, 0, sizeof(AudioOutBuffer));
        buf.buffer = pcmData;
        buf.buffer_size = BUFFER_SIZE * sizeof(int16_t);

        drmp3_uint64 framesRead = 0;
        do {
            framesRead = drmp3_read_pcm_frames_s16(&mp3, BUFFER_SIZE / channels, pcmData);
            if (framesRead > 0) {
                buf.data_size = framesRead * channels * sizeof(int16_t);
                audoutAppendAudioOutBuffer(&buf);
                
                AudioOutBuffer* releasedBuf;
                u32 releasedCount;
                audoutWaitPlayFinish(&releasedBuf, &releasedCount, U64_MAX);
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
std::string selectedTrack = "";
std::vector<std::string> mp3Files;
int currentHover = 0;

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
        closedir(dir);
    }
    std::sort(mp3Files.begin(), mp3Files.end());
    if (mp3Files.empty()) {
        currentHover = 0;
    } else if (currentHover >= (int)mp3Files.size()) {
        currentHover = (int)mp3Files.size() - 1;
    }
}

void draw_ui() {
    consoleClear();
    printf("\x1b[1;1H===============================================");
    printf("\x1b[2;1H     BOOTSOUND CONFIGURATOR (V1/V2/OLED)       ");
    printf("\x1b[3;1H===============================================\n\n");

    if (currentLang == LANG_TR) {
        printf(" [X] Dil Degistir: TURKCE\n");
        printf(" [A] Baslangic Sesi Olarak Sec\n");
        printf(" [+ ] Cikis\n\n");
        printf(" Aktif Ses: %s\n\n", selectedTrack.empty() ? "Yok" : selectedTrack.c_str());
        printf("--- BOOTSOUND Klasorundeki MP3 Dosyalari ---\n");
    } else {
        printf(" [X] Change Language: ENGLISH\n");
        printf(" [A] Set as Boot Sound\n");
        printf(" [+ ] Exit\n\n");
        printf(" Active Track: %s\n\n", selectedTrack.empty() ? "None" : selectedTrack.c_str());
        printf("--- MP3 Files in BOOTSOUND Directory ---\n");
    }

    if (mp3Files.empty()) {
        if (currentLang == LANG_TR)
            printf(" -> Hic MP3 bulunamadi! Lutfen sdmc:/BOOTSOUND/ icine MP3 atin.\n");
        else
            printf(" -> No MP3 files found! Please place MP3s in sdmc:/BOOTSOUND/\n");
    } else {
        for (size_t i = 0; i < mp3Files.size(); i++) {
            if ((int)i == currentHover) {
                printf(" > [%s] <\n", mp3Files[i].c_str());
            } else {
                printf("   %s\n", mp3Files[i].c_str());
            }
        }
    }
    consoleUpdate(NULL);
}

int main(int argc, char **argv) {
    consoleInit(NULL);
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
    }

    consoleExit(NULL);
    return 0;
}

#endif