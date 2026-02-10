-- (C)2025 by Ramires

local SPECIAL_MENU_ID = 99999
local SMSG_NPC_TEXT_UPDATE = 384
local MAX_GOSSIP_TEXT_OPTIONS = 8

-- Helper to set Gossip Text (copied from lilly.lua for consistency)
function Player:GossipSetText(text, textID)
    local data = CreatePacket(SMSG_NPC_TEXT_UPDATE, 100);
    data:WriteULong(textID or 0x7FFFFFFF)
    for i = 1, MAX_GOSSIP_TEXT_OPTIONS do
        data:WriteFloat(0) -- Probability
        data:WriteString(text) -- Text
        data:WriteString(text) -- Text
        data:WriteULong(0) -- language
        data:WriteULong(0) -- emote
        data:WriteULong(0) -- emote
        data:WriteULong(0) -- emote
        data:WriteULong(0) -- emote
        data:WriteULong(0) -- emote
        data:WriteULong(0) -- emote
    end
    self:SendPacket(data)
end

-- Table of Summonable NPCs
local summonMap = {
    [1] = { id = 290011, name = "Ling (Reagent Bank)" },
    [2] = { id = 299902, name = "Ashari (Vendor)" },
    [3] = { id = 299900, name = "Lilly (Helper)" },
    [4] = { id = 299901, name = "Lord Squeak (Emblems)" },
    [5] = { id = 300000, name = "Cromi (Instance Reset)" },
    [6] = { id = 299903, name = "Ciel (Mounts)" },
    [7] = { id = 200002, name = "Rename (Pet Renamer)" },
    [8] = { id = 190011, name = "Warpweaver (Transmog)" }
}

local function OnSpecialGossipSelect(event, player, object, sender, intid, code, menu_id)
    if intid == 100 then
        player:SendShowBank(player)
    elseif intid == 101 then
        player:SendShowMailBox(player:GetGUID())
    else
        local selection = summonMap[intid]
        if selection then
            local x = player:GetX()
            local y = player:GetY()
            local z = player:GetZ()
            local o = player:GetO()
            player:SpawnCreature(selection.id, x, y, z, o, 3, 60000) -- Spawn for 60 seconds
        end
    end
    player:GossipComplete()
end

local function CommandHandlerFunction(event, player, command)
  if command == "special" then
    player:GossipClearMenu()
    for i, npc in ipairs(summonMap) do
        player:GossipMenuAddItem(0, "Summon " .. npc.name, 0, i)
    end
    player:GossipMenuAddItem(0, "Open Bank", 0, 100)
    player:GossipMenuAddItem(0, "Open Mailbox", 0, 101)
    
    player:GossipSetText("Select a service or an NPC to summon for 60 seconds:")
    player:GossipSendMenu(0x7FFFFFFF, player, SPECIAL_MENU_ID) -- Sending menu with ID associated to player
    return false
  elseif command == "mountup" then
    local ridingLevel = player:GetSkillValue(762)
    local currentMapId = player:GetMapId()

    -- If already mounted, dismount
    if player:IsMounted() then
        player:Dismount()
        player:CastSpell(self, 81003, false)
        return false
    end

    -- List of maps where flying is normally allowed
    local flyingMaps = {
        [0] = true, [1] = true, [530] = true, [571] = true,
        [401] = true, [443] = true, [461] = true,
        [482] = true, [512] = true, [540] = true
    }

    local canFly = false

    -- Check if flying is allowed on this map
    if flyingMaps[currentMapId] then
        if currentMapId == 571 then
            -- Northrend: requires Cold Weather Flying
            canFly = player:HasSpell(54197)
        else
            canFly = true
        end
    end

    if canFly then
        if ridingLevel >= 300 then
            -- 300% flying mount
            player:CastSpell(self, 42668, true)
        elseif ridingLevel >= 225 then
            -- 150% flying mount
            player:CastSpell(self, 42667, true)
        else
            -- fallback ground mount (100% or 60%)
            player:CastSpell(self, 42683, true)
        end
    else
        -- Ground mount only
        if ridingLevel >= 100 then
            -- 100% ground mount
            player:CastSpell(self, 42683, true)
        else
            -- 60% ground mount
            player:CastSpell(self, 42683, true)
        end
    end

    return false
  end
  
  return true
end

RegisterPlayerGossipEvent(SPECIAL_MENU_ID, 2, OnSpecialGossipSelect) -- GOSSIP_EVENT_ON_SELECT
RegisterPlayerEvent(42, CommandHandlerFunction)