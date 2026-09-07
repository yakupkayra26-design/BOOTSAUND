#include <switch.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace {

const char* const kRoot = "sdmc:/switch/SwitchTurkceOverlay";
const char* const kInbox = "sdmc:/switch/SwitchTurkceOverlay/inbox.txt";
const char* const kDictionary = "sdmc:/switch/SwitchTurkceOverlay/dictionary.ini";
const char* const kConfig = "sdmc:/switch/SwitchTurkceOverlay/config.ini";

struct Settings {
    bool onlineAi;
    bool autoPatch;
    bool dubbing;
};

struct Subtitle {
    std::string source;
    std::string translated;
};

std::string trim(const std::string& value) {
    size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) ++first;
    size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) --last;
    return value.substr(first, last - first);
}

std::string translate(const std::string& source, const std::map<std::string, std::string>& dictionary) {
    std::map<std::string, std::string>::const_iterator match = dictionary.find(source);
    return match == dictionary.end() ? source : match->second;
}

std::map<std::string, std::string> load_dictionary() {
    std::map<std::string, std::string> dictionary;
    std::ifstream file(kDictionary);
    std::string line;
    while (std::getline(file, line)) {
        size_t separator = line.find('=');
        if (separator == std::string::npos) continue;
        std::string source = trim(line.substr(0, separator));
        std::string target = trim(line.substr(separator + 1));
        if (!source.empty()) dictionary[source] = target;
    }
    return dictionary;
}

Settings load_settings() {
    Settings settings = {true, false, false};
    std::ifstream file(kConfig);
    std::string line;
    while (std::getline(file, line)) {
        size_t separator = line.find('=');
        if (separator == std::string::npos) continue;
        std::string key = trim(line.substr(0, separator));
        bool enabled = trim(line.substr(separator + 1)) == "1";
        if (key == "online_ai") settings.onlineAi = enabled;
        if (key == "auto_patch") settings.autoPatch = enabled;
        if (key == "dubbing") settings.dubbing = enabled;
    }
    return settings;
}

void save_settings(const Settings& settings) {
    std::ofstream file(kConfig);
    file << "online_ai=" << (settings.onlineAi ? 1 : 0) << "\n";
    file << "auto_patch=" << (settings.autoPatch ? 1 : 0) << "\n";
    file << "dubbing=" << (settings.dubbing ? 1 : 0) << "\n";
}

std::vector<Subtitle> load_subtitles(const std::map<std::string, std::string>& dictionary) {
    std::vector<Subtitle> subtitles;
    std::ifstream file(kInbox);
    std::string line;
    while (std::getline(file, line)) {
        size_t separator = line.find('|');
        std::string text = separator == std::string::npos ? line : line.substr(separator + 1);
        text = trim(text);
        if (!text.empty()) {
            Subtitle subtitle = {text, translate(text, dictionary)};
            subtitles.push_back(subtitle);
        }
    }
    const size_t maxVisible = 4;
    if (subtitles.size() > maxVisible) {
        subtitles.erase(subtitles.begin(), subtitles.end() - maxVisible);
    }
    return subtitles;
}

void create_default_files() {
    mkdir(kRoot, 0777);
    std::ifstream dictionary(kDictionary);
    if (!dictionary.good()) {
        std::ofstream file(kDictionary);
        file << "Hello=Merhaba\n";
        file << "Good morning=Gunaydin\n";
        file << "Thank you=Tesekkurler\n";
    }
    std::ifstream config(kConfig);
    if (!config.good()) save_settings(load_settings());
}

void draw(const std::vector<Subtitle>& subtitles, bool paused, const Settings& settings) {
    consoleClear();
    printf("\x1b[1;1H\x1b[1;36m+--------------------------------------+");
    printf("\x1b[2;1H| DIL KOPRUSU  //  SWITCH TURKCE       |");
    printf("\x1b[3;1H+--------------------------------------+\x1b[0m\n");
        printf("\n  [ALTYAZI]  [YAMA]  [DUBLAJ]  [AYAR]\n");
    printf("  Durum: %s\n", paused ? "Durduruldu" : "Dinleniyor");
        printf("  AI: %s   Yama: %s   Dublaj: %s\n", settings.onlineAi ? "Acik" : "Kapali",
            settings.autoPatch ? "Otomatik" : "Elle", settings.dubbing ? "Acik" : "Kapali");
    printf("  Kaynak: %s\n\n", kInbox);
    if (subtitles.empty()) {
        printf("  Beklenen altyazi yok.\n");
        printf("  Adaptoru inbox.txt dosyasina satir eklemeli.\n");
    } else {
        for (std::vector<Subtitle>::const_iterator it = subtitles.begin(); it != subtitles.end(); ++it) {
            printf("  > %s\n", it->translated.c_str());
        }
    }
    printf("\n  A: Yenile   X: Duraklat   Y: AI degistir   +: Cikis\n");
    consoleUpdate(NULL);
}

}

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

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeAny(&pad);
    create_default_files();

    bool paused = false;
    Settings settings = load_settings();
    while (appletMainLoop()) {
        std::map<std::string, std::string> dictionary = load_dictionary();
        std::vector<Subtitle> subtitles = paused ? std::vector<Subtitle>() : load_subtitles(dictionary);
        draw(subtitles, paused, settings);
        padUpdate(&pad);
        u64 buttons = padGetButtonsDown(&pad);
        if (buttons & HidNpadButton_Plus) break;
        if (buttons & HidNpadButton_X) paused = !paused;
        if (buttons & HidNpadButton_Y) {
            settings.onlineAi = !settings.onlineAi;
            save_settings(settings);
        }
        svcSleepThread(100000000ULL);
    }
    consoleExit(NULL);
    return 0;
}