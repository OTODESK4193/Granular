# -*- coding: utf-8 -*-
import re

with open("Source/Presets/PresetData.h", "r", encoding="utf-8") as f:
    text = f.read()

# find each preset block
matches = re.findall(r'\{\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]*)",\s*\{\s*(.+?)\s*\}\s*\},', text)
print(f"Parsed {len(matches)} presets from PresetData.h")

errors = 0
for name, cat, sub, p_str in matches:
    fx_dict = {}
    tokens = re.findall(r'\{"([^"]+)",([0-9\.\-f]+)\}', p_str)
    for k, v in tokens:
        if k in ("fx1Type", "fx2Type", "fx3Type", "fx4Type", "fx5Type", "fx6Type"):
            val = int(float(v.replace("f", "")))
            if val != 0:
                if val in fx_dict:
                    print(f"DUPLICATE FX: '{name}' ({cat}/{sub}) has duplicate type {val} in {fx_dict[val]} and {k}")
                    errors += 1
                fx_dict[val] = k

if errors == 0 and len(matches) == 120:
    print(f"SUCCESS: All {len(matches)} presets validated! Zero FX duplicates in any preset!")
else:
    print(f"FAILED: {errors} duplicates found, or match count != 120 (got {len(matches)})")
    exit(1)
