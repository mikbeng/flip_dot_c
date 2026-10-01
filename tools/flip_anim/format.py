"""28x13 flip-dot frame packing (v1)."""

from __future__ import annotations

WIDTH = 28
HEIGHT = 13
PIXEL_COUNT = WIDTH * HEIGHT
BYTES_PER_FRAME = (PIXEL_COUNT + 7) // 8


def new_frame() -> list[list[int]]:
    return [[0 for _ in range(WIDTH)] for _ in range(HEIGHT)]


def set_pixel(frame: list[list[int]], row: int, col: int, value: int = 1) -> None:
    if 0 <= row < HEIGHT and 0 <= col < WIDTH:
        frame[row][col] = 1 if value else 0


def pack_frame(frame: list[list[int]]) -> bytes:
    """Row-major grid; linear bit index row*WIDTH+col, MSB-first within each byte."""
    out = bytearray(BYTES_PER_FRAME)
    for row in range(HEIGHT):
        for col in range(WIDTH):
            if not frame[row][col]:
                continue
            idx = row * WIDTH + col
            byte_idx = idx // 8
            bit = 7 - (idx % 8)
            out[byte_idx] |= 1 << bit
    return bytes(out)


def pack_frames(frames: list[list[list[int]]]) -> bytes:
    return b"".join(pack_frame(f) for f in frames)
