#!/usr/bin/env python3
"""Bake flip-dot animation clips and emit animations_gen.c / .h for ESP-IDF."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import yaml

# Allow imports from this directory when run as a script.
SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from emit_c import ClipSpec, clip_id_to_enum, emit_header, emit_source, _sanitize_symbol
from format import pack_frames
from generators import GENERATORS


def load_manifest(path: Path) -> dict:
    with path.open(encoding="utf-8") as f:
        return yaml.safe_load(f)


def build_clips(manifest: dict) -> tuple[list[ClipSpec], dict[str, str]]:
    clips_cfg = manifest.get("clips", [])
    clip_specs: list[ClipSpec] = []
    id_to_enum: dict[str, str] = {}

    for entry in clips_cfg:
        clip_id = entry["id"]
        generator_name = entry["generator"]
        delay_ms = int(entry["delay_ms"])
        params = dict(entry.get("params") or {})

        if generator_name not in GENERATORS:
            raise KeyError(f"Unknown generator {generator_name!r} for clip {clip_id!r}")

        frames = GENERATORS[generator_name](params)
        packed = pack_frames(frames)
        enum_name = clip_id_to_enum(clip_id)
        c_symbol = _sanitize_symbol(clip_id)
        id_to_enum[clip_id] = enum_name

        clip_specs.append(
            ClipSpec(
                enum_name=enum_name,
                c_symbol=c_symbol,
                frame_count=len(frames),
                delay_ms=delay_ms,
                packed=packed,
            )
        )

    return clip_specs, id_to_enum


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=SCRIPT_DIR / "manifest.yaml")
    parser.add_argument("--out-c", type=Path, required=True)
    parser.add_argument("--out-h", type=Path, required=True)
    args = parser.parse_args()

    manifest = load_manifest(args.manifest)
    clips, id_to_enum = build_clips(manifest)
    playlists = manifest.get("playlists") or {}

    from format import BYTES_PER_FRAME

    header = emit_header(clips, playlists, id_to_enum)
    source = emit_source(clips, playlists, id_to_enum, BYTES_PER_FRAME)

    args.out_h.parent.mkdir(parents=True, exist_ok=True)
    args.out_c.parent.mkdir(parents=True, exist_ok=True)
    args.out_h.write_text(header, encoding="utf-8")
    args.out_c.write_text(source, encoding="utf-8")
    print(f"Wrote {len(clips)} clips -> {args.out_c}, {args.out_h}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
