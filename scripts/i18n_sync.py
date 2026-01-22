"""
i18n_sync.py — synchronize missing translation keys from en_US.json to other language files

Usage:
  python scripts/i18n_sync.py [--apply]

- By default the script runs in dry-run and reports missing keys.
- With --apply it writes the missing keys into the target JSON files, marking values with a prefix.

Behavior:
- Preserves the top-level `name` field and the existing translations.
- Adds any missing keys from en_US.json -> other files under assets/lang/*.json
- Marks inserted values with "[MISSING TRANSLATION] " + English text so translators can find them.
"""

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LANG_DIR = ROOT / "assets" / "lang"
EN = LANG_DIR / "en_US.json"


def load(path: Path):
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def write(path: Path, data):
    with path.open("w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=4, sort_keys=False)
        f.write("\n")


def main(dry_run: bool):
    en = load(EN)
    en_trans = en.get("translations", {})

    results = []
    for p in sorted(LANG_DIR.glob("*.json")):
        if p.name == EN.name:
            continue
        data = load(p)
        trans = data.get("translations", {})
        missing = []
        for k, v in en_trans.items():
            if k not in trans:
                missing.append(k)
                trans[k] = "[MISSING TRANSLATION] " + v
        if missing:
            results.append((p.name, missing))
            if not dry_run:
                data["translations"] = trans
                write(p, data)

    if not results:
        print("No missing keys found — all good ✅")
        return 0

    for fname, keys in results:
        print(f"{fname}: {len(keys)} missing keys")
        for k in keys[:20]:
            print("  ", k)
        if len(keys) > 20:
            print("   ...\n")

    if dry_run:
        print("\nRun with --apply to write these changes to the files.")
    else:
        print("\nWrote missing keys. Please review and commit the changes.")
    return 0


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument("--apply", dest="apply", action="store_true", help="Write changes to files")
    args = ap.parse_args()
    raise SystemExit(main(dry_run=not args.apply))
