-- (C)2025 by Ramires

local MEND_PET_SPELL_ID = 136

local function OnLingSummon(event, creature, summoner)
	math.randomseed(os.time())
    local choice = math.random(1,12)

    if (choice == 1) then
		creature:SendUnitSay("What do you want, " .. summoner:GetName() .. "?",0)
	elseif (choice == 2) then
		creature:SendUnitSay("What if you do my job for a day, " .. summoner:GetName() .. "?",0)
	elseif (choice == 3) then
		creature:SendUnitSay("Let me guess, you want to store some reagents?",0)
	elseif (choice == 4) then
		creature:SendUnitSay("Maybe I am the one saving Azeroth.\n I mean, I carry all this stuff around.",0)
		creature:SendUnitEmote("Ling sighs.")
	elseif (choice == 5) then
		creature:SendUnitSay("It's always the same thing...",0)
	elseif (choice == 6) then
		creature:SendUnitSay("You know, my back is killing me. This bank isn't weightless!",0)
	elseif (choice == 7) then
		creature:SendUnitSay("I was having a lovely dream about a world without adventurers... then you called.",0)
	elseif (choice == 8) then
		creature:SendUnitSay("Can we make this quick? I have... absolutely nothing else to do, but I'd like to pretend.",0)
	elseif (choice == 9) then
		creature:SendUnitSay("Did you wash your hands? I don't want sticky fingerprints on the vault.",0)
	elseif (choice == 10) then
		creature:SendUnitSay("I am sworn to carry your burdens... literally.",0)
		creature:SendUnitEmote("Ling glares at you.")
	elseif (choice == 11) then
		creature:SendUnitSay("Do you have any idea what time it is? ...Actually, neither do I. It's always 'now' here.",0)
	else
		creature:SendUnitSay("Here I am. Again. Hooray.",0)
		creature:SendUnitEmote("Ling rolls her eyes.")
	end
end

local function OnVendorSummon(event, creature, summoner)
	math.randomseed(os.time())
    local choice = math.random(1,12)

    if (choice == 1) then
		creature:SendUnitSay("Oh, come on! I was just polishing my horns!",0)
	elseif (choice == 2) then
		creature:SendUnitSay("Hey " .. summoner:GetName() .. "! You really like me, eh?",0)
	elseif (choice == 3) then
		creature:SendUnitSay("Do you want to buy? To trade?\nI am good at impersonating Draenei.",0)
		creature:SendUnitEmote("Ashari giggles.")
	elseif (choice == 4) then
		creature:SendUnitSay("Ashari has wares, if you have coin!\nJust heard that from a really strange cat guy.",0)
	elseif (choice == 5) then
		creature:SendUnitSay("Ten thousand gold coins... ten thou... oh, hey " .. summoner:GetName() .. "!",0)
	elseif (choice == 6) then
		creature:SendUnitSay("No refunds! ...unless you are very, very persuasive.",0)
		creature:SendUnitEmote("Ashari winks.")
	elseif (choice == 7) then
		creature:SendUnitSay("I found this item... fell off a wagon. You want it?",0)
	elseif (choice == 8) then
		creature:SendUnitSay("Welcome to Ashari's Emporium of Wonders! And by wonders, I mean stuff I found.",0)
	elseif (choice == 9) then
		creature:SendUnitSay("You look like you have too much gold. Let me help you with that burden.",0)
	elseif (choice == 10) then
		creature:SendUnitSay("Don't tell the others, but you get the 'special friend' discount... which is 0% off!",0)
	elseif (choice == 11) then
		creature:SendUnitSay("I saw a goblin earlier. Tried to sell me my own shoes! Can you believe the nerve?",0)
	else
		creature:SendUnitSay("Is that a mirror in your pocket? Because I can see myself in your gold.",0)
		creature:SendUnitEmote("Ashari grins widely.")
	end
end


local function OnLillySummon(event, creature, summoner)
    math.randomseed(os.time())
    local choice = math.random(1, 12)
    if choice == 1 then
        creature:SendUnitSay("Greetings, " .. summoner:GetName() .. ". How may I serve the will of Ramires today?", 0)
        creature:SendUnitEmote("Lilly bows gracefully.")
    elseif choice == 2 then
        creature:SendUnitSay("I live to serve... and to reset your hearthstone cooldown.", 0)
    elseif choice == 3 then
        creature:SendUnitSay("Do you require assistance? or just company?", 0)
        creature:SendUnitEmote("Lilly smiles warmly.")
    elseif choice == 4 then
        creature:SendUnitSay("The great Ramires told me you might need help. He didn't mention you'd need *this* much help.", 0)
        creature:SendUnitEmote("Lilly winks.")
    elseif choice == 5 then
        creature:SendUnitSay("Teleportation, banking, advice... I do it all. Except laundry.", 0)
    elseif choice == 6 then
        creature:SendUnitSay("I was just organizing the master's library. What do you need?", 0)
    elseif choice == 7 then
        creature:SendUnitSay("You rang? Oh wait, that's a different franchise.", 0)
    elseif choice == 8 then
        creature:SendUnitSay("I am ready to assist. Please don't ask me to tank.", 0)
    elseif choice == 9 then
        creature:SendUnitSay("Your wish is my command. Within the boundaries of the script execution time limit.", 0)
    elseif choice == 10 then
         creature:SendUnitSay("A helper's work is never done.", 0)
         creature:SendUnitEmote("Lilly dusts off her shoulder.")
    elseif choice == 11 then
        creature:SendUnitSay("How can I make your Azerothian life easier today?", 0)
    else
        creature:SendUnitSay("At your service! No, I don't know where Mankrik's wife is.", 0)
    end
end

local function OnSqueakSummon(event, creature, summoner)
    math.randomseed(os.time())
    local choice = math.random(1, 12)
    if choice == 1 then
        creature:SendUnitSay("Squeak! I mean... Yes? What is it?", 0)
    elseif choice == 2 then
        creature:SendUnitSay("I am a very busy Lord! Make it quick!", 0)
    elseif choice == 3 then
        creature:SendUnitSay("You want my influence? It will cost you... cheese! I mean, emblems!", 0)
    elseif choice == 4 then
        creature:SendUnitSay("Do not look at my tail! Look at my monocle! I am civilized!", 0)
    elseif choice == 5 then
        creature:SendUnitSay("Yes-yes? You want trade? Shiny emblems for shiny armor?", 0)
        creature:SendUnitEmote("Lord Squeak rubs his paws together.")
    elseif choice == 6 then
        creature:SendUnitSay("I smell... success! And maybe old cheddar.", 0)
    elseif choice == 7 then
        creature:SendUnitSay("Quickly now! The cat-beast might be watching!", 0)
        creature:SendUnitEmote("Lord Squeak looks around nervously.")
    elseif choice == 8 then
        creature:SendUnitSay("You bring emblems? Good-good. I give gear. Fair trade!", 0)
    elseif choice == 9 then
        creature:SendUnitSay("My prices are the best in the under-city! I mean... the city!", 0)
    elseif choice == 10 then
        creature:SendUnitSay("No traps? Good. We talk business.", 0)
    elseif choice == 11 then
        creature:SendUnitSay("I am nobility! Treat me with respect or I knaw on your boots!", 0)
    else
        creature:SendUnitSay("Golden coins? Bah! I want the heavy tokens!", 0)
    end
end

local function OnCromiSummon(event, creature, summoner)
    math.randomseed(os.time())
    local choice = math.random(1, 12)
    if choice == 1 then
        creature:SendUnitSay("You are just in time! Or are you?", 0)
    elseif choice == 2 then
        creature:SendUnitSay("Let's unwind time! Did we do this already?", 0)
    elseif choice == 3 then
        creature:SendUnitSay("Time is money, friend! Wait, wrong goblin line. Time is... wibbly wobbly!", 0)
        creature:SendUnitEmote("Cromi adjusts her goggles.")
    elseif choice == 4 then
        creature:SendUnitSay("Did you need a reset? Or did you just want to say hello to a friendly gnome dragon?", 0)
    elseif choice == 5 then
        creature:SendUnitSay("Wait, haven't we had this conversation tomorrow?", 0)
    elseif choice == 6 then
        creature:SendUnitSay("I remember you from the future! You were taller.", 0)
    elseif choice == 7 then
        creature:SendUnitSay("Resetting instances... reversing entropy... hold on.", 0)
    elseif choice == 8 then
        creature:SendUnitSay("Don't step on any butterflies! The timeline is fragile!", 0)
        creature:SendUnitEmote("Cromi looks panicked.")
    elseif choice == 9 then
        creature:SendUnitSay("The Bronze Dragonflight sends their regards. Or they will. Or they have.", 0)
    elseif choice == 10 then
        creature:SendUnitSay("Let's just say I made that dungeon lockout... disappear.", 0)
        creature:SendUnitEmote("Cromi makes a magical gesture.")
    elseif choice == 11 then
        creature:SendUnitSay("Chronologically speaking, you're late.", 0)
    else
        creature:SendUnitSay("I love the smell of temporal paradoxes in the morning.", 0)
    end
end

local function OnCielSummon(event, creature, summoner)
    math.randomseed(os.time())
    local choice = math.random(1, 12)
    if choice == 1 then
        creature:SendUnitSay("Horses? Cats? Dragons? I have them all! Well, mostly horses.", 0)
    elseif choice == 2 then
        creature:SendUnitSay("Do you think this mount makes me look fast?", 0)
        creature:SendUnitEmote("Ciel strikes a pose.")
    elseif choice == 3 then
        creature:SendUnitSay("Walking is so last season. You need hooves! Or wings!", 0)
    elseif choice == 4 then
        creature:SendUnitSay("I found a stray gryphon once. It bit me. Stick to the trained ones.", 0)
    elseif choice == 5 then
        creature:SendUnitSay("Life in the fast lane! That's where you belong.", 0)
    elseif choice == 6 then
        creature:SendUnitSay("Why walk when you can ride in style?", 0)
    elseif choice == 7 then
        creature:SendUnitSay("I have mounts that eat gold and poop rainbows. Okay, maybe just eat gold.", 0)
    elseif choice == 8 then
        creature:SendUnitSay("Four legs are better than two. It's basic math.", 0)
    elseif choice == 9 then
        creature:SendUnitSay("Need a lift? Or just something to show off in Dalaran?", 0)
    elseif choice == 10 then
        creature:SendUnitSay("Check out the horsepower on this beast!", 0)
        creature:SendUnitEmote("Ciel pats a nearby imaginary mount.")
    elseif choice == 11 then
        creature:SendUnitSay("My mounts are 100% organic. Except the mechanical ones. Those spill oil.", 0)
    else
        creature:SendUnitSay("Catch a ride before I leave!", 0)
    end
end

local function OnRenameSummon(event, creature, summoner)
    math.randomseed(os.time())
    local choice = math.random(1, 12)
    if choice == 1 then
        creature:SendUnitSay("I am NOT a transmogger! I change names, not clothes!", 0)
        creature:SendUnitEmote("Rename looks annoyed.")
    elseif choice == 2 then
        creature:SendUnitSay("Names are power. Choose wisely. Or just pick something silly, everyone else does.", 0)
    elseif choice == 3 then
        creature:SendUnitSay("You want to rename your demon? Does it really care? FINE.", 0)
    elseif choice == 4 then
        creature:SendUnitSay("Don't ask me about my outfit. Just tell me the name.", 0)
    elseif choice == 5 then
        creature:SendUnitSay("Please don't pick 'Legolas' again. I beg you.", 0)
    elseif choice == 6 then
        creature:SendUnitSay("A rose by any other name... would cost you gold to rename.", 0)
    elseif choice == 7 then
        creature:SendUnitSay("Identity crisis? I can help with that.", 0)
    elseif choice == 8 then
        creature:SendUnitSay("You want to call your pet WHAT? ...Okay, no judgment. Maybe a little.", 0)
        creature:SendUnitEmote("Rename raises an eyebrow.")
    elseif choice == 9 then
        creature:SendUnitSay("Paperwork, paperwork. Fill out form 3-B for a name change.", 0)
    elseif choice == 10 then
        creature:SendUnitSay("New name, new life. Same smell though.", 0)
    elseif choice == 11 then
        creature:SendUnitSay("I specialize in onomastic reconstruction. That means I rename stuff.", 0)
    else
        creature:SendUnitSay("Make it catchy! Something that screams 'Victory'!", 0)
    end
end

local function OnWarpweaverSummon(event, creature, summoner)
    math.randomseed(os.time())
    local choice = math.random(1, 12)
    if choice == 1 then
        creature:SendUnitSay("The void calls... do you have the funds to answer?", 0)
    elseif choice == 2 then
        creature:SendUnitSay("Reality is fragile. Fashion... is eternal.", 0)
    elseif choice == 3 then
        creature:SendUnitSay("I can shape the threads of your existence. For a price.", 0)
    elseif choice == 4 then
        creature:SendUnitSay("Everything you see is an illusion. Except my fees. Those are real.", 0)
    elseif choice == 5 then
        creature:SendUnitSay("That helm... with those shoulders? A tragedy.", 0)
        creature:SendUnitEmote("Warpweaver shudders.")
    elseif choice == 6 then
        creature:SendUnitSay("Let us tailor the fabric of the universe to suit your style.", 0)
    elseif choice == 7 then
        creature:SendUnitSay("You wish to change your appearance? A wise choice.", 0)
    elseif choice == 8 then
        creature:SendUnitSay("The Ethereals know style. You... need assistance.", 0)
    elseif choice == 9 then
        creature:SendUnitSay("Purple is the new black. Or is it void-black?", 0)
    elseif choice == 10 then
        creature:SendUnitSay("Your soul looks... heavy. Maybe lighter fabrics?", 0)
    elseif choice == 11 then
        creature:SendUnitSay("I weave the nether. And cotton. Mostly nether.", 0)
    else
        creature:SendUnitSay("Do not look into the void... unless it matches your boots.", 0)
    end
end

-- Stoneform is now handled by C++ spell script (spell_custom_stoneform.cpp)
-- The Lua handler has been removed as ALE doesn't expose GetDispellableAuraList

-- Function to handle the spell cast event
local function OnMendPetCast(event, caster, spell)
  -- Check if the caster is a player and has a pet
  if caster:IsPlayer() and spell:GetEntry() == MEND_PET_SPELL_ID then
    -- Get the player's pet
    local pet = caster:GetMap():GetWorldObject(caster:GetPetGUID())
	
	if pet then
    -- Set the pet's scale to 1
		local currentScale = pet:GetScale()
		local newScale = currentScale * 2
		if newScale > 5 then
			newScale = 5
		end
		pet:SetScale(newScale)
	end
  end
end

local function GrantUtilityOnLogin(event, player)

	local playerClass = player:GetClassAsString()
	local playerRace = player:GetRaceAsString()
	
	-- Profession Check for specializations
	
    -- === Engineering (Single Tier) ===
    if player:HasSpell(20222) or player:HasSpell(20219) then
        if not player:HasSpell(20222) then player:LearnSpell(20222) end -- Learn Goblin
        if not player:HasSpell(20219) then player:LearnSpell(20219) end -- Learn Gnomish
    end

    -- === Alchemy (Single Tier) ===
    if player:HasSpell(28672) or player:HasSpell(28675) or player:HasSpell(28677) then
        if not player:HasSpell(28672) then player:LearnSpell(28672) end -- Learn Potion Master
        if not player:HasSpell(28675) then player:LearnSpell(28675) end -- Learn Elixir Master
        if not player:HasSpell(28677) then player:LearnSpell(28677) end -- Learn Transmutation Master
    end

    -- === Leatherworking (Single Tier) ===
    if player:HasSpell(10656) or player:HasSpell(10658) or player:HasSpell(10660) then
        if not player:HasSpell(10656) then player:LearnSpell(10656) end -- Learn Dragonscale
        if not player:HasSpell(10658) then player:LearnSpell(10658) end -- Learn Elemental
        if not player:HasSpell(10660) then player:LearnSpell(10660) end -- Learn Tribal
    end

    -- === Tailoring (Single Tier) ===
    if player:HasSpell(26797) or player:HasSpell(26801) or player:HasSpell(26798) then
        if not player:HasSpell(26797) then player:LearnSpell(26797) end -- Learn Spellfire
        if not player:HasSpell(26801) then player:LearnSpell(26801) end -- Learn Shadoweave
        if not player:HasSpell(26798) then player:LearnSpell(26798) end -- Learn Mooncloth
    end

    -- === Blacksmithing TIER 1 (Armor vs Weapon) ===
    -- This block is independent. It only checks for the base specializations.
    if player:HasSpell(9788) or player:HasSpell(9787) then
        if not player:HasSpell(9788) then player:LearnSpell(9788) end -- Learn Armorsmith
        if not player:HasSpell(9787) then player:LearnSpell(9787) end -- Learn Weaponsmith
    end

    -- === Blacksmithing TIER 2 (Weapon Masteries) ===
    -- This block is also independent. It only checks if you have a weapon mastery.
    if player:HasSpell(17041) or player:HasSpell(17040) or player:HasSpell(17039) then
        if not player:HasSpell(17041) then player:LearnSpell(17041) end -- Learn Master Swordsmith
        if not player:HasSpell(17040) then player:LearnSpell(17040) end -- Learn Master Hammersmith
        if not player:HasSpell(17039) then player:LearnSpell(17039) end -- Learn Master Axesmith
    end
	
    if not player:HasSpell(81002) then
        player:LearnSpell(81002)
    end
	
    if not player:HasSpell(81003) then
        player:LearnSpell(81003)
    end
	
    if not player:HasSpell(81005) and playerClass ~= "Rogue" then
        player:LearnSpell(81005)
    end
	
    if not player:HasSpell(81007) then
        player:LearnSpell(81007)
    end
	
	-- Diplomacy
	if not player:HasSpell(20599) then
		player:LearnSpell(20599)
	end
	--

	-- Pick Pocket (Rogue) for all
	if not player:HasSpell(921) then
		player:LearnSpell(921)
	end
	--
	
	-- Heroic Presence
    if not player:HasSpell(6562) and playerRace == "Draenei" then
        player:LearnSpell(6562)
    end
	
	if player:HasSpell(28878) and playerRace == "Draenei" then
		player:RemoveSpell(28878)
	end
	--
	
    if not player:HasSpell(59541) and playerRace == "Draenei" then
        player:LearnSpell(59541)
    end
	
	-- Arcane Torrent (warrior)
	if not player:HasSpell(81011) and playerRace == "Blood Elf" and playerClass == "Warrior" then
        player:LearnSpell(81011)
    end
	--
	
	-- Stoneform (Human)
	if not player:HasSpell(81013) and playerRace == "Human" then
        player:LearnSpell(81013)
    end
	--
	
	-- Gift of the Naaru
    if not player:HasSpell(59542) and playerRace == "Draenei" then
        player:LearnSpell(59542)
    end
	
	if player:HasSpell(28880) and playerRace == "Draenei" then
		player:RemoveSpell(28880)
	end
	
	if player:HasSpell(59543) and playerRace == "Draenei" then
		player:RemoveSpell(59543)
	end
	
	if player:HasSpell(59544) and playerRace == "Draenei" then
		player:RemoveSpell(59544)
	end
	
	if player:HasSpell(59545) and playerRace == "Draenei" then
		player:RemoveSpell(59545)
	end
	
	if player:HasSpell(59548) and playerRace == "Draenei" then
		player:RemoveSpell(59548)
	end
	
	if player:HasSpell(59547) and playerRace == "Draenei" then
		player:RemoveSpell(59547)
	end
	--
	
    if not player:HasSpell(28875) and playerRace == "Draenei" then
        player:LearnSpell(28875)
    end
			
    if not player:HasSpell(81008) then
        player:LearnSpell(81008)
    end
	
	if player:HasSpell(81010) then
		player:RemoveSpell(81010)
	end
	
    if player:HasSpell(2645) and playerClass ~= "Shaman" then
		player:RemoveSpell(2645)
    end
	
	if player:HasSpell(5118) and playerClass ~= "Hunter" then
		player:RemoveSpell(5118)
    end
	
	if player:HasSpell(1066) and playerClass ~= "Druid" then
		player:RemoveSpell(1066)
    end
	
	if player:HasSpell(1804) and playerClass ~= "Rogue" then
		player:RemoveSpell(1804)
    end
	
	if player:HasSpell(81005) and playerClass == "Rogue" then
		player:RemoveSpell(81005)
    end	
	
	if player:HasSpell(81001) then
		player:RemoveSpell(81001)
    end
	
end

local function OnLogout(event, player)

end

RegisterPlayerEvent(4, OnLogout) -- PLAYER_EVENT_ON_LOGOUT
RegisterCreatureEvent(290011, 22, OnLingSummon)
RegisterCreatureEvent(299902, 22, OnVendorSummon)
RegisterCreatureEvent(299900, 22, OnLillySummon)
RegisterCreatureEvent(299901, 22, OnSqueakSummon)
RegisterCreatureEvent(300000, 22, OnCromiSummon)
RegisterCreatureEvent(299903, 22, OnCielSummon)
RegisterCreatureEvent(200002, 22, OnRenameSummon)
RegisterCreatureEvent(190011, 22, OnWarpweaverSummon)
RegisterPlayerEvent(5, OnMendPetCast)
RegisterPlayerEvent(3, GrantUtilityOnLogin)