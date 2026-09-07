#!/usr/bin/env python3
"""Build Turkish text and voice assets from an extracted game content tree.

The pipeline is deliberately format-preserving for common text resources. It
does not decrypt or modify game binaries. Translation is optional and uses an
OpenAI-compatible HTTP endpoint when --translate is supplied.
"""

import argparse
import hashlib
import json
import re
import sys
import urllib.request
from pathlib import Path


TEXT_EXTENSIONS = {
    ".csv", ".dialogue", ".ini", ".json", ".loc", ".po", ".sub", ".txt",
    ".xml", ".yaml", ".yml",
}
PLACEHOLDER = re.compile(r"(\{[^{}]+\}|%\d*\$?[sdif]|<[^<>]+>|\\n)")
ASCII_TOKEN = re.compile(r"^[\W_\d]+$", re.UNICODE)


def protect_placeholders(text):
    values = []

    def replace(match):
        values.append(match.group(0))
        return f"__PLACEHOLDER_{len(values) - 1}__"

    return PLACEHOLDER.sub(replace, text), values


def restore_placeholders(text, values):
    for index, value in enumerate(values):
        text = text.replace(f"__PLACEHOLDER_{index}__", value)
    return text


def candidate(line):
    value = line.strip()
    if len(value) < 2 or ASCII_TOKEN.match(value):
        return False
    if value.startswith(("//", "#", ";", "<!--", "/*")):
        return False
    return any(character.isalpha() for character in value)


def key_for(path, line_number, source):
    digest = hashlib.sha256(f"{path}:{line_number}:{source}".encode("utf-8")).hexdigest()
    return digest[:16]


def scan(root):
    entries = []
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in TEXT_EXTENSIONS:
            continue
        try:
            lines = path.read_text(encoding="utf-8-sig").splitlines(keepends=True)
        except UnicodeDecodeError:
            continue
        for line_number, line in enumerate(lines, 1):
            source = line.rstrip("\r\n")
            if candidate(source):
                entries.append({
                    "id": key_for(path.relative_to(root).as_posix(), line_number, source),
                    "file": path.relative_to(root).as_posix(),
                    "line": line_number,
                    "source": source,
                    "target": "",
                })
    return entries


def ask_translation(endpoint, model, source, api_key):
    protected, placeholders = protect_placeholders(source)
    body = {
        "model": model,
        "temperature": 0.1,
        "messages": [{
            "role": "system",
            "content": (
                "Translate game text to natural Turkish. Return only the translation. "
                "Preserve names, punctuation, line breaks and every placeholder token."
            ),
        }, {"role": "user", "content": protected}],
    }
    request = urllib.request.Request(
        endpoint.rstrip("/") + "/v1/chat/completions",
        data=json.dumps(body).encode("utf-8"),
        headers={
            "Content-Type": "application/json",
            **({"Authorization": f"Bearer {api_key}"} if api_key else {}),
        },
    )
    with urllib.request.urlopen(request, timeout=120) as response:
        payload = json.load(response)
    translated = payload["choices"][0]["message"]["content"].strip()
    return restore_placeholders(translated, placeholders)


def translate_entries(entries, endpoint, model, api_key):
    for index, entry in enumerate(entries, 1):
        if not entry["target"]:
            entry["target"] = ask_translation(endpoint, model, entry["source"], api_key)
        print(f"translated {index}/{len(entries)}", file=sys.stderr)


def export(entries, source_root, output_root):
    by_file = {}
    for entry in entries:
        by_file.setdefault(entry["file"], {})[entry["line"]] = entry["target"] or entry["source"]
    for relative, replacements in by_file.items():
        source_path = source_root / relative
        lines = source_path.read_text(encoding="utf-8-sig").splitlines(keepends=True)
        for line_number, target in replacements.items():
            ending = "\n" if lines[line_number - 1].endswith("\n") else ""
            lines[line_number - 1] = target + ending
        destination = output_root / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text("".join(lines), encoding="utf-8")


def write_voice_manifest(entries, output):
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8") as file:
        for entry in entries:
            if entry["target"]:
                file.write(json.dumps({
                    "id": entry["id"],
                    "source": entry["source"],
                    "text": entry["target"],
                    "audio": f"audio/{entry['id']}.opus",
                }, ensure_ascii=False) + "\n")


def main():
    parser = argparse.ArgumentParser(description="Extract, translate and prepare Turkish game assets")
    parser.add_argument("content", type=Path, help="Extracted game text/resource directory")
    parser.add_argument("--catalog", type=Path, default=Path("build/catalog.json"))
    parser.add_argument("--output", type=Path, default=Path("build/turkish-romfs"))
    parser.add_argument("--translate", action="store_true")
    parser.add_argument("--endpoint", default="http://127.0.0.1:11434")
    parser.add_argument("--model", default="qwen2.5:3b")
    parser.add_argument("--api-key", default="")
    parser.add_argument("--voice-manifest", type=Path, default=Path("build/voice-manifest.jsonl"))
    args = parser.parse_args()

    entries = scan(args.content)
    if args.translate:
        translate_entries(entries, args.endpoint, args.model, args.api_key)
    args.catalog.parent.mkdir(parents=True, exist_ok=True)
    args.catalog.write_text(json.dumps(entries, ensure_ascii=False, indent=2), encoding="utf-8")
    if any(entry["target"] for entry in entries):
        export(entries, args.content, args.output)
        write_voice_manifest(entries, args.voice_manifest)
    print(f"found {len(entries)} translatable entries")


if __name__ == "__main__":
    main()