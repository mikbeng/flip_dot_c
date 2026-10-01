import math

from format import HEIGHT, WIDTH, new_frame, set_pixel


def generate(params: dict) -> list[list[list[int]]]:
    frame_count = int(params.get("frame_count", 200))
    frames: list[list[list[int]]] = []

    for frame_idx in range(frame_count):
        buf = new_frame()
        for col in range(WIDTH):
            angle = float(col + frame_idx) * 0.3
            sine_val = math.sin(angle)
            row = int(((sine_val + 1.0) / 2.0) * (HEIGHT - 1))
            if 0 <= row < HEIGHT:
                set_pixel(buf, row, col, 1)
                angle2 = angle + (math.pi / 2.0)
                sine_val2 = math.sin(angle2)
                row2 = int(((sine_val2 + 1.0) / 2.0) * (HEIGHT - 1))
                if 0 <= row2 < HEIGHT and row2 != row:
                    set_pixel(buf, row2, col, 1)
        frames.append(buf)

    return frames
