import math
import random

from format import HEIGHT, WIDTH, new_frame, set_pixel


def generate(params: dict) -> list[list[list[int]]]:
    frame_count = int(params.get("frame_count", 500))
    seed = int(params.get("seed", 0))
    rng = random.Random(seed)

    drop_pos = [-(rng.randint(0, 9)) for _ in range(WIDTH)]
    drop_length = [3 + rng.randint(0, 4) for _ in range(WIDTH)]
    frames: list[list[list[int]]] = []

    for _ in range(frame_count):
        buf = new_frame()
        for col in range(WIDTH):
            for i in range(drop_length[col]):
                row = drop_pos[col] - i
                if 0 <= row < HEIGHT:
                    pixel_on = (i < drop_length[col] // 2) or (rng.randint(0, 2) == 0)
                    if pixel_on:
                        set_pixel(buf, row, col, 1)
            drop_pos[col] += 1
            if drop_pos[col] > HEIGHT + drop_length[col]:
                drop_pos[col] = -(rng.randint(0, 9))
                drop_length[col] = 3 + rng.randint(0, 4)
        frames.append(buf)

    return frames
