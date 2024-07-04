
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <regex>
#include <array>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <limits.h>

// Vindusstørrelse
const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 900;
const int MENU_WIDTH = 200;
const int HEADER_HEIGHT = 150;
const int MARGIN = 10;
const int LINE_HEIGHT = 30;
const int BOX_SIZE = 20;
const int SCROLL_SPEED = 10;

// Strukturer for sider
struct Page {
    std::string title;
    std::string content;
};

// Strukturer for tekstfelt
struct TextField {
    std::string label;
    std::string value;
    int x, y, width, height;
    bool active;
};

struct DropdownField {
    std::string label;
    std::vector<std::string> options;
    int selectedIndex;
    int x, y, width, height;
    bool active;
};

// Globale variabler
std::string selectedDisk; // Lagre valgt disk
std::string selectedPreset; // Lagre valgt forhåndsinnstilling
bool encryptionEnabled = false;
bool validEmail = false;
std::string authStatus = "Checking auth status";
int scrollOffset = 0;
int animationFrame = 0;
Uint32 lastAnimationTime = 0;

// Brukerinformasjonsfelter
std::vector<TextField> textFields = {
    {"Hostname", "", MENU_WIDTH + MARGIN + 20, HEADER_HEIGHT + 100, 300, 40, false},
    {"Username", "", MENU_WIDTH + MARGIN + 20, HEADER_HEIGHT + 180, 300, 40, false},
    {"Password", "", MENU_WIDTH + MARGIN + 20, HEADER_HEIGHT + 260, 300, 40, false},
    {"Encryption Key", "", MENU_WIDTH + MARGIN + 20, HEADER_HEIGHT + 640, 300, 40, false},
    {"Github Username", "", MENU_WIDTH + MARGIN + 20, HEADER_HEIGHT + 820, 300, 40, false},
    {"Github E-mail", "", MENU_WIDTH + MARGIN + 20, HEADER_HEIGHT + 900, 300, 40, false},
    {"SSH Github Key", "", MENU_WIDTH + MARGIN + 20, HEADER_HEIGHT + 980, 300, 40, false}
};

std::vector<DropdownField> dropdownFields = {
    {"Keyboard Layout", {"US", "UK", "DE"}, 0, MENU_WIDTH + MARGIN + 20, HEADER_HEIGHT + 340, 300, 40, false},
    {"Country", {"Norway", "Sweden", "Denmark"}, 0, MENU_WIDTH + MARGIN + 20, HEADER_HEIGHT + 420, 300, 40, false}
};

// Funksjonsdeklarasjoner
void drawText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color, bool bold = false, int wrapLength = 0);
void drawPage(SDL_Renderer* renderer, TTF_Font* font, Page& page);
std::vector<std::string> getDisks();
TTF_Font* loadFont(const std::string& fontPath, int fontSize);
void drawMenu(SDL_Renderer* renderer, TTF_Font* font, int currentPage);
void drawHeader(SDL_Renderer* renderer, TTF_Font* font, const std::string& headerText);
void drawLines(SDL_Renderer* renderer);
void drawCheckbox(SDL_Renderer* renderer, int x, int y, bool checked);
bool pointInRect(int x, int y, SDL_Rect& rect);
bool validateEmail(const std::string& email);
void cloneAndSelectPreset(const std::string& preset);
void runInstallScript(const std::string& disk, const std::string& preset);
void drawTextField(SDL_Renderer* renderer, TTF_Font* font, TextField& field, int yOffset, bool showCursor);
void drawDropdownField(SDL_Renderer* renderer, TTF_Font* font, DropdownField& field, int yOffset);
void adjustTextFieldPositions();
void handleScrolling(SDL_Event& e);
int getTextHeight(TTF_Font* font, const std::string& text, int wrapLength);
void updateAuthStatusAnimation(Uint32 currentTime);
void drawClippedRect(SDL_Renderer* renderer, SDL_Rect& rect);

// Funksjoner
void drawText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color, bool bold, int wrapLength) {
    if (bold) {
        TTF_SetFontStyle(font, TTF_STYLE_BOLD);
    } else {
        TTF_SetFontStyle(font, TTF_STYLE_NORMAL);
    }

    SDL_Surface* surface;
    if (wrapLength > 0) {
        surface = TTF_RenderText_Blended_Wrapped(font, text.c_str(), color, wrapLength);
    } else {
        surface = TTF_RenderText_Blended(font, text.c_str(), color);
    }

    if (surface == nullptr || surface->w == 0) {
        std::cerr << "TTF_RenderText_Blended_Wrapped Error: " << TTF_GetError() << std::endl;
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dest = { x, y, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

int getTextHeight(TTF_Font* font, const std::string& text, int wrapLength) {
    int w, h;
    TTF_SizeText(font, text.c_str(), &w, &h);
    if (wrapLength > 0) {
        SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(font, text.c_str(), {255, 255, 255, 255}, wrapLength);
        h = surface->h;
        SDL_FreeSurface(surface);
    }
    return h;
}

void drawPage(SDL_Renderer* renderer, TTF_Font* font, Page& page) {
    SDL_Color textColor = { 255, 255, 255, 255 };
    int y = HEADER_HEIGHT + 20 - scrollOffset;
    drawText(renderer, font, page.title, MENU_WIDTH + MARGIN, y, textColor, true);
    y += 50;
    drawText(renderer, font, page.content, MENU_WIDTH + MARGIN, y, textColor, false, WINDOW_WIDTH - MENU_WIDTH - 2 * MARGIN);
}

std::vector<std::string> getDisks() {
    std::vector<std::string> disks;
    std::ifstream file("/proc/partitions");
    std::string line;
    std::regex disk_regex("^\\s*[0-9]+\\s+[0-9]+\\s+[0-9]+\\s+([a-z]+)$");
    std::smatch match;

    while (std::getline(file, line)) {
        if (std::regex_search(line, match, disk_regex)) {
            std::string disk = match[1];
            if (disk.find('s') == 0 && !std::isdigit(disk.back())) {
                std::string size_cmd = "lsblk -d -o SIZE -n /dev/" + disk;
                std::array<char, 128> buffer;
                std::string result;
                std::shared_ptr<FILE> pipe(popen(size_cmd.c_str(), "r"), pclose);
                if (pipe) {
                    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                        result += buffer.data();
                    }
                }
                result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
                disks.push_back("/dev/" + disk + " - Size: " + result);
            }
        }
    }

    return disks;
}

TTF_Font* loadFont(const std::string& fontPath, int fontSize) {
    std::cerr << "Loading font from: " << fontPath << std::endl;
    if (std::filesystem::exists(fontPath)) {
        TTF_Font* font = TTF_OpenFont(fontPath.c_str(), fontSize);
        if (font != nullptr) {
            return font;
        } else {
            std::cerr << "TTF_OpenFont Error: " << TTF_GetError() << std::endl;
        }
    } else {
        std::cerr << "Font file does not exist: " << fontPath << std::endl;
    }

    return nullptr;
}

std::string getExecutablePath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) {
        return std::string(result, count);
    }
    return "";
}

void drawMenu(SDL_Renderer* renderer, TTF_Font* font, int currentPage) {
    const std::vector<std::string> menuItems = { "Select Drive", "Select preset", "Your info", "Install" };
    int y = HEADER_HEIGHT + 40; // Start litt lavere for jevn avstand

    for (int i = 0; i < menuItems.size(); ++i) {
        SDL_Color textColor = (i == currentPage) ? SDL_Color{ 255, 165, 0, 255 } : SDL_Color{ 255, 255, 255, 255 };
        drawText(renderer, font, menuItems[i], MARGIN, y, textColor, false, MENU_WIDTH - 2 * MARGIN);
        y += 60; // Økt avstand mellom knappene
    }
}

void drawHeader(SDL_Renderer* renderer, TTF_Font* font, const std::string& headerText) {
    SDL_Color textColor = { 255, 255, 255, 255 };
    drawText(renderer, font, headerText, MENU_WIDTH + MARGIN, 50, textColor, true);
}

void drawLines(SDL_Renderer* renderer) {
    // Draw vertical line for the menu with fading ends
    for (int i = 0; i < WINDOW_HEIGHT; ++i) {
        int colorValue = 255 - static_cast<int>((255.0 / WINDOW_HEIGHT) * i);
        SDL_SetRenderDrawColor(renderer, colorValue, colorValue, colorValue, 255);
        SDL_RenderDrawPoint(renderer, MENU_WIDTH, i);
    }

    // Draw horizontal line for the header with fading ends
    for (int i = 0; i < WINDOW_WIDTH; ++i) {
        int colorValue = 255 - static_cast<int>((255.0 / WINDOW_WIDTH) * i);
        SDL_SetRenderDrawColor(renderer, colorValue, colorValue, colorValue, 255);
        SDL_RenderDrawPoint(renderer, i, HEADER_HEIGHT);
    }
}

void drawCheckbox(SDL_Renderer* renderer, int x, int y, bool checked) {
    SDL_Rect box = { x, y, BOX_SIZE, BOX_SIZE };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &box);
    if (checked) {
        SDL_RenderDrawLine(renderer, x, y, x + BOX_SIZE, y + BOX_SIZE);
        SDL_RenderDrawLine(renderer, x + BOX_SIZE, y, x, y + BOX_SIZE);
    }
}

bool pointInRect(int x, int y, SDL_Rect& rect) {
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

bool validateEmail(const std::string& email) {
    const std::regex pattern("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+");
    return std::regex_match(email, pattern);
}

void cloneAndSelectPreset(const std::string& preset) {
    std::string command = "rm -rf /tmp/nixum_config && git clone https://github.com/aCeTotal/nixum_config /tmp/nixum_config";
    system(command.c_str());

    std::string hostsPath = "/tmp/nixum_config/hosts";
    for (const auto& entry : std::filesystem::directory_iterator(hostsPath)) {
        if (entry.is_directory() && entry.path().filename() != preset) {
            std::filesystem::remove_all(entry.path());
        }
    }
}

void runInstallScript(const std::string& disk, const std::string& preset) {
    cloneAndSelectPreset(preset);

    std::string script =
        "#!/bin/bash\n"
        "DISK=\"" + disk + "\"\n"
        "sudo wipefs -af \"$DISK\" &>/dev/null\n"
        "sudo sgdisk -Zo \"$DISK\" &>/dev/null\n"
        "sudo parted -s \"$DISK\" mklabel gpt\n"
        "sudo parted -s \"$DISK\" mkpart NIXBOOT fat32 1MiB 513MiB\n"
        "sudo parted -s \"$DISK\" set 1 esp on\n"
        "sudo parted -s \"$DISK\" mkpart NIXROOT 513MiB 100%\n"
        "NIXBOOT=\"/dev/disk/by-partlabel/NIXBOOT\"\n"
        "NIXROOT=\"/dev/disk/by-partlabel/NIXROOT\"\n"
        "sudo mkfs.fat -F 32 \"$NIXBOOT\"\n"
        "sudo mkfs.btrfs -f \"$NIXROOT\"\n"
        "sudo mkdir -p /mnt\n"
        "sudo mount \"$NIXROOT\" /mnt\n"
        "subvols=(root home nix log)\n"
        "for subvol in \"\" \"${subvols[@]}\"; do\n"
        "    sudo btrfs su cr /mnt/@\"$subvol\"\n"
        "done\n"
        "sudo umount -l /mnt\n"
        "sudo mount -t btrfs -o subvol=@root,defaults,noatime,compress=zstd,discard=async,ssd \"$NIXROOT\" /mnt\n"
        "sudo mkdir -p /mnt/{home,nix,var/log,boot} &>/dev/null\n"
        "sudo mount -t btrfs -o subvol=@home,defaults,noatime,compress=zstd,discard=async,ssd \"$NIXROOT\" /mnt/home\n"
        "sudo mount -t btrfs -o subvol=@nix,defaults,noatime,compress=zstd,discard=async,ssd \"$NIXROOT\" /mnt/nix\n"
        "sudo mount -t btrfs -o subvol=@log,defaults,noatime,compress=zstd,discard=async,ssd \"$NIXROOT\" /mnt/var/log\n"
        "sudo mount -t vfat -o defaults,noatime,rw,fmask=0137,dmask=0027 \"$NIXBOOT\" /mnt/boot\n"
        "sudo nixos-generate-config --root /mnt\n";

    std::string scriptPath = "/tmp/install_script.sh";
    std::ofstream scriptFile(scriptPath);
    scriptFile << script;
    scriptFile.close();

    std::string command = "bash " + scriptPath;
    system(command.c_str());
}

void drawTextField(SDL_Renderer* renderer, TTF_Font* font, TextField& field, int yOffset, bool showCursor) {
    SDL_Color color = { 255, 255, 255, 255 };
    SDL_Rect rect = { field.x, field.y - yOffset, field.width, field.height };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &rect);
    drawText(renderer, font, field.label, field.x, field.y - 25 - yOffset, color, false);
    if (!field.value.empty()) {
        drawText(renderer, font, field.value, field.x + 5, field.y + 5 - yOffset, color, false);
    }
    if (showCursor && field.active) {
        int textWidth = 0;
        TTF_SizeText(font, field.value.c_str(), &textWidth, nullptr);
        SDL_RenderDrawLine(renderer, field.x + 5 + textWidth, field.y + 5 - yOffset, field.x + 5 + textWidth, field.y + field.height - 5 - yOffset);
    }
}

void drawDropdownField(SDL_Renderer* renderer, TTF_Font* font, DropdownField& field, int yOffset) {
    SDL_Color color = { 255, 255, 255, 255 };
    SDL_Rect rect = { field.x, field.y - yOffset, field.width, field.height };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &rect);
    drawText(renderer, font, field.label, field.x, field.y - 25 - yOffset, color, false);
    drawText(renderer, font, field.options[field.selectedIndex], field.x + 5, field.y + 5 - yOffset, color, false);
}

void adjustTextFieldPositions() {
    int yOffset = HEADER_HEIGHT + 120;
    for (auto& field : textFields) {
        if (field.label == "Encryption Key" && !encryptionEnabled) {
            continue;
        }
        field.y = yOffset;
        yOffset += field.height + 40; // Adding enough space between fields
    }
    for (auto& field : dropdownFields) {
        field.y = yOffset;
        yOffset += field.height + 40; // Adding enough space between fields
    }
}

void handleScrolling(SDL_Event& e) {
    if (e.type == SDL_MOUSEWHEEL) {
        if (e.wheel.y > 0) { // Scroll up
            scrollOffset -= SCROLL_SPEED;
            if (scrollOffset < 0) scrollOffset = 0;
        } else if (e.wheel.y < 0) { // Scroll down
            scrollOffset += SCROLL_SPEED;
        }
    }
}

void updateAuthStatusAnimation(Uint32 currentTime) {
    if (currentTime - lastAnimationTime > 500) {
        animationFrame = (animationFrame + 1) % 4;
        lastAnimationTime = currentTime;
    }
}

void drawClippedRect(SDL_Renderer* renderer, SDL_Rect& rect) {
    SDL_RenderSetClipRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &rect);
    SDL_RenderSetClipRect(renderer, nullptr);
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("My SDL2 App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::string execPath = getExecutablePath();
    std::string execDir = std::filesystem::path(execPath).parent_path();
    std::string fontPath = execDir + "/test.ttf";

    TTF_Font* font = loadFont(fontPath, 24);
    if (font == nullptr) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Velkomstside
    Page welcomePage;
    welcomePage.title = "Welcome to NixumOS";
    welcomePage.content = "NixumOS is a project to make Linux more mainstream and simple. "
                          "NixumOS is based entirely on the Nix ecosystem where everything "
                          "you do - every program or setting, is stored in your own private "
                          "repository on GitHub. This makes your system 100% reproducible, "
                          "which makes your system completely reliable.";

    // De fire sidene
    std::vector<Page> pages(4);
    pages[0].title = "Select Drive";
    pages[0].content = "Available disks:\n";
    auto disks = getDisks();
    for (const auto& disk : disks) {
        pages[0].content += disk + "\n";
    }
    pages[1].title = "Select preset";
    pages[1].content = "1. Desktop\n2. HTPC\n3. Server";
    pages[2].title = "Your info";
    pages[2].content = "Fill in your details:";
    pages[3].title = "Install";
    pages[3].content = "Ready to install NixumOS.";

    int currentPage = -1; // Start på velkomstsiden
    bool quit = false;

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int x = e.button.x;
                int y = e.button.y;
                std::cerr << "Mouse click at (" << x << ", " << y << ")" << std::endl;
                if (x > WINDOW_WIDTH - 50 && y < 50) {
                    quit = true;
                }
                if (x > WINDOW_WIDTH - 150 && y > WINDOW_HEIGHT - 100 && y < WINDOW_HEIGHT - 50) {
                    if (currentPage == -1) {
                        std::cerr << "Transition from welcome page to Select Drive" << std::endl;
                        currentPage = 0;
                    } else if (currentPage < static_cast<int>(pages.size()) - 1) {
                        currentPage++;
                        std::cerr << "Transition to page " << currentPage << std::endl;
                    } else if (currentPage == static_cast<int>(pages.size()) - 1) {
                        runInstallScript(selectedDisk, selectedPreset);
                    }
                }
                if (x > 50 && x < 150 && y > WINDOW_HEIGHT - 100 && y < WINDOW_HEIGHT - 50) {
                    if (currentPage > 0) {
                        currentPage--;
                        std::cerr << "Transition to previous page " << currentPage << std::endl;
                    }
                }
                if (x > MARGIN && x < MENU_WIDTH - MARGIN) {
                    int index = (y - (HEADER_HEIGHT + 40)) / 60;
                    if (index >= 0 && index < static_cast<int>(pages.size())) {
                        currentPage = index;
                    }
                }
                if (currentPage == 0 && x > MENU_WIDTH + MARGIN && y > HEADER_HEIGHT + 70) {
                    int index = (y - (HEADER_HEIGHT + 70)) / LINE_HEIGHT;
                    if (index < static_cast<int>(disks.size())) {
                        selectedDisk = disks[index].substr(0, disks[index].find(" - Size:"));
                        std::cerr << "Selected disk: " << selectedDisk << std::endl;
                    }
                }
                if (currentPage == 1 && x > MENU_WIDTH + MARGIN && y > HEADER_HEIGHT + 70) {
                    int index = (y - (HEADER_HEIGHT + 70)) / LINE_HEIGHT;
                    std::vector<std::string> presets = { "desktop", "htpc", "server" };
                    if (index < static_cast<int>(presets.size())) {
                        selectedPreset = presets[index];
                        std::cerr << "Selected preset: " << selectedPreset << std::endl;
                    }
                }
                if (currentPage == 2) {
                    for (auto& field : textFields) {
                        SDL_Rect rect = { field.x, field.y - scrollOffset, field.width, field.height };
                        if (pointInRect(x, y, rect)) {
                            field.active = true;
                        } else {
                            field.active = false;
                        }
                    }
                    for (auto& field : dropdownFields) {
                        SDL_Rect rect = { field.x, field.y - scrollOffset, field.width, field.height };
                        if (pointInRect(x, y, rect)) {
                            field.active = true;
                        } else {
                            field.active = false;
                        }
                    }
                    SDL_Rect encryptionCheckbox = { MENU_WIDTH + MARGIN + 20, HEADER_HEIGHT + 640 - scrollOffset, BOX_SIZE, BOX_SIZE };
                    if (pointInRect(x, y, encryptionCheckbox)) {
                        encryptionEnabled = !encryptionEnabled;
                    }
                }
            } else if (e.type == SDL_TEXTINPUT) {
                for (auto& field : textFields) {
                    if (field.active) {
                        field.value += e.text.text;
                    }
                }
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_BACKSPACE) {
                    for (auto& field : textFields) {
                        if (field.active && !field.value.empty()) {
                            field.value.pop_back();
                        }
                    }
                } else if (e.key.keysym.sym == SDLK_TAB) {
                    for (size_t i = 0; i < textFields.size(); ++i) {
                        if (textFields[i].active) {
                            textFields[i].active = false;
                            textFields[(i + 1) % textFields.size()].active = true;
                            break;
                        }
                    }
                } else if (e.key.keysym.sym == SDLK_DOWN) {
                    for (auto& field : dropdownFields) {
                        if (field.active) {
                            field.selectedIndex = (field.selectedIndex + 1) % field.options.size();
                        }
                    }
                } else if (e.key.keysym.sym == SDLK_UP) {
                    for (auto& field : dropdownFields) {
                        if (field.active) {
                            field.selectedIndex = (field.selectedIndex - 1 + field.options.size()) % field.options.size();
                        }
                    }
                }
            }

            handleScrolling(e);
        }

        Uint32 currentTime = SDL_GetTicks();
        updateAuthStatusAnimation(currentTime);
        std::string animatedAuthStatus = authStatus;
        for (int i = 0; i < animationFrame; ++i) {
            animatedAuthStatus += ".";
        }

        adjustTextFieldPositions();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        drawLines(renderer);
        drawMenu(renderer, font, currentPage);
        drawHeader(renderer, font, "NixumOS Installer");
        drawText(renderer, font, animatedAuthStatus, WINDOW_WIDTH - 400, 50, { 255, 255, 255, 255 });

        if (currentPage == -1) {
            drawPage(renderer, font, welcomePage);
        } else if (currentPage == 0) {
            int y = HEADER_HEIGHT + 70;
            for (const auto& disk : disks) {
                drawText(renderer, font, disk, MENU_WIDTH + MARGIN + BOX_SIZE + 10, y, { 255, 255, 255, 255 });
                drawCheckbox(renderer, MENU_WIDTH + MARGIN, y, selectedDisk == disk.substr(0, disk.find(" - Size:")));
                y += LINE_HEIGHT;
            }
        } else if (currentPage == 1) {
            int y = HEADER_HEIGHT + 70;
            std::vector<std::string> presets = { "desktop", "htpc", "server" };
            for (const auto& preset : presets) {
                drawText(renderer, font, preset, MENU_WIDTH + MARGIN + BOX_SIZE + 10, y, { 255, 255, 255, 255 });
                drawCheckbox(renderer, MENU_WIDTH + MARGIN, y, selectedPreset == preset);
                y += LINE_HEIGHT;
            }
        } else if (currentPage == 2) {
            drawText(renderer, font, "Your info", MENU_WIDTH + MARGIN, HEADER_HEIGHT + 20 - scrollOffset, { 255, 255, 255, 255 }, true);
            drawText(renderer, font, "Fill in your details:", MENU_WIDTH + MARGIN, HEADER_HEIGHT + 60 - scrollOffset, { 255, 255, 255, 255 });
            for (auto& field : textFields) {
                if (field.label == "Encryption Key" && !encryptionEnabled) {
                    continue;
                }
                drawTextField(renderer, font, field, scrollOffset, currentTime / 500 % 2 == 0);
            }
            for (auto& field : dropdownFields) {
                drawDropdownField(renderer, font, field, scrollOffset);
            }
            SDL_Rect encryptionCheckbox = { MENU_WIDTH + MARGIN + 20, HEADER_HEIGHT + 640 - scrollOffset, BOX_SIZE, BOX_SIZE };
            drawCheckbox(renderer, encryptionCheckbox.x, encryptionCheckbox.y, encryptionEnabled);
            drawText(renderer, font, "Enable Encryption", encryptionCheckbox.x + BOX_SIZE + 10, encryptionCheckbox.y, { 255, 255, 255, 255 });
        } else {
            drawPage(renderer, font, pages[currentPage]);
        }

        SDL_Rect nextButton = { WINDOW_WIDTH - 150, WINDOW_HEIGHT - 100, 100, 50 };
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &nextButton);
        drawText(renderer, font, (currentPage == -1 ? "Let's begin!" : (currentPage == static_cast<int>(pages.size()) - 1 ? "Let's Install" : "Next")), WINDOW_WIDTH - 140, WINDOW_HEIGHT - 90, { 255, 255, 255, 255 });

        if (currentPage > 0) {
            SDL_Rect backButton = { 50, WINDOW_HEIGHT - 100, 100, 50 };
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &backButton);
            drawText(renderer, font, "Back", 80, WINDOW_HEIGHT - 90, { 255, 255, 255, 255 });
        }

        SDL_Rect closeButton = { WINDOW_WIDTH - 50, 0, 50, 50 };
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &closeButton);
        drawText(renderer, font, "X", WINDOW_WIDTH - 35, 10, { 255, 255, 255, 255 });

        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
