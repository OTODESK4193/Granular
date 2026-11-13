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
        r'^\s*\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*"([^"]*)"\s*,\s*\{(.+)\}\s*\}\s*,?\s*$',
        text,
        re.M,
    )
    errors: list[str] = []
    if len(presets) != 150:
        errors.append(f"PresetData.h count expected 150, got {len(presets)}")

    seen_names: set[str] = set()
    for name, cat, _sub, body in presets:
        if name in seen_names:
            errors.append(f"[{cat}/{name}] duplicate preset name")
        seen_names.add(name)

        pairs = re.findall(r'\{\s*"([^"]+)"\s*,\s*([-+0-9.eEf]+)\s*\}', body)
        pmap: dict[str, float] = {}
        fx_seen: dict[int, str] = {}
        for k, val_s in pairs:
            val = float(val_s.rstrip("fF"))
            pmap[k] = val
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
            if re.fullmatch(r"mod(?:[1-9]|1[0-6])Dst", k):
                d = int(round(val))
                if d < 0 or d >= dst_count:
                    errors.append(f"[{cat}/{name}] {k}={d} out of [0, {dst_count - 1}]")
            if re.fullmatch(r"mod(?:[1-9]|1[0-6])Src", k):
                s = int(round(val))
                if s < 0 or s > 12:
                    errors.append(f"[{cat}/{name}] {k}={s} out of [0, 12]")

        for slot in range(1, 6):
            t = int(round(pmap.get(f"fx{slot}Type", 0.0)))
            a = pmap.get(f"fx{slot}Amount", 0.0)
            if t != 0 and a <= 0.0:
                errors.append(f"[{cat}/{name}] fx{slot}Type={t} has missing/zero fx{slot}Amount")

        for m in range(1, 17):
            s = int(round(pmap.get(f"mod{m}Src", 0.0)))
            d = int(round(pmap.get(f"mod{m}Dst", 0.0)))
            a = pmap.get(f"mod{m}Amt", 0.0)
            any_set = any(f"mod{m}{suf}" in pmap for suf in ("Src", "Dst", "Amt"))
            if any_set and (s == 0 or d == 0 or abs(a) < 1e-6):
                errors.append(f"[{cat}/{name}] incomplete mod{m} slot (src={s}, dst={d}, amt={a})")

        if int(round(pmap.get("scaleOn", 0.0))) == 1 and int(round(pmap.get("scale", 0.0))) == 0:
            errors.append(f"[{cat}/{name}] scaleOn=1 with Chromatic(0) scale")

        if cat == "Pitch" and pmap.get("pitchJitter", 0.0) > 20.0:
            errors.append(f"[{cat}/{name}] Pitch jitter {pmap.get('pitchJitter')} > 20")

        if pmap.get("satDrive", 2.0) >= 6.0 and pmap.get("satTrim", 0.0) >= 0.0:
            errors.append(f"[{cat}/{name}] satDrive >= 6.0 missing negative satTrim")

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

    print(f"OK: 150 presets in PresetData.h valid (0 duplicate FX, 0 fx6*, ModDst < {dst_count}, Phase 1-4 checks passed)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
