#!/usr/bin/env python3
"""
Lottery Enchant Migration — Legacy DBC → Synthetic Flat 1-77 IDs
================================================================
Re-randomizes ALL lottery enchant rows using the new synthetic ID format.
Same probabilities as the C++ code:
  - Cascading slot chances: 70/60/50/50/50/40/40%
  - 1% per slot → Flying enchant (75/20/5% for stage 1/2/3)
  - 99% per slot → flat pool of 20 types (13 stats + 6 resists + 1 speed)
  - Value: urand(1, 77) for every stat
  - No duplicate enchant IDs on the same item

Usage:
  python3 tools/migrate_lottery_enchants.py

Reads MySQL credentials from conf/worldserver.conf (CharacterDatabaseInfo).
Generates and executes SQL directly — backs up old data first.
"""

import configparser
import os
import random
import re
import sys

# ---------------------------------------------------------------------------
# Constants — mirror of random_enchants.cpp
# ---------------------------------------------------------------------------
LOTTERY_MAX_VALUE = 77
MAX_LOTTERY_SLOTS = 7
ROLL_CHANCES = [70.0, 60.0, 50.0, 50.0, 50.0, 40.0, 40.0]

# 13 allowed stat types (ITEM_MOD_* enum values from ItemTemplate.h)
LOTTERY_STAT_POOL = [
    3,   # ITEM_MOD_AGILITY
    4,   # ITEM_MOD_STRENGTH
    5,   # ITEM_MOD_INTELLECT
    6,   # ITEM_MOD_SPIRIT
    7,   # ITEM_MOD_STAMINA
    45,  # ITEM_MOD_SPELL_POWER
    31,  # ITEM_MOD_HIT_RATING
    32,  # ITEM_MOD_CRIT_RATING
    36,  # ITEM_MOD_HASTE_RATING
    37,  # ITEM_MOD_EXPERTISE_RATING
    44,  # ITEM_MOD_ARMOR_PENETRATION_RATING
    47,  # ITEM_MOD_SPELL_PENETRATION
    35,  # ITEM_MOD_RESILIENCE_RATING
]
LOTTERY_STAT_POOL_SIZE = len(LOTTERY_STAT_POOL)
LOTTERY_RESIST_SCHOOLS = 6  # Holy(1)..Arcane(6)
LOTTERY_TOTAL_POOL_SIZE = LOTTERY_STAT_POOL_SIZE + LOTTERY_RESIST_SCHOOLS + 1  # = 20

# Synthetic enchant ID encoding bases
CUSTOM_STAT_ENCHANT_BASE   = 800000
CUSTOM_RESIST_ENCHANT_BASE = 850000
CUSTOM_ENCHANT_SPEED_BASE  = 900001  # + (1..77) = pct
CUSTOM_ENCHANT_FLY_BASE    = 900100  # + (1..3) = stage


def is_synthetic(enchant_id: int) -> bool:
    """Check if an enchant ID is already in the new synthetic format."""
    return enchant_id >= CUSTOM_STAT_ENCHANT_BASE


def roll_enchants_for_item() -> list[int]:
    """Roll a complete set of new enchants for one item — same logic as C++."""
    result = []
    for slot_idx in range(MAX_LOTTERY_SLOTS):
        chance = ROLL_CHANCES[slot_idx]
        if random.uniform(0, 100) >= chance:
            break  # Cascading — once a roll fails, no more slots

        enchant_id = _roll_one_enchant(result)
        if enchant_id == 0:
            break
        result.append(enchant_id)

    return result


def _roll_one_enchant(exclude_set: list[int]) -> int:
    """Roll a single enchant, avoiding duplicates. Same logic as GetRandomEnchant."""
    # 1% chance: Flying enchant
    if random.randint(1, 100) == 1:
        fly_roll = random.randint(1, 100)
        if fly_roll <= 75:
            fly_id = CUSTOM_ENCHANT_FLY_BASE + 1  # Stage 1
        elif fly_roll <= 95:
            fly_id = CUSTOM_ENCHANT_FLY_BASE + 2  # Stage 2
        else:
            fly_id = CUSTOM_ENCHANT_FLY_BASE + 3  # Stage 3

        if fly_id not in exclude_set:
            return fly_id
        # Fall through to normal pool if same stage already present

    # 99% chance: random type from pool of 20, value 1-77
    for _ in range(10):
        type_index = random.randint(0, LOTTERY_TOTAL_POOL_SIZE - 1)
        value = random.randint(1, LOTTERY_MAX_VALUE)

        if type_index < LOTTERY_STAT_POOL_SIZE:
            stat_type = LOTTERY_STAT_POOL[type_index]
            enchant_id = CUSTOM_STAT_ENCHANT_BASE + stat_type * 100 + value
        elif type_index < LOTTERY_STAT_POOL_SIZE + LOTTERY_RESIST_SCHOOLS:
            school = (type_index - LOTTERY_STAT_POOL_SIZE) + 1
            enchant_id = CUSTOM_RESIST_ENCHANT_BASE + school * 100 + value
        else:
            enchant_id = CUSTOM_ENCHANT_SPEED_BASE + value

        if enchant_id not in exclude_set:
            return enchant_id

    return 0  # All 10 attempts collided (astronomically unlikely)


def parse_db_config(conf_path: str) -> dict:
    """Extract CharacterDatabaseInfo from worldserver.conf."""
    pattern = re.compile(
        r'^\s*CharacterDatabaseInfo\s*=\s*"([^"]+)"\s*$', re.MULTILINE
    )
    with open(conf_path, "r") as f:
        content = f.read()

    match = pattern.search(content)
    if not match:
        print("ERROR: CharacterDatabaseInfo not found in", conf_path)
        sys.exit(1)

    # Format: "host;port;user;pass;dbname"
    parts = match.group(1).split(";")
    if len(parts) != 5:
        print(f"ERROR: Expected 5 parts in CharacterDatabaseInfo, got {len(parts)}")
        sys.exit(1)

    return {
        "host": parts[0],
        "port": int(parts[1]),
        "user": parts[2],
        "password": parts[3],
        "database": parts[4],
    }


def main():
    # Find worldserver.conf
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    conf_path = os.path.join(project_root, "conf", "worldserver.conf")
    if not os.path.exists(conf_path):
        # Try env/dist/etc path
        conf_path = os.path.join(project_root, "env", "dist", "etc", "worldserver.conf")
    if not os.path.exists(conf_path):
        print("ERROR: worldserver.conf not found. Tried conf/ and env/dist/etc/")
        sys.exit(1)

    print(f"Reading config from: {conf_path}")
    db_cfg = parse_db_config(conf_path)
    print(f"Database: {db_cfg['database']} @ {db_cfg['host']}:{db_cfg['port']}")

    try:
        import mysql.connector
    except ImportError:
        print("ERROR: mysql-connector-python not installed.")
        print("  pip install mysql-connector-python")
        sys.exit(1)

    conn = mysql.connector.connect(**db_cfg)
    cursor = conn.cursor()

    # Step 1: Read all existing lottery enchant rows
    cursor.execute(
        "SELECT item_guid, slot_index, enchant_id "
        "FROM character_item_lottery_enchants "
        "ORDER BY item_guid, slot_index"
    )
    rows = cursor.fetchall()
    print(f"Found {len(rows)} enchant rows across items")

    if not rows:
        print("Nothing to migrate. Exiting.")
        cursor.close()
        conn.close()
        return

    # Group by item_guid
    items: dict[int, list[tuple[int, int]]] = {}
    for item_guid, slot_index, enchant_id in rows:
        if item_guid not in items:
            items[item_guid] = []
        items[item_guid].append((slot_index, enchant_id))

    total_items = len(items)
    already_synthetic = 0
    legacy_items = 0

    for item_guid, enchants in items.items():
        if all(is_synthetic(eid) for _, eid in enchants):
            already_synthetic += 1
        else:
            legacy_items += 1

    print(f"Items total: {total_items}")
    print(f"  Already synthetic (new format): {already_synthetic}")
    print(f"  Legacy (needs migration):       {legacy_items}")

    if legacy_items == 0:
        print("All items already in synthetic format. Nothing to do.")
        cursor.close()
        conn.close()
        return

    # Step 2: Backup old data
    print("\nCreating backup table character_item_lottery_enchants_backup...")
    cursor.execute("DROP TABLE IF EXISTS character_item_lottery_enchants_backup")
    cursor.execute(
        "CREATE TABLE character_item_lottery_enchants_backup "
        "AS SELECT * FROM character_item_lottery_enchants"
    )
    conn.commit()
    print("Backup created.")

    # Step 3: Re-roll legacy items
    print(f"\nRe-rolling {legacy_items} items with legacy enchant IDs...")
    migrated = 0
    total_old_enchants = 0
    total_new_enchants = 0

    for item_guid, enchants in items.items():
        # Skip items already in synthetic format
        if all(is_synthetic(eid) for _, eid in enchants):
            continue

        total_old_enchants += len(enchants)

        # Delete old rows for this item
        cursor.execute(
            "DELETE FROM character_item_lottery_enchants WHERE item_guid = %s",
            (item_guid,)
        )

        # Roll new enchants
        new_enchants = roll_enchants_for_item()
        total_new_enchants += len(new_enchants)

        # Insert new rows
        for slot_idx, enchant_id in enumerate(new_enchants):
            cursor.execute(
                "INSERT INTO character_item_lottery_enchants "
                "(item_guid, slot_index, enchant_id) VALUES (%s, %s, %s)",
                (item_guid, slot_idx, enchant_id)
            )

        migrated += 1

    conn.commit()

    print(f"\nMigration complete!")
    print(f"  Items migrated: {migrated}")
    print(f"  Old enchant rows removed: {total_old_enchants}")
    print(f"  New enchant rows created: {total_new_enchants}")
    print(f"  Backup table: character_item_lottery_enchants_backup")
    print(f"\nIMPORTANT: Restart the worldserver for stat recalculation on login.")
    print(f"  Old data is in character_item_lottery_enchants_backup if you need to restore.")

    cursor.close()
    conn.close()


if __name__ == "__main__":
    main()
