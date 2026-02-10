# Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3

file(GLOB_RECURSE sources_CustomRamvaris src/*.cpp)

set(scripts_CustomRamvaris
  ${sources_CustomRamvaris}
)

message("  -> Prepared: Custom Ramvaris Module (private server customizations)")
