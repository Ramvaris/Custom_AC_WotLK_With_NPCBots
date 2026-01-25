# Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3

file(GLOB_RECURSE sources_SoulKeeper src/*.cpp)

set(scripts_SoulKeeper
  ${sources_SoulKeeper}
)

message("  -> Prepared: Soul Keeper Module")
