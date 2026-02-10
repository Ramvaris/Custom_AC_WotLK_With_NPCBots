-- (C)2025 by Ramires

-- GLOBAL VARIABLES
local npcId = 299900
local menuId = 299900

local SMSG_NPC_TEXT_UPDATE = 384
local MAX_GOSSIP_TEXT_OPTIONS = 8

-- HELPER FUNCTIONS
 
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

function Player:TeleportService(coordX, coordY, coordZ, orientation, vmap, creature, player, cost)
   player:GossipComplete()
   player:SetCoinage(player:GetCoinage() - cost)
   creature:SendUnitWhisper("Safe travels, $n!", 0, player)
   player:Teleport(vmap, coordX, coordY, coordZ, orientation)
end

-- NPC GOSSIP

local function GossipHello(event, player, unit)
    player:GossipClearMenu()
    player:GossipMenuAddItem(0, "Let me see your services", 0, 1) 
    player:GossipMenuAddItem(0, "Nevermind", 0, 999)
    player:GossipSetText("Greetings, $n.$B$BThe great Ramires gave me permission to help you out on your quest.$B$BHow may I help you today?")
    player:GossipSendMenu(0x7FFFFFFF, unit)
end

local function GossipSelect(event, player, creature, sender, intid, code, menuid)
    if (intid == 1) then
       player:GossipMenuAddItem(0, "I want to use the teleportation service", 0, 3)
       player:GossipMenuAddItem(0, "I want to use the instance reset service", 0, 4)
       player:GossipMenuAddItem(0, "I want to use the exalted special service",0 , 2)
       player:GossipMenuAddItem(0, "I have no need for your services right now", 0, 999)
       player:GossipSetText("I am allowed to offer you the following services.")
       player:GossipSendMenu(0x7FFFFFFF, creature)
    elseif (intid == 2) then
       if (player:GetReputationRank(1161) < 7) then
          player:GossipMenuAddItem(0, "Ok. Maybe another time...", 0, 1)
          player:GossipSetText("Your reputation with our faction is not nearly high enough for this service...")
       else
          if (player:HasItem(60001, 7)) then
             player:GossipMenuAddItem(0, "Great. I want to purchase it!", 0, 57)
             player:GossipMenuAddItem(0, "I think I will pass this time", 0, 1)
             player:GossipSetText("You have enough reputation with our faction and you have the required 7 Emblems of Ramires.$B$BDo you want to purchase a permanent additional talent point for the Emblems?")
          else
             player:GossipMenuAddItem(0, "Ok. I will try to gather them...", 0, 1)
             player:GossipSetText("You do not have enough Emblems of Ramires to use this service.$B$BYou need at least 7!")
          end
       end
       player:GossipSendMenu(0x7FFFFFFF, creature)
    elseif (intid == 3) then
       player:GossipMenuAddItem(0, "I want to travel to a bigger city...", 0, 6)
       player:GossipMenuAddItem(0, "I want to travel to a dungeon...", 0, 8)
       player:GossipMenuAddItem(0, "I want to travel to a point of interest...", 0, 7)
       player:GossipMenuAddItem(0, "I want to use a different service", 0, 1)
       player:GossipSetText("So, let's choose your destination category first!")
       player:GossipSendMenu(0x7FFFFFFF, creature)

    elseif (intid == 4) then
       player:GossipMenuAddItem(0, "Fine with me...", 0, 5)
       player:GossipMenuAddItem(0, "Nevermind...", 0, 1)
       player:GossipSetText("So you want to reset your instances?$B$BThen let's see... how about 5 gold coins?")
       player:GossipSendMenu(0x7FFFFFFF, creature)

    elseif (intid == 5) then
       if (player:GetCoinage() < 50000) then
          player:GossipMenuAddItem(0, "Ok...", 0, 1)
          player:GossipSetText("You do not have enough money to pay for my services.$B$BSo I guess you have to wait or you might come again with a bigger purse.")
       else
          player:GossipMenuAddItem(0, "That's great!", 0, 1)
          player:SetCoinage(player:GetCoinage() - 50000)
	  player:UnbindAllInstances()
          player:GossipSetText("Thank you!$B$BAll of your instances have been reset.")
       end

       player:GossipSendMenu(0x7FFFFFFF, creature)
    elseif (intid == 6) then
       local enoughMoney = false
       local faction = 0

       if (player:GetCoinage() < 1000) then
          player:GossipSetText("You don't seem to be wealthy enough to use my teleportation services for bigger cities.$B$BYou need at least 10 silver coins in your purse!")
       else
          enoughMoney = true
       end

       if (player:IsAlliance()) then
          faction = 1
       elseif (player:IsHorde()) then
          faction = 2
       else
          player:GossipMenuAddItem(0, "I am neither Alliance or Horde... what am I?", 0, 999)
       end
       
       if ((faction == 1) and enoughMoney) then
          player:GossipMenuAddItem(0, "Bring me to Stormwind", 0, 10)
          player:GossipMenuAddItem(0, "Bring me to Ironforge", 0, 11)
          player:GossipMenuAddItem(0, "Bring me to Darnassus", 0, 12)
          player:GossipMenuAddItem(0, "Bring me to the Exodar", 0, 13)
          player:GossipMenuAddItem(0, "Bring me to Shattrath", 0, 14)
          player:GossipMenuAddItem(0, "Bring me to Dalaran", 0, 15)
          player:GossipMenuAddItem(0, "Sorry, but I changed my opinion.", 0, 3)
       elseif ((faction == 2) and enoughMoney) then
          player:GossipMenuAddItem(0, "Bring me to Orgrimmar", 0, 16)
          player:GossipMenuAddItem(0, "Bring me to Thunderbluff", 0, 17)
          player:GossipMenuAddItem(0, "Bring me to Undercity",0, 18)
          player:GossipMenuAddItem(0, "Bring me to Silvermoon",0, 19)
          player:GossipMenuAddItem(0, "Bring me to Shattrath", 0, 14)
          player:GossipMenuAddItem(0, "Bring me to Dalaran", 0, 15)
          player:GossipMenuAddItem(0, "Sorry, but I changed my opinion.", 0, 3)
       end

       if (enoughMoney) then
          player:GossipSetText("A bigger city then!$B$BBut which one exactly?$B$BAnd remember that I will charge 10 silver coins for each travel!")
       else
          player:GossipMenuAddItem(0, "Oh. I will try to get more coinage then.", 0, 3)
       end

       player:GossipSendMenu(0x7FFFFFFF, creature)
    elseif (intid == 7) then
       if (player:GetCoinage() < 200000) then
          player:GossipMenuAddItem(0, "Oh. I will try to get more coinage then.", 0, 3)
          player:GossipSetText("You don't seem to be wealthy enough to use my teleportation services for a point of interest.$B$BYou need at least 20 gold coins in your purse!")
       else
          player:GossipMenuAddItem(0, "Bring me to Old Ironforge", 0, 41)
          player:GossipMenuAddItem(0, "Bring me to Shatterspear Village", 0, 42)
          player:GossipMenuAddItem(0, "Bring me to Wetlands Dwarven Village", 0, 43)
          player:GossipMenuAddItem(0, "Bring me to the Karazhan Crypts", 0, 44)
          player:GossipMenuAddItem(0, "Bring me to Hyjal Blizzard Construction Site", 0, 45)
          player:GossipMenuAddItem(0, "Bring me to the Silithus Southern Farm", 0, 46)
          player:GossipMenuAddItem(0, "Bring me to the Quel'thalas Tower", 0, 47)
          player:GossipMenuAddItem(0, "Bring me to the Arathi Dwarven Farm", 0, 48)
          player:GossipMenuAddItem(0, "Bring me to the Wetlands Hidden Spot", 0, 49)
          player:GossipMenuAddItem(0, "Bring me to the Outsides of Stratholme (Instanced)", 0, 50)
          player:GossipMenuAddItem(0, "Bring me to Ortells Hideout in Dun Morogh", 0, 51)
          player:GossipMenuAddItem(0, "Bring me to Newmans Landing", 0, 52)
          player:GossipMenuAddItem(0, "Bring me to Nessy at the Deeprun Tram", 0, 53)
          player:GossipMenuAddItem(0, "Bring me to the other side of the Deeprun Tram", 0, 54)
          player:GossipMenuAddItem(0, "Bring me to the Emerald Dream Forest", 0, 55)
          player:GossipMenuAddItem(0, "Bring me to the Old Scarlet Monastery", 0, 56)
          
          player:GossipMenuAddItem(0, "I changed my opinion", 0, 3)
          player:GossipSetText("A point of interest then!$B$BBut which one exactly?$B$BRemember that I will charge 20 gold coins for each travel and that I do not guarantee that you can return without your hearthstone!!")
       end

       player:GossipSendMenu(0x7FFFFFFF, creature)
    elseif (intid == 8) then
       if (player:GetCoinage() < 200000) then
          player:GossipMenuAddItem(0, "Oh. I will try to get more coinage then.", 0, 3)
          player:GossipSetText("You don't seem to be wealthy enough to use my teleportation services for a dungeon.$B$BYou need at least 20 gold coins in your purse!")
       else
          if (player:IsAlliance()) then
             player:GossipMenuAddItem(9, "Bring me to the Ragefire Chasm [Dangerous]", 0, 21)
          elseif (player:IsHorde()) then
             player:GossipMenuAddItem(0, "Bring me to the Ragefire Chasm", 0, 22)
          end
          player:GossipMenuAddItem(0, "Bring me to the Wailing Caverns", 0, 23)
          if (player:IsAlliance()) then
             player:GossipMenuAddItem(0, "Bring me to the Stockades", 0, 24)
          elseif (player:IsHorde()) then
             player:GossipMenuAddItem(9, "Bring me to the Stockades [Dangerous]", 0, 25)
          end
	  player:GossipMenuAddItem(0, "Bring me to the Deadmines", 0, 27)
          player:GossipMenuAddItem(0, "Bring me to the Blackfathom Deeps", 0, 28)
          player:GossipMenuAddItem(0, "Bring me to the Shadowfang Keep", 0, 29)
          player:GossipMenuAddItem(0, "Bring me to the Razorfen Kraul", 0, 26)
          player:GossipMenuAddItem(0, "Bring me to the Scarlet Monastery", 0, 30)
          player:GossipMenuAddItem(0, "Bring me to the Gnomeregan", 0, 31)
          player:GossipMenuAddItem(0, "Bring me to the Razorfen Downs", 0, 32)
          player:GossipMenuAddItem(0, "Bring me to the Zul'Farrak", 0, 33)
          player:GossipMenuAddItem(0, "Bring me to the Uldaman", 0, 34)
          player:GossipMenuAddItem(0, "Bring me to the Blackrock Depths", 0, 35)
          player:GossipMenuAddItem(0, "Bring me to the Sunken Temple", 0, 36)
          player:GossipMenuAddItem(0, "Bring me to the Blackrock Spire", 0, 37)
          player:GossipMenuAddItem(0, "Bring me to the Scholomance", 0, 38)
          player:GossipMenuAddItem(0, "Bring me to the Stratholme", 0, 39)
          player:GossipMenuAddItem(0, "Bring me to the Dire Maul East", 0, 40)
          player:GossipMenuAddItem(0, "I changed my opinion", 0, 3)

          player:GossipSetText("A dungeon then!$B$BBut which one exactly?$B$BRemember that I will charge 20 gold coins for each travel and that I will only send you to classic dungeons. You can fly in the Outlands and Northrend!$B$BOh. Before I forget it. Travels to enemy territory cost 100 gold coins.")
       end
      
       player:GossipSendMenu(0x7FFFFFFF, creature)
    elseif (intid == 9) then
    elseif (intid == 10) then
       player:TeleportService(-8833.38, 628.628, 94.0066, 1.06535, 0, creature, player, 1000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 11) then
       player:TeleportService(-4918.88, -940.406, 501.564, 5.42347, 0, creature, player, 1000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 12) then
       player:TeleportService(9949.56, 2284.21, 1341.4, 1.59587, 1, creature, player, 1000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 13) then
       player:TeleportService(-3965.7, -11653.6, -138.844, 0.852154, 530, creature, player, 1000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 14) then
       if (player:GetLevel() < 60) then
          player:GossipSetText("Sorry, but you need to be level 60 to travel to Shattrath.")
          player:GossipMenuAddItem(0, "...", 0, 6)
          player:GossipSendMenu(0x7FFFFFFF, creature)
       else
          player:TeleportService(-1838.16, 5301.79, -12.428, 5.9517, 530, creature, player, 1000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
       end
    elseif (intid == 15) then
       if (player:GetLevel() < 70) then
          player:GossipSetText("Sorry, but you need to be level 70 to travel to Dalaran.")
          player:GossipMenuAddItem(0, "...", 0, 6)
          player:GossipSendMenu(0x7FFFFFFF, creature)
       else
          player:TeleportService(5804.15, 624.771, 647.767, 1.64, 571, creature, player, 1000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
       end
    elseif (intid == 16) then
       player:TeleportService(1629.85, -4373.64, 31.5573, 3.69762, 1, creature, player, 1000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 17) then
       player:TeleportService(-1277.37, 124.804, 131.287, 5.22274, 1, creature, player, 1000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 18) then
       player:TeleportService(1584.14, 240.308, -52.1534, 0.041793, 0, creature, player, 1000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 19) then
       player:TeleportService(9487.69, -7279.2, 14.2866, 6.16478, 530, creature, player, 1000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 20) then
       player:TeleportService(1902.31, -1363.98, 99.987, 0.929737, 1, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 21) then
       if (player:GetCoinage() < 1000000) then
          player:GossipAddItem(0, "Fine... another time then", 0, 8)
          player:GossipSetText("Sorry, you do not have enough coinage to travel to the Ragefire Chasm.")
          player:GossipSendMenu(0x7FFFFFFF, creature)
       else
          player:TeleportService(1811.78, -4410.5, -18.4704, 5.20165, 1, creature, player, 1000000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
       end
    elseif (intid == 22) then
       player:TeleportService(1811.78, -4410.5, -18.4704, 5.20165, 1, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 23) then
       player:TeleportService(-805.049, -2032.03, 95.8796, 6.18912, 1, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 24) then
       player:TeleportService(-8779.9, 834.349, 94.6801, 0.653013, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 25) then
       if (player:GetCoinage() < 1000000) then
          player:GossipAddItem(0, "Fine... another time then", 0, 8)
          player:GossipSetText("Sorry, you do not have enough coinage to travel to the Stockades.")
          player:GossipSendMenu(0x7FFFFFFF, creature)
       else
          player:TeleportService(-8779.9, 834.349, 94.6801, 0.653013, 0, creature, player, 1000000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
       end
    elseif (intid == 26) then
       player:TeleportService(-4470.28, -1677.77, 81.3925, 1.16302, 1, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 27) then
       player:TeleportService(-11208.7, 1673.52, 24.6361, 1.51067, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 28) then
       player:TeleportService(4249.99, 740.102, -25.671, 1.34062, 1, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 29) then
       player:TeleportService(-234.675, 1561.63, 76.8921, 1.24031, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 30) then
       player:TeleportService(2872.6, -764.398, 160.332, 5.05735, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 31) then
       player:TeleportService(-5163.54, 925.423, 257.181, 1.57423, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 32) then
       player:TeleportService(-4657.3, -2519.35, 81.0529, 4.54808, 1, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 33) then
       player:TeleportService(-6801.19, -2893.02, 9.00388, 0.158639, 1, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 34) then
       player:TeleportService(-6071.37, -2955.16, 209.782, 0.015708, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 35) then
       player:TeleportService(-7179.34, -921.212, 165.821, 5.09599, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 36) then
       player:TeleportService(-10177.9, -3994.9, -111.239, 6.01885, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 37) then
       player:TeleportService(-7527.05, -1226.77, 285.732, 5.29626, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 38) then
       player:TeleportService(1269.64, -2556.21, 93.6088, 0.620623, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 39) then
       player:TeleportService(3352.92, -3379.03, 144.782, 6.25978, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 40) then
       player:TeleportService(-3980.8, 789.005, 161.007, 4.71945, 1, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 41) then
       player:TeleportService(-4819.56, -974.088, 464.709, 3.963, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 42) then
       player:TeleportService(7367.77, -1560.74, 163.446, 2.55011, 1, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 43) then
       player:TeleportService(-4031.92, -1411.13, 156.758, 0.429314, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 44) then
       player:TeleportService(-11069.7, -1795.77, 53.7318, 3.09641, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 45) then
       player:TeleportService(5475.99, -3728.73, 1593.44, 5.80284, 1, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 46) then
       player:TeleportService(-10750.5, 2432.04, 6.14444, 0.240715, 1, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 47) then
       player:TeleportService(4296.04, -2763.72, 16.268, 0.51962, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 48) then
       player:TeleportService(-1849.54, -4149.05, 9.8162, 3.07668, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 49) then
       player:TeleportService(-3857, -3485, 579.64, 3.3092, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 50) then
       player:TeleportService(3176.63, -4039.28, 105.464, 3.3092, 329, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 51) then
       player:TeleportService(-5313.68, -2512.7, 484.236, 3.57754, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 52) then
       player:TeleportService(-6376.91, 1262.61, 7.18831, 2.23557, 0, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 53) then
       player:TeleportService(-98.3923, 1216.83, -122.163, 1.48305, 369, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 54) then
       player:TeleportService(74.5078, 1184.51, -119.551, 2.90473, 369, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 55) then
       player:TeleportService(2738.87, -3320.93, 101.917, 0.366472, 169, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 56) then
       player:TeleportService(79, -1, 18.6778, 0, 44, creature, player, 200000) -- X,Y,Z,Orientation,Map,Sender,Player,Cost in Copper
    elseif (intid == 57) then
       player:RemoveItem(60001,7)
	   local freeTalentPointAmt = player:GetFreeTalentPoints()
	   player:SetFreeTalentPoints(freeTalentPointAmt + 1)
       player:GossipMenuAddItem(0, "Thank you and yes I still need something", 0, 1)
       player:GossipMenuAddItem(0, "Thank you and can you help me to re-enter the world?", 0, 997)
       player:GossipMenuAddItem(0, "Thank you and no that was all", 0, 999)
       player:GossipSetText("The great Ramires just granted you another permanent talent point!$B$BBut remember, you need to re-enter this world, level up or fulfill a quest to use and see it.$B$BDo you need some other service?")
       player:GossipSendMenu(0x7FFFFFFF, creature)
    elseif (intid == 997) then
       player:LogoutPlayer(true)
       player:GossipComplete()
    elseif (intid == 998) then
       player:GossipMenuAddItem(0, "What an odd thing to say", 0, 1)
       player:GossipSetText("Sorry, $n.$B$BI think I can't offer you this service right now, even though I proposed it to you.$B$BMaybe you could try again later?")
       player:GossipSendMenu(0x7FFFFFFF, creature)
    elseif (intid == 999) then
       math.randomseed(os.time())
       local goodbye = math.random(1,5)

       if (goodbye == 1) then
           creature:SendUnitSay("Hey! Already enough of me?", 0)
       elseif (goodbye == 2) then
           creature:SendUnitEmote("Lilly shrugs her shoulders.")
       elseif (goodbye == 3) then
           creature:SendUnitWhisper("Do not forget. I will watch and rate your progress!", 0, player)
       elseif (goodbye == 4) then
           creature:SendUnitEmote("Lilly notes something on her imaginary clipboard.")
       else
           creature:SendUnitSay("Have a nice day then.", 0)
       end

       player:GossipComplete()
    end
end

RegisterCreatureGossipEvent(npcId, 1, GossipHello)
RegisterCreatureGossipEvent(npcId, 2, GossipSelect)