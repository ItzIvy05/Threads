#include "ModSettings.h"

namespace {
    RE::BGSKeyword* armorClothing = nullptr;
    RE::BGSKeyword* vendorItemClothing = nullptr;
    RE::TESObjectMISC* leather = nullptr;
    RE::TESObjectMISC* leatherStrips = nullptr;
    RE::TESObjectMISC* threadSpool = nullptr;
    RE::BGSPerk* arcaneBlacksmith = nullptr;
    bool ignoreBackpack = false;
    bool ignoreEnchanted = false;
    bool perkLock = true;
    bool logging = true;
    std::unordered_set<const RE::TESFile*> blacklist;
    std::unordered_set<const RE::TESForm*> blacklistItems;

    void LoadSettings() {
        CSimpleIniA ini;

        if (ini.LoadFile("Data/SKSE/Plugins/Threads.ini") < 0 || !ini.GetValue("Settings", "bIgnoreBackpack") || !ini.GetValue("Settings", "bIgnoreEnchanted") || !ini.GetValue("Settings", "bPerkLock") || !ini.GetValue("Settings", "bLogging")) {
            SKSE::stl::report_and_fail("Data/SKSE/Plugins/Threads.ini is missing or incomplete");
        }

        ignoreBackpack = ini.GetBoolValue("Settings", "bIgnoreBackpack");
        ignoreEnchanted = ini.GetBoolValue("Settings", "bIgnoreEnchanted");
        perkLock = ini.GetBoolValue("Settings", "bPerkLock");
        logging = ini.GetBoolValue("Settings", "bLogging");
    }

    void LoadBlacklist() {
        std::ifstream file{ "Data/SKSE/Plugins/Threads.json" };

        if (!file) {
            SKSE::stl::report_and_fail("Data/SKSE/Plugins/Threads.json is missing");
        }

        const auto dataHandler = RE::TESDataHandler::GetSingleton();

        try {
            const auto json = nlohmann::json::parse(file);

            for (const auto& plugin : json.at("blacklist")) {
                if (const auto mod = dataHandler->LookupModByName(plugin.get<std::string>())) {
                    blacklist.insert(mod);
                }
            }

            for (const auto& item : json.at("blacklistItems")) {
                const auto entry = item.get<std::string>();
                const auto separator = entry.find('~');

                if (separator == std::string::npos) {
                    SKSE::stl::report_and_fail(std::format("Threads.json: \"{}\" must look like 0x800~Plugin.esp", entry));
                }

                if (const auto mod = dataHandler->LookupModByName(entry.substr(separator + 1))) {
                    auto id = std::stoul(entry.substr(0, separator), nullptr, 16) & 0xFFFFFF;

                    if (mod->IsLight()) {
                        id &= 0xFFF;
                    }

                    blacklistItems.insert(dataHandler->LookupForm(id, mod->GetFilename()));
                }
            }
        } catch (const std::exception& e) {
            SKSE::stl::report_and_fail(std::format("Data/SKSE/Plugins/Threads.json: {}", e.what()));
        }
    }

    bool IsLoomClothing(const RE::TESObjectARMO* a_armor) {
        return a_armor->IsClothing() && (a_armor->HasKeyword(armorClothing) || a_armor->HasKeyword(vendorItemClothing)) && !(ignoreBackpack && a_armor->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kModBack)) && !(ignoreEnchanted && a_armor->formEnchanting) && !blacklist.contains(a_armor->GetFile(0)) && !blacklistItems.contains(a_armor);
    }

    std::string_view FileName(const RE::TESForm* a_form, std::int32_t a_index) {
        if (const auto file = a_form->GetFile(a_index)) {
            return file->GetFilename();
        }

        return "created at runtime";
    }

    void SetMaterials(RE::TESContainer& a_items, const RE::TESObjectARMO* a_armor) {
        a_items.ClearDataComponent();

        const auto slots = a_armor->GetSlotMask();

        if (slots.any(RE::BIPED_MODEL::BipedObjectSlot::kBody)) {

            a_items.AddObjectToContainer(leather, 2, nullptr);
            a_items.AddObjectToContainer(leatherStrips, 2, nullptr);
            a_items.AddObjectToContainer(threadSpool, 2, nullptr);

        } else if (slots.any(RE::BIPED_MODEL::BipedObjectSlot::kModChestPrimary, RE::BIPED_MODEL::BipedObjectSlot::kModNeck)) {

            a_items.AddObjectToContainer(threadSpool, 2, nullptr);

        } else if (slots.any(RE::BIPED_MODEL::BipedObjectSlot::kHands, RE::BIPED_MODEL::BipedObjectSlot::kFeet)) {

            a_items.AddObjectToContainer(leather, 2, nullptr);
            a_items.AddObjectToContainer(leatherStrips, 1, nullptr);
            a_items.AddObjectToContainer(threadSpool, 1, nullptr);

        } else if (slots.any(RE::BIPED_MODEL::BipedObjectSlot::kHead, RE::BIPED_MODEL::BipedObjectSlot::kHair, RE::BIPED_MODEL::BipedObjectSlot::kLongHair, RE::BIPED_MODEL::BipedObjectSlot::kCirclet, RE::BIPED_MODEL::BipedObjectSlot::kEars, RE::BIPED_MODEL::BipedObjectSlot::kModMouth, RE::BIPED_MODEL::BipedObjectSlot::kModFaceJewelry)) {

            a_items.AddObjectToContainer(threadSpool, 1, nullptr);

        } else {

            a_items.AddObjectToContainer(leather, 2, nullptr);
            a_items.AddObjectToContainer(threadSpool, 1, nullptr);

        }
    }

    void AddPerkLock(RE::BGSConstructibleObject* a_recipe, const RE::TESObjectARMO* a_armor) {
        if (!perkLock || !a_armor->formEnchanting) {
            return;
        }

        const auto condition = new RE::TESConditionItem;
        condition->data.comparisonValue.f = 1.0f;
        condition->data.functionData.function = RE::FUNCTION_DATA::FunctionID::kHasPerk;
        condition->data.functionData.params[0] = arcaneBlacksmith;
        condition->next = a_recipe->conditions.head;
        a_recipe->conditions.head = condition;
    }

    void ThreadsofTheNorth() {
        const auto dataHandler = RE::TESDataHandler::GetSingleton();
        const auto loom = dataHandler->LookupForm<RE::BGSKeyword>(0x800, "Threads of the North.esp");
        threadSpool = dataHandler->LookupForm<RE::TESObjectMISC>(0x42, "Threads of the North.esp");

        if (!loom) {
            SKSE::stl::report_and_fail("Threads of the North.esp is not loaded");
        }

        // Look up table of the year.
        const auto forge = RE::TESForm::LookupByID<RE::BGSKeyword>(0x88105);
        const auto tanningRack = RE::TESForm::LookupByID<RE::BGSKeyword>(0x7866A);
        const auto armorTable = RE::TESForm::LookupByID<RE::BGSKeyword>(0xADB78);
        const auto nordRace = RE::TESForm::LookupByID<RE::TESRace>(0x13746);
        armorClothing = RE::TESForm::LookupByID<RE::BGSKeyword>(0x6BBE8);
        vendorItemClothing = RE::TESForm::LookupByID<RE::BGSKeyword>(0x8F95B);
        leather = RE::TESForm::LookupByID<RE::TESObjectMISC>(0xDB5D2);
        leatherStrips = RE::TESForm::LookupByID<RE::TESObjectMISC>(0x800E4);
        arcaneBlacksmith = RE::TESForm::LookupByID<RE::BGSPerk>(0x5218E);
        LoadBlacklist();

        auto& recipes = dataHandler->GetFormArray<RE::BGSConstructibleObject>();
        std::unordered_set<RE::TESForm*> crafted;
        std::unordered_set<RE::TESForm*> leveled;
        std::map<std::string_view, std::uint32_t> skipped;
        std::map<std::string_view, std::uint32_t> moved;
        std::map<std::string_view, std::uint32_t> created;

        for (const auto list : dataHandler->GetFormArray<RE::TESLevItem>()) {
            for (const auto& entry : list->entries) {
                leveled.insert(entry.form);
            }
        }

        for (const auto recipe : recipes) {
            const auto armor = skyrim_cast<RE::TESObjectARMO*>(recipe->createdItem);

            if (recipe->benchKeyword != armorTable) {
                crafted.insert(recipe->createdItem);
            }

            if (armor && IsLoomClothing(armor)) {
                if (recipe->benchKeyword == forge || recipe->benchKeyword == tanningRack) {
                    recipe->benchKeyword = loom;
                    SetMaterials(recipe->requiredItems, armor);
                    AddPerkLock(recipe, armor);

                    if (logging) {
                        ++moved[FileName(recipe, 0)];
                    }
                } else if (logging && recipe->benchKeyword == loom) {
                    ++skipped[FileName(recipe, -1)];
                }
            }
        }

        const auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::BGSConstructibleObject>();

        for (const auto armor : dataHandler->GetFormArray<RE::TESObjectARMO>()) {
            if (armor->GetPlayable() && armor->GetFullNameLength() > 0 && !armor->templateArmor && IsLoomClothing(armor) && armor->GetArmorAddon(nordRace) && leveled.contains(armor) && !crafted.contains(armor)) {
                const auto recipe = factory->Create();
                recipe->benchKeyword = loom;
                recipe->createdItem = armor;
                recipe->data.numConstructed = 1;
                SetMaterials(recipe->requiredItems, armor);
                AddPerkLock(recipe, armor);
                recipes.push_back(recipe);

                if (logging) {
                    ++created[FileName(armor, 0)];
                }
            }
        }

        for (const auto& [file, count] : skipped) {
            logger::info("Skipped {} recipes already at the loom from {}", count, file);
        }

        for (const auto& [file, count] : moved) {
            logger::info("Moved {} recipes to the loom from {}", count, file);
        }

        for (const auto& [file, count] : created) {
            logger::info("Created {} loom recipes for items from {}", count, file);
        }
    }

    void OnMessgae(SKSE::MessagingInterface::Message* a_message) {
        if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
            ThreadsofTheNorth();
            Menu::Install();
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    LoadSettings();
    SKSE::Init(skse, { .logPattern = "[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] [%s:%#] %v" });

    if (!logging) {
        spdlog::set_level(spdlog::level::off);
    }

    SKSE::GetMessagingInterface()->RegisterListener(OnMessgae);
    return true;
}