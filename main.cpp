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

extern "C" {
void __attribute__((weak)) __appInit(void) {
    smInitialize();
    hidInitialize();
    fsInitialize();
    fsdevMountSdmc();
}

void __attribute__((weak)) __appExit(void) {
    fsdevUnmountAll();
    fsExit();
    hidExit();
    smExit();
}
}

std::vector<std::string> mp3Files;
std::string selectedTrack;
int currentHover = 0;

void loadConfig() {
    std::ifstream file("sdmc:/BOOTSOUND/config.ini");
    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("active_track=", 0) == 0) selectedTrack = line.substr(13);
    }
}

void saveConfig() {
    std::ofstream file("sdmc:/BOOTSOUND/config.ini");
    file << "language=TR\nactive_track=" << selectedTrack << "\n";
}

void scanFiles() {
    mp3Files.clear();
    DIR* directory = opendir("sdmc:/BOOTSOUND");
    if (!directory) return;
    struct dirent* entry;
    while ((entry = readdir(directory)) != NULL) {
        if (has_mp3_extension(entry->d_name)) mp3Files.push_back(entry->d_name);
    }
    closedir(directory);
    std::sort(mp3Files.begin(), mp3Files.end());
    if (mp3Files.empty()) currentHover = 0;
    else if (currentHover >= (int)mp3Files.size()) currentHover = (int)mp3Files.size() - 1;
}

void drawConsole() {
    consoleClear();
    printf("\x1b[1;1H\x1b[1;36mBOOT SOUND\x1b[0m  |  CROX\n");
    printf("\x1b[2;1H----------------------------------------\n\n");
    printf("  Baslangic sesini sec\n");
    printf("  MP3 klasoru: sdmc:/BOOTSOUND/\n\n");
    printf("  Aktif: %s\n\n", selectedTrack.empty() || selectedTrack == "none" ? "Yok" : selectedTrack.c_str());
    if (mp3Files.empty()) {
        printf("  MP3 bulunamadi. SD karta BOOTSOUND klasorune MP3 koy.\n");
    } else {
        for (size_t index = 0; index < mp3Files.size(); index++) {
            printf("  %s %s\n", (int)index == currentHover ? ">" : " ", mp3Files[index].c_str());
        }
    }
    printf("\n  Yukari/Asagi: Sec   A: Kaydet   X: Yenile   +: Cikis\n");
    consoleUpdate(NULL);
}

int main(int argc, char** argv) {
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeAny(&pad);
    mkdir("sdmc:/BOOTSOUND", 0777);
    loadConfig();
    scanFiles();
    drawConsole();
    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 buttons = padGetButtonsDown(&pad);
        if (buttons & HidNpadButton_Plus) break;
        if (!mp3Files.empty() && (buttons & HidNpadButton_Down)) currentHover = (currentHover + 1) % mp3Files.size();
        if (!mp3Files.empty() && (buttons & HidNpadButton_Up)) currentHover = (currentHover + mp3Files.size() - 1) % mp3Files.size();
        if (!mp3Files.empty() && (buttons & HidNpadButton_A)) {
            selectedTrack = mp3Files[currentHover];
            saveConfig();
        }
        if (buttons & HidNpadButton_X) scanFiles();
        drawConsole();
    }
    consoleExit(NULL);
    return 0;
}

#endif