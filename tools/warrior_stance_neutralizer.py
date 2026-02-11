#!/usr/bin/env python3
"""
Warrior Stance Neutralizer — Spell.dbc Patcher
Author: Ryo (AI Agent) for Ramires' Custom AzerothCore

Removes ALL warrior stance requirements from EVERY spell in Spell.dbc.
This includes active abilities (Heroic Strike, Mortal Strike, etc.),
defensive cooldowns (Shield Wall, Last Stand, etc.), and passive talents
(Improved Berserker Stance, Improved Defensive Stance, etc.).

After patching:
  - Active abilities can be used in ANY stance (or no stance)
  - Passive talent effects apply in ALL stances
    Example: "Improved Berserker Stance" gives +20% STR in all stances
    Example: "Improved Defensive Stance" reduces spell damage in all stances
  - Stance-switch spells themselves are unchanged (they don't have stance reqs)
  - No other class forms (Druid, Rogue Stealth, etc.) are affected

Technical Details:
  WotLK 3.3.5a Spell.dbc format:
    Header: 20 bytes (WDBC magic, nRecords, nFields, recordSize, stringSize)
    Records: nRecords × recordSize bytes (234 columns × 4 bytes = 936 bytes each)
    String block: stringSize bytes

  Warrior stance bits in ShapeshiftMask fields:
    Stances (column 12, offset +48): bitmask `1 << (form - 1)`
    StancesNot (column 14, offset +56): exclusion bitmask

    FORM_BATTLESTANCE    = 17 → bit 16 → 0x10000
    FORM_DEFENSIVESTANCE = 18 → bit 17 → 0x20000
    FORM_BERSERKERSTANCE = 19 → bit 18 → 0x40000
    Combined warrior mask:               0x70000

  Operation: For each spell record, clear bits 16-18 in both Stances and StancesNot.
  Only modifies spells that actually HAD warrior stance bits set — pure no-op on others.

Usage:
  python3 warrior_stance_neutralizer.py [path_to_Spell.dbc]
  Default path: env/dist/data/dbc/Spell.dbc (relative to project root)
"""

import struct
import sys
import os
import shutil
from pathlib import Path

# Warrior stance bitmask values (1 << (form - 1))
BATTLE_STANCE_BIT    = 1 << 16  # 0x10000, FORM_BATTLESTANCE = 17
DEFENSIVE_STANCE_BIT = 1 << 17  # 0x20000, FORM_DEFENSIVESTANCE = 18
BERSERKER_STANCE_BIT = 1 << 18  # 0x40000, FORM_BERSERKERSTANCE = 19
WARRIOR_STANCE_MASK  = BATTLE_STANCE_BIT | DEFENSIVE_STANCE_BIT | BERSERKER_STANCE_BIT  # 0x70000

# DBC column offsets (bytes from record start)
STANCES_OFFSET     = 12 * 4  # Column 12 → byte 48
STANCES_NOT_OFFSET = 14 * 4  # Column 14 → byte 56

# DBC header size
HEADER_SIZE = 20

def stance_name(bit):
    """Human-readable stance name from a single bit."""
    names = {
        BATTLE_STANCE_BIT: "Battle",
        DEFENSIVE_STANCE_BIT: "Defensive",
        BERSERKER_STANCE_BIT: "Berserker",
    }
    return names.get(bit, f"Unknown(0x{bit:X})")


def describe_mask(mask):
    """Describe a stance bitmask as human-readable stance list."""
    parts = []
    for bit in [BATTLE_STANCE_BIT, DEFENSIVE_STANCE_BIT, BERSERKER_STANCE_BIT]:
        if mask & bit:
            parts.append(stance_name(bit))
    return "+".join(parts) if parts else "(none)"


def main():
    # Resolve DBC path
    if len(sys.argv) > 1:
        dbc_path = Path(sys.argv[1])
    else:
        # Default: relative to project root
        script_dir = Path(__file__).resolve().parent
        project_root = script_dir.parent
        dbc_path = project_root / "env" / "dist" / "data" / "dbc" / "Spell.dbc"

    if not dbc_path.exists():
        print(f"ERROR: Spell.dbc not found at: {dbc_path}")
        sys.exit(1)

    print(f"Reading: {dbc_path}")
    print(f"Warrior stance mask: 0x{WARRIOR_STANCE_MASK:08X} ({describe_mask(WARRIOR_STANCE_MASK)})")
    print()

    # Read entire file into memory
    with open(dbc_path, "rb") as f:
        data = bytearray(f.read())

    # Parse header
    magic = data[:4]
    if magic != b"WDBC":
        print(f"ERROR: Invalid DBC magic: {magic} (expected WDBC)")
        sys.exit(1)

    n_records, n_fields, record_size, string_size = struct.unpack_from("<4I", data, 4)
    print(f"Records: {n_records}")
    print(f"Fields: {n_fields}")
    print(f"Record size: {record_size} bytes ({record_size // 4} columns)")
    print()

    if record_size != 936:
        print(f"WARNING: Unexpected record size {record_size} (expected 936 for WotLK 3.3.5a)")
        print("Continuing anyway — field offsets should still be correct.")
        print()

    # Process all records
    modified_stances = 0
    modified_stances_not = 0
    modified_spells = []

    for i in range(n_records):
        record_offset = HEADER_SIZE + i * record_size

        # Read spell ID (column 0)
        spell_id = struct.unpack_from("<I", data, record_offset)[0]

        # Read Stances (column 12) and StancesNot (column 14)
        stances = struct.unpack_from("<I", data, record_offset + STANCES_OFFSET)[0]
        stances_not = struct.unpack_from("<I", data, record_offset + STANCES_NOT_OFFSET)[0]

        changed = False

        # Clear warrior stance bits from Stances
        if stances & WARRIOR_STANCE_MASK:
            old_stances = stances
            stances &= ~WARRIOR_STANCE_MASK
            struct.pack_into("<I", data, record_offset + STANCES_OFFSET, stances)
            modified_stances += 1
            changed = True

        # Clear warrior stance bits from StancesNot
        if stances_not & WARRIOR_STANCE_MASK:
            old_stances_not = stances_not
            stances_not &= ~WARRIOR_STANCE_MASK
            struct.pack_into("<I", data, record_offset + STANCES_NOT_OFFSET, stances_not)
            modified_stances_not += 1
            changed = True

        if changed:
            modified_spells.append(spell_id)

    print(f"Modified spells: {len(modified_spells)}")
    print(f"  Stances cleared:    {modified_stances}")
    print(f"  StancesNot cleared: {modified_stances_not}")
    print()

    if not modified_spells:
        print("No warrior stance requirements found — DBC may already be neutralized.")
        sys.exit(0)

    # Create backup
    backup_path = dbc_path.with_suffix(".dbc.bak")
    if not backup_path.exists():
        shutil.copy2(dbc_path, backup_path)
        print(f"Backup: {backup_path}")
    else:
        print(f"Backup already exists: {backup_path}")

    # Write modified DBC
    with open(dbc_path, "wb") as f:
        f.write(data)

    print(f"Written: {dbc_path}")
    print()
    print(f"Stance neutralization complete! {len(modified_spells)} spells patched. 🍥")
    print("Copy this Spell.dbc to your client MPQ to match server-side changes.")


if __name__ == "__main__":
    main()
