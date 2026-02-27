import os

# Paths
WORKSPACE = "/home/ramires/projects/Custom_AC_WotLK_With_NPCBots"
OLD_MODULES_DIR = "/mnt/e/games/WoWPrivateNew/bin/bin/Release/configs/modules"
NEW_MODULES_DIR = os.path.join(WORKSPACE, "env/dist/etc/modules")

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
                # Remove inline comments manually
                if "  -" in val:
                     val = val.split("  -")[0].strip()
                config[key] = val
    return config

def migrate_file(dist_path, old_config_path, output_path):
    old_config = load_config(old_config_path)
    if not old_config:
        print(f"Skipping {output_path} (No old config found)")
        return

    # If output exists (already copied .dist to .conf by build), read it as base??
    # Actually, we should treat the .dist file in the new location as the source of truth for DEFAULTS,
    # and the .conf file in the old location as the source of VALUES.
    # The output should be the .conf file in the new location.
    
    with open(dist_path, 'r') as f_in, open(output_path, 'w') as f_out:
        for line in f_in:
            stripped = line.strip()
            
            # Check if this line is a setting
            if stripped and not stripped.startswith('#') and not stripped.startswith('[') and '=' in stripped:
                parts = stripped.split('=', 1)
                key = parts[0].strip()
                default_val = parts[1].strip()
                
                if key in old_config:
                    old_val = old_config[key]
                    
                    # Normalization
                    norm_old = old_val.strip('"')
                    norm_new = default_val.strip('"').split(" -")[0].strip() 
                    
                    if norm_old != norm_new:
                        print(f"Updating {key}: {default_val} -> {old_val}")
                        f_out.write(f'{key} = {old_val}\n')
                    else:
                        f_out.write(line)
                else:
                    f_out.write(line)
            else:
                f_out.write(line)

print("Starting module migration...")

if not os.path.exists(NEW_MODULES_DIR):
    print(f"Error: {NEW_MODULES_DIR} does not exist.")
    exit(1)

# Iterate over .dist files in new modules dir
for filename in os.listdir(NEW_MODULES_DIR):
    if filename.endswith(".conf.dist"):
        conf_name = filename[:-5] # remove .dist to get .conf
        
        # Check if we have this config in the old folder
        # Old folder might have .conf or .conf.dist. We prefer .conf
        old_conf_path = os.path.join(OLD_MODULES_DIR, conf_name)
        new_dist_path = os.path.join(NEW_MODULES_DIR, filename)
        new_conf_path = os.path.join(NEW_MODULES_DIR, conf_name)

        if os.path.exists(old_conf_path):
            print(f"Migrating {conf_name}...")
            migrate_file(new_dist_path, old_conf_path, new_conf_path)
        else:
            # If old config doesn't exist, we just check if we need to create the default .conf from .dist
            # But the build process usually copies .dist to .conf. 
            # If it didn't, we should copy it.
            if not os.path.exists(new_conf_path):
                # Simple copy
                print(f"Creating default {conf_name}...")
                with open(new_dist_path, 'r') as src, open(new_conf_path, 'w') as dst:
                    dst.write(src.read())

print("Module migration complete.")
