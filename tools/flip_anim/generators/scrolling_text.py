from format import HEIGHT, WIDTH, new_frame, set_pixel

FONT_A = [0x1F, 0x24, 0x24, 0x24, 0x1F, 0x00, 0x00]
FONT_SPACE = [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]


def _glyph_for_char(ch: str) -> list[int]:
    if ch in ("A", "a"):
        return FONT_A
    return FONT_SPACE


def generate(params: dict) -> list[list[list[int]]]:
    text = str(params.get("text", "A"))
    text_len = len(text)
    total_width = text_len * 6
    frames: list[list[list[int]]] = []

    for offset in range(WIDTH, -total_width - 1, -1):
        buf = new_frame()
        for char_idx, ch in enumerate(text):
            char_x = offset + char_idx * 6
            font_data = _glyph_for_char(ch)
            for col in range(5):
                px = char_x + col
                if px < 0 or px >= WIDTH:
                    continue
                for row in range(7):
                    if row + 3 >= HEIGHT:
                        continue
                    pixel = (font_data[col] >> row) & 1
                    if pixel:
                        set_pixel(buf, row + 3, px, 1)
        frames.append(buf)

    return frames
