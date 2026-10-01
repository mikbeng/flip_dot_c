import math

from format import HEIGHT, WIDTH, new_frame, set_pixel


def generate(params: dict) -> list[list[list[int]]]:
    frame_count = int(params.get("frame_count", 100))
    center_x = WIDTH / 2.0
    center_y = HEIGHT / 2.0
    frames: list[list[list[int]]] = []

    for frame_idx in range(frame_count):
        buf = new_frame()
        radius1 = frame_idx * 0.5
        radius2 = (frame_idx - 20) * 0.5 if frame_idx > 20 else 0.0
        radius3 = (frame_idx - 40) * 0.5 if frame_idx > 40 else 0.0

        for row in range(HEIGHT):
            for col in range(WIDTH):
                dist = math.sqrt((col - center_x) ** 2 + (row - center_y) ** 2)
                on_ripple = False
                if radius1 > 0 and abs(dist - radius1) < 0.8:
                    on_ripple = True
                if radius2 > 0 and abs(dist - radius2) < 0.8:
                    on_ripple = True
                if radius3 > 0 and abs(dist - radius3) < 0.8:
                    on_ripple = True
                if on_ripple:
                    set_pixel(buf, row, col, 1)
        frames.append(buf)

    return frames
