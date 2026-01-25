-- ============================================================================
-- Resource Node Fast Respawn (2 minutes for all mining/herb nodes)
-- Custom mod for relaxation 2-player server. Apply after each DB reset.
-- ============================================================================

-- Update ALL mining nodes and herbs to 120 second respawn (2 minutes)
-- Standard Blizzard values range from 45 minutes to 7 DAYS which is insane for private use.

UPDATE gameobject g
JOIN gameobject_template gt ON g.id = gt.entry
SET g.spawntimesecs = 120
WHERE gt.type = 3 
  AND (
    -- Mining nodes (all standard minable veins and deposits)
    gt.name IN (
        'Copper Vein', 'Tin Vein', 'Silver Vein', 'Gold Vein', 'Iron Deposit',
        'Mithril Deposit', 'Truesilver Deposit', 'Small Thorium Vein', 'Rich Thorium Vein',
        'Dark Iron Deposit', 'Fel Iron Deposit', 'Adamantite Deposit', 'Rich Adamantite Deposit',
        'Khorium Vein', 'Cobalt Deposit', 'Rich Cobalt Deposit', 'Saronite Deposit',
        'Rich Saronite Deposit', 'Titanium Vein', 'Nethercite Deposit', 'Ancient Gem Vein',
        'Pure Saronite Deposit'
    )
    OR gt.name LIKE 'Ooze Covered%Vein%'
    OR gt.name LIKE 'Ooze Covered%Deposit%'
    OR gt.name LIKE 'Hakkari%Vein%'
    OR gt.name LIKE 'Incendicite%Vein%'
    OR gt.name LIKE 'Indurium%Vein%'
    OR gt.name LIKE 'Lesser Bloodstone%'

    -- Herbs (all standard gatherable herbs)
    OR gt.name IN (
        'Silverleaf', 'Peacebloom', 'Earthroot', 'Mageroyal', 'Briarthorn',
        'Stranglekelp', 'Bruiseweed', 'Wild Steelbloom', 'Grave Moss', 'Kingsblood',
        'Liferoot', 'Fadeleaf', 'Goldthorn', 'Wintersbite', 'Firebloom',
        'Purple Lotus', 'Sungrass', 'Blindweed', 'Ghost Mushroom', 'Gromsblood',
        'Golden Sansam', 'Dreamfoil', 'Mountain Silversage', 'Plaguebloom', 'Icecap',
        'Black Lotus', 'Felweed', 'Dreaming Glory', 'Ragveil', 'Flame Cap',
        'Terocone', 'Ancient Lichen', 'Netherbloom', 'Nightmare Vine', 'Mana Thistle',
        'Goldclover', 'Firethorn', 'Icethorn', 'Lichbloom', 'Frost Lotus',
        'Frozen Herb', 'Tiger Lily', 'Glowcap'
    )
    OR gt.name LIKE '%Khadgar%Whisker%'
    OR gt.name LIKE '%Arthas%Tears%'
    OR gt.name LIKE '%Talandra%Rose%'
    OR gt.name LIKE '%Adder%Tongue%'
  )
  AND g.spawntimesecs > 120;
