#!/usr/bin/env python3
"""
Patch DBC files to allow ALL classes to use ALL weapon/armor/shield skills
and core combat passives (Parry/Block/Dual Wield), and give all classes the
ranged weapon + ammo slot.

Modifies:
  - SkillRaceClassInfo.dbc  (ClassMask -> 0x7FF for all weapon/armor/combat skills)
    - SkillLineAbility.dbc    (ClassMask -> 0x7FF, ExcludeClass -> 0 for same)
  - ChrClasses.dbc          (clear UsesRelicSlot flag 0x08 so all classes get ammo slot)

Usage:
  python3 patch_dbc_allclass_skills.py [dbc_dir]

Default dbc_dir: env/dist/data/dbc/

After patching, copy these THREE DBC files to your client's patch-*.MPQ:
  - SkillRaceClassInfo.dbc
  - SkillLineAbility.dbc
  - ChrClasses.dbc
"""

import struct, shutil, sys, os

DEFAULT_DBC_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'env', 'dist', 'data', 'dbc')

# All weapon + armor + combat skill IDs to open to all classes
TARGET_SKILLS = {
    43,   # Swords
    44,   # Axes
    45,   # Bows
    46,   # Guns
    54,   # Maces
    55,   # 2H Swords
    95,   # Defense
    118,  # Dual Wield
    136,  # Staves
    160,  # 2H Maces
    162,  # Unarmed
    172,  # 2H Axes
    173,  # Daggers
    176,  # Thrown
    226,  # Crossbows
    228,  # Wands
    229,  # Polearms
    293,  # Plate Mail
    413,  # Mail
    414,  # Leather
    415,  # Cloth
    433,  # Shield
    473,  # Fist Weapons
}

# Specific combat passive spells to force-open by class mask in SkillLineAbility
# (in addition to skill-line based matching above).
TARGET_SPELLS = {
    81,    # Dodge
    107,   # Block
    674,   # Dual Wield
    3127,  # Parry
}

ALL_CLASSES = 0x7FF
ALL_RACES = 0x7FFFFFFF


def patch_skill_race_class_info(dbc_dir):
    path = os.path.join(dbc_dir, 'SkillRaceClassInfo.dbc')
    shutil.copy2(path, path + '.bak')
    with open(path, 'rb') as f:
        header = f.read(20)
        nrecs, nfields, recsz, strsz = struct.unpack('<IIII', header[4:20])
        records = bytearray(f.read(nrecs * recsz))
        string_block = f.read(strsz)
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
    modified = 0
    for i in range(nrecs):
        offset = i * recsz
        fields = list(struct.unpack_from('<' + 'i' * nfields, records, offset))
        skill_line_id = fields[1]
        spell_id = fields[2]
        if skill_line_id in TARGET_SKILLS or spell_id in TARGET_SPELLS:
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


def patch_chr_classes(dbc_dir):
    """Clear the UsesRelicSlot flag (0x08) so all classes get ranged weapon + ammo slot."""
    path = os.path.join(dbc_dir, 'ChrClasses.dbc')
    shutil.copy2(path, path + '.bak')
    with open(path, 'rb') as f:
        header = f.read(20)
        nrecs, nfields, recsz, strsz = struct.unpack('<IIII', header[4:20])
        records = bytearray(f.read(nrecs * recsz))
        string_block = f.read(strsz)
    CLASS_NAMES = {1: 'Warrior', 2: 'Paladin', 3: 'Hunter', 4: 'Rogue', 5: 'Priest',
                   6: 'Death Knight', 7: 'Shaman', 8: 'Mage', 9: 'Warlock', 11: 'Druid'}
    modified = 0
    for i in range(nrecs):
        offset = i * recsz
        fields = list(struct.unpack_from('<' + 'i' * nfields, records, offset))
        cid = fields[0]
        if cid in CLASS_NAMES and (fields[57] & 0x08):
            fields[57] &= ~0x08
            struct.pack_into('<' + 'i' * nfields, records, offset, *fields)
            modified += 1
            print(f'  {CLASS_NAMES[cid]}: cleared UsesRelicSlot flag')
    with open(path, 'wb') as f:
        f.write(header)
        f.write(records)
        f.write(string_block)
    print(f'ChrClasses.dbc: {modified} records modified')
    return modified


if __name__ == '__main__':
    dbc_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.abspath(DEFAULT_DBC_DIR)
    print(f'Patching DBC files in: {dbc_dir}\n')
    patch_skill_race_class_info(dbc_dir)
    patch_skill_line_ability(dbc_dir)
    patch_chr_classes(dbc_dir)
    print('\nDone! Copy these to your client patch MPQ:')
    print(f'  {os.path.join(dbc_dir, "SkillRaceClassInfo.dbc")}')
    print(f'  {os.path.join(dbc_dir, "SkillLineAbility.dbc")}')
    print(f'  {os.path.join(dbc_dir, "ChrClasses.dbc")}')
