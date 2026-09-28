#include "ModSettings.h"

#include <shellapi.h>

namespace {
    constexpr auto kGroup = "Threads of the North";
    constexpr auto kDescription = "Threads of the North adds looms across Skyrim, moves clothing crafting out of tanning racks, restores missing vanilla clothing recipes.";
    constexpr auto kIni = "Data/SKSE/Plugins/Threads.ini";

    void Field(const char* a_label, const char* a_value) {
        FUCK::LeftLabel(a_label);
        FUCK::TextDisabled("%s", a_value);
    }

    class InfoPage : public FUCK::ITool {
    public:
        const char* Name() const override {
            return "Mod Info";
        }

        const char* Group() const override {
            return kGroup;
        }

        void Draw() override {
            static const auto log = SKSE::log::log_directory().value_or(std::filesystem::path{}) / std::format("{}.log", SKSE::GetPluginName());
            const auto version = SKSE::GetPluginVersion();
            FUCK::SeparatorText("Mod Info");
            Field("Name", kGroup);
            Field("Version", std::format("{}.{}.{}", version.major(), version.minor(), version.patch()).c_str());
            Field("Author", SKSE::GetPluginAuthor().data());

            FUCK::SeparatorText("Description");
            FUCK::TextColoredWrapped(FUCK::GetStyleColorVec4(ImGuiCol_TextDisabled), "%s", kDescription);

            FUCK::SeparatorText("Resources");

            if (FUCK::Button("View Log File")) {
                ShellExecuteW(nullptr, L"open", log.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            }
            FUCK::SetTooltip(log.string().c_str());
            FUCK::SameLine();

            if (FUCK::Button("GitHub")) {
                ShellExecuteA(nullptr, "open", "https://github.com/ItzIvy05/Threads", nullptr, nullptr, SW_SHOWNORMAL);
            }

            FUCK::SameLine();
            if (FUCK::Button("Nexus")) {
                ShellExecuteA(nullptr, "open", "https://www.nexusmods.com/skyrimspecialedition/mods/175676", nullptr, nullptr, SW_SHOWNORMAL);
            }
            FUCK::SeparatorText("Debug");

            if (FUCK::Checkbox("Enable Logging", &logging)) {
                Save();
            }
            FUCK::SameLine();
            FUCK::HelpMarker("Writes Threads.log. Takes effect the next time you start the game.");
        };

        void Load() {
            CSimpleIniA ini;
            ini.LoadFile(kIni);
            logging = ini.GetBoolValue("Settings", "bLogging");
        }

    private:
        void Save() {
            CSimpleIniA ini;
            ini.SetSpaces(false);
            ini.LoadFile(kIni);
            ini.SetLongValue("Settings", "bLogging", logging);
            ini.SaveFile(kIni);
        }
        bool logging = true;
    };
    InfoPage g_settingsPage;

    class SettingsPage : public FUCK::ITool {
    public:
        const char* Name() const override {
            return "Settings";
        }

        const char* Group() const override {
            return kGroup;
        }

        void Draw() override {
            FUCK::SeparatorText("Recipes");

            if (FUCK::Checkbox("Ignore Backpacks", &ignoreBackpack)) {
                Save();
            }
            FUCK::SameLine();
            FUCK::HelpMarker("Backpacks keeps its own recipes.");

            if (FUCK::Checkbox("Enable Enchanted", &enchanted)) {
                Save();
            }
            FUCK::SameLine();
            FUCK::HelpMarker("Enchanted clothing moves to the loom and gets loom recipes.");

            FUCK::BeginDisabled(!enchanted);

            if (FUCK::Checkbox("Require Arcane Blacksmith", &perkLock)) {
                Save();
            }

            FUCK::EndDisabled();
            FUCK::SameLine();
            FUCK::HelpMarker("Enchanted recipes only show up at the loom once you have the Arcane Blacksmith perk.");
        }

        void Load() {
            CSimpleIniA ini;
            ini.LoadFile(kIni);
            ignoreBackpack = ini.GetBoolValue("Settings", "bIgnoreBackpack");
            enchanted = !ini.GetBoolValue("Settings", "bIgnoreEnchanted");
            perkLock = ini.GetBoolValue("Settings", "bPerkLock");
        }

    private:
        void Save() {
            CSimpleIniA ini;
            ini.SetSpaces(false);
            ini.LoadFile(kIni);
            ini.SetLongValue("Settings", "bIgnoreBackpack", ignoreBackpack);
            ini.SetLongValue("Settings", "bIgnoreEnchanted", !enchanted);
            ini.SetLongValue("Settings", "bPerkLock", perkLock);
            ini.SaveFile(kIni);
        }

        bool ignoreBackpack = true;
        bool enchanted = false;
        bool perkLock = true;
    };

    SettingsPage g_settings;
}

void Menu::Install() {
    if (!FUCK::Connect(SKSE::GetPluginName().data())) {
        logger::info("FLICK is not installed, so the menu is turned off");
        return;
    }

    g_settingsPage.Load();
    FUCK::RegisterTool(&g_settingsPage);
    g_settings.Load();
    FUCK::RegisterTool(&g_settings);
}