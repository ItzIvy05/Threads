#pragma once

namespace Settings {
    struct Values {
        bool feed = false;
        bool chop = false;
        bool eat = false;
        bool take = false;
        bool equip = false;
        float openDelay = 0.0f;
        int stoneLock = 0;
        float damageDealt = 0.0f;
        float damageTaken = 0.0f;
        float weaponDamage = 0.0f;
        float ammoDamage = 0.0f;
    };

    Values& Get();
    void Load();
    void Save();
}
