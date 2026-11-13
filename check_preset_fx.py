import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def load_mod_dst_count() -> int:
    text = (ROOT / "Source" / "DSP" / "ModMatrix.h").read_text(encoding="utf-8")
    m = re.search(r"enum\s+Dst\s*\{(.*?)\};", text, re.S)
    if not m:
        raise RuntimeError("Could not locate enum Dst in ModMatrix.h")
    entries = re.findall(r"\bDst[A-Za-z0-9_]+\b", m.group(1))
    # Remove DstNum sentinel
    return len([e for e in entries if e != "DstNum"])


def check_preset_data_h(dst_count: int) -> list[str]:
    text = (ROOT / "Source" / "Presets" / "PresetData.h").read_text(encoding="utf-8")
    presets = re.findall(
        r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*"([^"]*)"\s*,\s*\{(.*?)\}\s*\}',
        text,
        re.S,
    )
    errors: list[str] = []
    if len(presets) != 120:
        errors.append(f"PresetData.h count expected 120, got {len(presets)}")

    for name, cat, _sub, body in presets:
        pairs = re.findall(r'\{\s*"([^"]+)"\s*,\s*([-+0-9.eEf]+)\s*\}', body)
        fx_seen: dict[int, str] = {}
        for k, val_s in pairs:
            val = float(val_s.rstrip("fF"))
            if k.startswith("fx6"):
                errors.append(f"[{cat}/{name}] invalid parameter {k}")
            if k in ("fx1Type", "fx2Type", "fx3Type", "fx4Type", "fx5Type"):
                t = int(round(val))
                if t != 0:
                    if t in fx_seen:
                        errors.append(
                            f"[{cat}/{name}] duplicate FX type {t} on {fx_seen[t]} and {k}"
                        )
                    fx_seen[t] = k
            if re.fullmatch(r"mod[1-8]Dst", k):
                d = int(round(val))
                if d < 0 or d >= dst_count:
                    errors.append(f"[{cat}/{name}] {k}={d} out of [0, {dst_count - 1}]")
    return errors


def main() -> int:
    dst_count = load_mod_dst_count()
    gen_text = (ROOT / "generate_presets.py").read_text(encoding="utf-8")
    errors = []

    if re.search(r'"fx6(Type|Amount)"', gen_text):
        errors.append("generate_presets.py still contains fx6Type/fx6Amount")

    errors.extend(check_preset_data_h(dst_count))

    if errors:
        print(f"Found {len(errors)} preset issues:")
        for e in errors:
            print("  ERROR:", e)
        return 1

    print(f"OK: 120 presets in PresetData.h valid (0 duplicate FX, 0 fx6*, ModDst < {dst_count})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
