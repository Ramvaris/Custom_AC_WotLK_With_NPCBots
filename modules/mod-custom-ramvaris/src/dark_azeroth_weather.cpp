/*
 * Dark Azeroth — Biome-aware bad weather system
 * Config: CustomRamvaris.DarkAzeroth.Enable
 *
 * Makes the world feel darker and more atmospheric:
 *   - Hot zones (deserts, volcanic) → Thunderstorms
 *   - Cold zones (snow, tundra, arctic) → Snowfall
 *   - Default zones → Rain
 *
 * Intensity distribution: 5% sunny, 25% light, 30% medium, 40% heavy.
 * Triggers on player login and zone change. Indoor areas ignore weather (client-side).
 * [NO DB] — works with vanilla client, no custom data needed.
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Map.h"
#include "Weather.h"
#include "Config.h"

// ============================================================================
// Hot Zones — Deserts, volcanic, arid regions
// ============================================================================
static const std::unordered_set<uint32> HotZones =
{
    // Eastern Kingdoms
    3, 4, 46, 51,
    // Kalimdor
    400, 405, 440, 976, 1377, 490, 439, 1176, 3428, 3429,
    // Outland
    3483, 3520, 3522, 3523,
};

// ============================================================================
// Cold Zones — Snowy, tundra, arctic regions
// ============================================================================
static const std::unordered_set<uint32> ColdZones =
{
    // Eastern Kingdoms
    1, 36, 2597, 2839, 2959, 2960, 2961, 2964, 2977, 2978,
    3299, 3300, 3301, 3302, 3303, 3304, 3305, 3306,
    // Kalimdor
    618,
    // Northrend + misc
    495, 3537, 65, 66, 67, 210, 2817, 4197, 4742, 4024,
    4418, 4428, 4438, 4440, 4445, 4479, 4495, 4504,
    4510, 4513, 4518,
};

// ============================================================================
// Helpers
// ============================================================================

/// Weighted random intensity: 5% clear, 25% light, 30% medium, 40% heavy
static float GetRandomIntensity()
{
    uint32 roll = urand(1, 100);
    if (roll <= 5)
        return 0.0f;
    if (roll <= 30)
        return 0.6f;
    if (roll <= 60)
        return 0.8f;
    return 1.0f;
}

/// Determines weather type based on zone biome
static WeatherType GetWeatherForZone(uint32 zoneId)
{
    if (HotZones.count(zoneId))
        return WEATHER_TYPE_STORM;
    if (ColdZones.count(zoneId))
        return WEATHER_TYPE_SNOW;
    return WEATHER_TYPE_RAIN;
}

/// Applies weather to a zone for a player's map
static void ApplyWeather(Player* player, uint32 zoneId)
{
    if (!player || !zoneId)
        return;

    Map* map = player->GetMap();
    if (!map)
        return;

    float intensity = GetRandomIntensity();
    WeatherType weatherType = WEATHER_TYPE_FINE;

    if (intensity > 0.0f)
        weatherType = GetWeatherForZone(zoneId);

    Weather* weather = map->GetOrGenerateZoneDefaultWeather(zoneId);
    if (weather)
        weather->SetWeather(weatherType, intensity);
}

// ============================================================================
// PlayerScript — triggers weather on login and zone change
// ============================================================================
class custom_dark_azeroth_weather : public PlayerScript
{
public:
    custom_dark_azeroth_weather() : PlayerScript("custom_dark_azeroth_weather") { }

    void OnPlayerLogin(Player* player) override
    {
        if (!IsEnabled() || !player)
            return;

        uint32 zoneId = player->GetZoneId();
        if (zoneId)
            ApplyWeather(player, zoneId);
    }

    void OnPlayerUpdateZone(Player* player, uint32 newZone, uint32 /*newArea*/) override
    {
        if (!IsEnabled() || !player)
            return;

        if (newZone)
            ApplyWeather(player, newZone);
    }

private:
    static bool IsEnabled()
    {
        return sConfigMgr->GetOption<bool>("CustomRamvaris.Enable", true) &&
               sConfigMgr->GetOption<bool>("CustomRamvaris.DarkAzeroth.Enable", false);
    }
};

void AddSC_dark_azeroth_weather()
{
    new custom_dark_azeroth_weather();
}
