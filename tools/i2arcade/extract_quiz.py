#!/usr/bin/env python3
"""Build the Quiz Kagemori SD-card bank from i2 OS 2000's kquiz.js.

Usage: extract_quiz.py <path/to/kquiz.js> <sdcard root>

Writes <root>/i2/Arcade/Quiz/{questions,order,odd,true}.txt, the four
pipe-separated tables the Tracker edition reads. The bank is i2 interactive's
own content and is not stored in this repository.
"""
import re
import sys
from pathlib import Path

TABLES = {
    "KQ_RAW": ("questions.txt", 6),
    "KQ_ORD": ("order.txt", 5),
    "KQ_ODD": ("odd.txt", 5),
    "KQ_TRUE": ("true.txt", 4),
}

# Characters the Tracker's fonts can't draw, mapped to plain ASCII
ASCII = {
    "‘": "'", "’": "'", "“": '"', "”": '"', "–": "-", "—": "-",
    "…": "...", "×": "x", "°": " deg", "é": "e", "è": "e", "ê": "e",
    "á": "a", "à": "a", "â": "a", "ä": "a", "ñ": "n", "ö": "o",
    "ü": "u", "í": "i", "ó": "o", "ú": "u", "ç": "c", "²": "2",
    "³": "3", "½": "1/2", "¼": "1/4", "¾": "3/4", " ": " ", "−": "-",
    "ø": "o", "É": "E", "å": "a", "π": "pi",
}


def to_ascii(s):
    out = []
    for ch in s:
        if ord(ch) < 128:
            out.append(ch)
        elif ch in ASCII:
            out.append(ASCII[ch])
        else:
            out.append("?")
    return "".join(out)


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    src = Path(sys.argv[1]).read_text(encoding="utf-8")
    out_dir = Path(sys.argv[2]) / "i2" / "Arcade" / "Quiz"
    out_dir.mkdir(parents=True, exist_ok=True)
    unknown = set()
    for name, (fname, fields) in TABLES.items():
        m = re.search(r"const " + name + r" = `(.*?)`;", src, re.S)
        if not m:
            print("missing table", name)
            return 1
        lines = []
        for raw in m.group(1).split("\n"):
            raw = raw.strip()
            if not raw:
                continue
            parts = raw.split("|")
            if len(parts) < fields:
                print("skipping short line in", name, ":", raw[:60])
                continue
            for ch in raw:
                if ord(ch) >= 128 and ch not in ASCII:
                    unknown.add(ch)
            lines.append(to_ascii(raw))
        (out_dir / fname).write_text("\n".join(lines) + "\n", encoding="ascii")
        print(f"{fname}: {len(lines)} lines")
    if unknown:
        print("replaced with '?':", " ".join(sorted(unknown)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
