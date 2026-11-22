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

        # v1.2.0 Anti-howling safety bounds
        has_freeze = any(int(round(pmap.get(f"fx{s}Type", 0.0))) == 4 for s in range(1, 6))
        if has_freeze:
            if "frzFeedback" not in pmap or "frzDamp" not in pmap:
                errors.append(f"[{cat}/{name}] Freeze preset missing explicit frzFeedback/frzDamp")
            elif pmap["frzFeedback"] > 0.85 or pmap["frzDamp"] < 0.32:
                errors.append(f"[{cat}/{name}] Freeze feedback/damp out of safe bounds")

        if pmap.get("dlyFeedback", 0.35) > 0.68:
            errors.append(f"[{cat}/{name}] dlyFeedback {pmap.get('dlyFeedback')} > 0.68")

        if pmap.get("revDecay", 0.7) > 0.88 or pmap.get("revShimmer", 0.4) > 0.68:
            errors.append(f"[{cat}/{name}] revDecay/revShimmer exceeds anti-howling ceiling")

        tone_val = pmap.get("tone", 0.5)
        pos_tone_mod = sum(
            max(0.0, pmap.get(f"mod{m}Amt", 0.0))
            for m in range(1, 17)
            if int(round(pmap.get(f"mod{m}Dst", 0.0))) == 9
        )
        if tone_val > 0.82 or (tone_val + pos_tone_mod) > 0.90 + 1e-6:
            errors.append(f"[{cat}/{name}] tone/mod ceiling exceeded ({tone_val} + {pos_tone_mod:.2f})")

        modal_mix = pmap.get("modalMix", 0.0)
        if modal_mix > 0.60 or pmap.get("modalDecay", 0.8) > 2.2:
            errors.append(f"[{cat}/{name}] modalMix/modalDecay exceeds anti-howling ceiling")
        if modal_mix >= 0.30 and (tone_val > 0.74 or pmap.get("modalDamp", 0.4) < 0.32):
            errors.append(f"[{cat}/{name}] stacked Modal+Tone or low modalDamp")

        if pmap.get("grainFilterRes", 0.7) > 3.6:
            errors.append(f"[{cat}/{name}] grainFilterRes {pmap.get('grainFilterRes')} > 3.6")

        # v1.2.1 Unipolar boundary dead-zone checks
        for m in range(1, 17):
            d_val = int(round(pmap.get(f"mod{m}Dst", 0.0)))
            a_val = pmap.get(f"mod{m}Amt", 0.0)
            u_val = int(round(pmap.get(f"mod{m}Uni", 1.0)))
            if u_val == 1:
                if d_val == 14 and pmap.get("masterLPF", 20000.0) >= 19500.0 and a_val > 0.0:
                    errors.append(f"[{cat}/{name}] clamped +LPF mod on 20kHz (mod{m})")
                if d_val == 7 and pmap.get("panSpread", 1.0) >= 0.90 and a_val > 0.0:
                    errors.append(f"[{cat}/{name}] clamped +Pan mod at max (mod{m})")
                if d_val == 1 and pmap.get("position", 0.0) <= 0.01 and a_val < 0.0:
                    errors.append(f"[{cat}/{name}] clamped -Pos mod at 0 (mod{m})")

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
