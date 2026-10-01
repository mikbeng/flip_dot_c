from format import HEIGHT, WIDTH, new_frame, set_pixel


def generate(params: dict) -> list[list[list[int]]]:
    frame_count = int(params.get("frame_count", 300))
    ball_x = 5.0
    ball_y = 5.0
    vel_x = 0.8
    vel_y = 0.6
    frames: list[list[list[int]]] = []

    for _ in range(frame_count):
        buf = new_frame()
        ball_x += vel_x
        ball_y += vel_y

        if ball_x <= 1 or ball_x >= WIDTH - 2:
            vel_x = -vel_x
            ball_x = 1 if ball_x <= 1 else float(WIDTH - 2)
        if ball_y <= 1 or ball_y >= HEIGHT - 2:
            vel_y = -vel_y
            ball_y = 1 if ball_y <= 1 else float(HEIGHT - 2)

        center_x = int(ball_x)
        center_y = int(ball_y)
        for dy in range(-1, 2):
            for dx in range(-1, 2):
                set_pixel(buf, center_y + dy, center_x + dx, 1)
        frames.append(buf)

    return frames
