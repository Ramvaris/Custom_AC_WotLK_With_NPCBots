import os
import re

# Paths
WORKSPACE = "/home/ramires/projects/Custom_AC_WotLK_With_NPCBots"
DIST_DIR = os.path.join(WORKSPACE, "env/dist/etc")
OLD_WORLD_PATH = os.path.join(WORKSPACE, "var/old_world_active.txt")
OLD_AUTH_PATH = os.path.join(WORKSPACE, "var/old_auth_active.txt")
NEW_WORLD_DIST = os.path.join(DIST_DIR, "worldserver.conf.dist")
NEW_AUTH_DIST = os.path.join(DIST_DIR, "authserver.conf.dist")
NEW_WORLD_CONF = os.path.join(DIST_DIR, "worldserver.conf")
NEW_AUTH_CONF = os.path.join(DIST_DIR, "authserver.conf")

# Path Overrides
PATH_OVERRIDES = {
    "DataDir": os.path.join(WORKSPACE, "env/dist/data"),
    "LogsDir": os.path.join(WORKSPACE, "env/dist/logs"),
    "TempDir": os.path.join(WORKSPACE, "env/dist/temp"),
    "MySQLExecutable": "/usr/bin/mysql",
    "SourceDirectory": WORKSPACE,
    "BuildDirectory": os.path.join(WORKSPACE, "var/build/obj"),
    "PidFile": os.path.join(WORKSPACE, "env/dist/bin/worldserver.pid")
}

# DB Credentials to ignore (Use default for new environment)
IGNORE_KEYS = [
    "LoginDatabaseInfo", "WorldDatabaseInfo", "CharacterDatabaseInfo",
    "LoginDatabase.WorkerThreads", "WorldDatabase.WorkerThreads", # Keep new defaults for threads
]

def load_config(path):
    config = {}
    if not os.path.exists(path):
        return config
    
    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#') or line.startswith('['):
                continue
            
            if '=' in line:
                parts = line.split('=', 1)
                key = parts[0].strip()
                val = parts[1].strip()
                # Remove inline comments manually if any, though risky with strings
                if "  -" in val: # heuristics for comments like "1 - (Enabled)"
                     val = val.split("  -")[0].strip()
                config[key] = val
    return config

def migrate(dist_path, old_config_path, output_path, config_type):
    old_config = load_config(old_config_path)
    
    with open(dist_path, 'r') as f_in, open(output_path, 'w') as f_out:
        for line in f_in:
            original_line = line
            stripped = line.strip()
            
            # Check if this line is a setting
            if stripped and not stripped.startswith('#') and not stripped.startswith('[') and '=' in stripped:
                parts = stripped.split('=', 1)
                key = parts[0].strip()
                default_val = parts[1].strip()
                
                # Logic:
                # 1. If key is in PATH_OVERRIDES, ensure we write the override
                # 2. If key is in IGNORE_KEYS, keep the DIST line (New Default)
                # 3. If key is in old_config:
                #    If old_value != default_val (New Default), use old_value.
                #    Else keep default.
                
                if key in PATH_OVERRIDES:
                    # Apply Override
                    val = PATH_OVERRIDES[key]
                    # Quote if text
                    if key != "MySQLExecutable": # binary path might not need quotes but usually config does
                         if not val.startswith('"'): val = f'"{val}"'
                    f_out.write(f'{key} = {val}\n')
                    continue

                if key in IGNORE_KEYS and config_type == "world":
                     f_out.write(line)
                     continue

                if key in old_config:
                    old_val = old_config[key]
                    
                    # Normalization for comparison
                    # Strip check for quotes
                    norm_old = old_val.strip('"')
                    norm_new = default_val.strip('"').split(" -")[0].strip() # Cleanup comments in dist value
                    
                    if norm_old != norm_new:
                        print(f"[{config_type}] Migrating {key}: {default_val} -> {old_val}")
                        f_out.write(f'{key} = {old_val}\n')
                    else:
                        f_out.write(line) # Keep new default
                else:
                    f_out.write(line) # Keep new default
            else:
                f_out.write(line)

print("Starting migration...")
migrate(NEW_WORLD_DIST, OLD_WORLD_PATH, NEW_WORLD_CONF, "world")
migrate(NEW_AUTH_DIST, OLD_AUTH_PATH, NEW_AUTH_CONF, "auth")
print("Migration complete.")
