-- Darker Azeroth: Biome-aware bad weather
-- Rain is default everywhere. Hot zones -> Storm. Cold zones -> Snow.
-- Weather is randomized: 5% Sunny, 95% Bad Weather (variable intensity).
-- Indoor areas won't render weather (client ignores it).
-- (C) 2025 Ramires

math.randomseed(os.time())

local WEATHER_FINE  = 0
local WEATHER_RAIN  = 1
local WEATHER_SNOW  = 2
local WEATHER_STORM = 3

-- =========================================================
-- HOT ZONES (deserts, volcanic, arid)
-- =========================================================
local HotZones = {
    [3]=true,[4]=true,[46]=true,[51]=true,            -- EK
    [400]=true,[405]=true,[440]=true,[976]=true,      -- Kalimdor
    [1377]=true,[490]=true,[439]=true,[1176]=true,
    [3428]=true,[3429]=true,
    [3483]=true,[3520]=true,[3522]=true,[3523]=true,  -- Outland
}

-- =========================================================
-- COLD ZONES (snowy, tundra, arctic)
-- =========================================================
local ColdZones = {
    [1]=true,[36]=true,[2597]=true,[2839]=true,       -- EK
    [2959]=true,[2960]=true,[2961]=true,[2964]=true,
    [2977]=true,[2978]=true,[3299]=true,[3300]=true,
    [3301]=true,[3302]=true,[3303]=true,[3304]=true,
    [3305]=true,[3306]=true,
    [618]=true,                                       -- Kalimdor
    [495]=true,[3537]=true,[65]=true,[66]=true,       -- Northrend
    [67]=true,[210]=true,[2817]=true,[4197]=true,
    [4742]=true,[4024]=true,[4418]=true,[4428]=true,
    [4438]=true,[4440]=true,[4445]=true,[4479]=true,
    [4495]=true,[4504]=true,[4510]=true,[4513]=true,
    [4518]=true,
}

-- =========================================================
-- Helpers
-- =========================================================

-- Weighted random intensity:
-- 5% = 0.0 (Clear/Sunny)
-- 25% = 0.6 (Light weather)
-- 30% = 0.8 (Medium weather)
-- 40% = 1.0 (Heavy weather)
local function GetRandomIntensity()
    local roll = math.random(100)
    if roll <= 5 then
        return 0.0
    elseif roll <= 30 then
        return 0.6
    elseif roll <= 60 then
        return 0.8
    else
        return 1.0
    end
end

local function GetWeatherForZone(zoneId)
    if HotZones[zoneId] then
        return WEATHER_STORM
    elseif ColdZones[zoneId] then
        return WEATHER_SNOW
    else
        return WEATHER_RAIN
    end
end

local function ApplyWeather(player, zoneId)
    local map = player:GetMap()
    if not map or not zoneId then return end
    
    local intensity   = GetRandomIntensity()
    local weatherType = WEATHER_FINE

    if intensity > 0.0 then
        weatherType = GetWeatherForZone(zoneId)
    end

    map:SetWeather(zoneId, weatherType, intensity)
end

-- =========================================================
-- Events
-- =========================================================

local function OnZoneUpdate(event, player, newZone, newArea)
    if player and newZone then
        ApplyWeather(player, newZone)
    end
end

local function OnLogin(event, player)
    if not player then return end
    local zoneId = player:GetZoneId()
    if zoneId then
        ApplyWeather(player, zoneId)
    end
end

RegisterPlayerEvent(27, OnZoneUpdate) -- PLAYER_EVENT_ON_UPDATE_ZONE
RegisterPlayerEvent(3,  OnLogin)      -- PLAYER_EVENT_ON_LOGIN
