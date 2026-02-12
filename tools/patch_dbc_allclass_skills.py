#!/usr/bin/env python3
"""
Patch DBC files to allow ALL classes to use ALL weapon/armor/shield skills.

Modifies:
  - SkillRaceClassInfo.dbc  (ClassMask -> 0x7FF for weapon/armor/shield skills)
  - SkillLineAbility.dbc    (ClassMask -> 0x7FF, ExcludeClass -> 0 for same)

Usage:
  python3 patch_dbc_allclass_skills.py [dbc_dir]

Default dbc_dir: env/dist/data/dbc/

After patching, copy these two DBC files to your client's patch-*.MPQ:
  - SkillRaceClassInfo.dbc
  - SkillLineAbility.dbc
"""

import struct, shutil, sys, os

# Default path relative to project root
DEFAULT_DBC_DIR = os.path.join(os.path.dirname(__file__), '..', 'env', 'dist', 'data', 'dbc')

# All weapon + armor + shield skill IDs to open to all classes
TARGET_SKILLS = {
    43,   # Swords
    44,   # Axes
    45,   # Bows
    46,   # Guns
    54,   # Maces
    55,   # 2H Maces
    136,  # Staves
    160,  # 2H Axes
    162,  # Unarmed
    172,  # 2H Swords
    173,  # Daggers
    176,  # Thrown
    226,  # Crossbows
    228,  # Wands
    229,  # Polearms
    413,  # Mail
    414,  # Plate Mail
    415,  # Shield
    473,  # Fist Weapons
}

ALL_CLASSES = 0x7FF  # bits 0-10 = all 10 classes + DK
ALL_RACES = 0x7FFFFFFF


def patch_skill_race_class_info(dbc_dir):
    path = os.path.join(dbc_dir, 'SkillRaceClassInfo.dbc')
    shutil.copy2(path, path + '.bak')

    with open(path, 'rb') as f:
        header = f.read(20)
        nrecs, nfields, recsz, strsz = struct.unpack('<IIII', header[4:20])
        records = bytearray(f.read(nrecs * recsz))
        string_block = f.read(strsz)

    # Fields: ID(0), SkillID(1), RaceMask(2), ClassMask(3), Flags(4), MinLevel(5), SkillTierID(6), SkillCostIndex(7)
    modified = 0
    for i in range(nrecs):
        offset = i * recsz
        rec = list(struct.unpack_from('<iiiiiiii', records, offset))
        if rec[1] in TARGET_SKILLS and rec[3] != 0 and rec[3] != ALL_CLASSES:
            rec[2] = ALL_RACES
            rec[3] = ALL_CLASSES
            struct.pack_into('<iiiiiiii', records, offset, *rec)
            modified += 1

    with open(path, 'wb') as f:
        f.write(header)
        f.write(records)
        f.write(string_block)

    print(f'SkillRaceClassInfo.dbc: {modified} records modified')
    return modified


def patch_skill_line_ability(dbc_dir):
    path = os.path.join(dbc_dir, 'SkillLineAbility.dbc')
    shutil.copy2(path, path + '.bak')

    with open(path, 'rb') as f:
        header = f.read(20)
        nrecs, nfields, recsz, strsz = struct.unpack('<IIII', header[4:20])
        records = bytearray(f.read(nrecs * recsz))
        string_block = f.read(strsz)

    # Fields: ID(0), SkillLine(1), Spell(2), RaceMask(3), ClassMask(4), ExcludeRace(5), ExcludeClass(6), ...
    modified = 0
    for i in range(nrecs):
        offset = i * recsz
        fields = list(struct.unpack_from('<' + 'i' * nfields, records, offset))
        if fields[1] in TARGET_SKILLS:
            changed = False
            if fields[4] != 0:
                fields[4] = ALL_CLASSES
                changed = True
            if fields[6] != 0:
                fields[6] = 0
                changed = True
            if changed:
                struct.pack_into('<' + 'i' * nfields, records, offset, *fields)
                modified += 1

    with open(path, 'wb') as f:
        f.write(header)
        f.write(records)
        f.write(string_block)

    print(f'SkillLineAbility.dbc: {modified} records modified')
    return modified


if __name__ == '__main__':
    dbc_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.abspath(DEFAULT_DBC_DIR)
    print(f'Patching DBC files in: {dbc_dir}')
    patch_skill_race_class_info(dbc_dir)
    patch_skill_line_ability(dbc_dir)
    print('\nDone! Copy these to your client patch MPQ:')
    print(f'  {os.path.join(dbc_dir, "SkillRaceClassInfo.dbc")}')
    print(f'  {os.path.join(dbc_dir, "SkillLineAbility.dbc")}')
