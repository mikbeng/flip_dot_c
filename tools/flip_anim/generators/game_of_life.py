import random

from format import HEIGHT, WIDTH, new_frame, set_pixel


def generate(params: dict) -> list[list[list[int]]]:
    generations = int(params.get("generations", 100))
    seed = int(params.get("seed", 0))
    rng = random.Random(seed)

    current = new_frame()
    for row in range(HEIGHT):
        for col in range(WIDTH):
            current[row][col] = 1 if rng.randint(0, 2) == 0 else 0

    frames: list[list[list[int]]] = []

    for _ in range(generations):
        next_gen = new_frame()
        for row in range(HEIGHT):
            for col in range(WIDTH):
                neighbors = 0
                for dr in (-1, 0, 1):
                    for dc in (-1, 0, 1):
                        if dr == 0 and dc == 0:
                            continue
                        nr = row + dr
                        nc = col + dc
                        if nr < 0:
                            nr = HEIGHT - 1
                        elif nr >= HEIGHT:
                            nr = 0
                        if nc < 0:
                            nc = WIDTH - 1
                        elif nc >= WIDTH:
                            nc = 0
                        if current[nr][nc]:
                            neighbors += 1
                if current[row][col]:
                    next_gen[row][col] = 1 if neighbors in (2, 3) else 0
                else:
                    next_gen[row][col] = 1 if neighbors == 3 else 0
        frames.append([row[:] for row in next_gen])
        current = next_gen

    return frames
