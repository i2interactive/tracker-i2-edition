"""Fonts for the host harness. The Tracker's fonts 0-4 are Liberation Sans
(core/libs/fonts): render.py draws them with the Liberation Sans TTF, and
gen_metrics.py takes their widths from the firmware's own font tables, so
measuring is exact. The FT812 ROM fonts are approximated with DejaVu."""
DEJAVU = "/usr/share/fonts/truetype/dejavu/"
LIBERATION = "/usr/share/fonts/truetype/liberation/"
REG = LIBERATION + "LiberationSans-Regular.ttf"
BOLD = LIBERATION + "LiberationSans-Bold.ttf"
ROM = DEJAVU + "DejaVuSans.ttf"

# font id -> the firmware's font table (core/libs/fonts/<name>_L4.cpp)
FIRMWARE = {
    4: "LiberationSans-Regular_12",
    0: "LiberationSans-Regular_14",
    2: "LiberationSans-Bold_14",
    1: "LiberationSans-Regular_17",
    3: "LiberationSans-Bold_17",
}

# font id -> (ttf, pixel size, line height)
FONTS = {
    4: (REG, 12, 14),   # 12 regular
    0: (REG, 14, 16),   # 14 regular
    2: (BOLD, 14, 16),  # 14 bold
    1: (REG, 17, 20),   # 17 regular
    3: (BOLD, 17, 20),  # 17 bold
    27: (ROM, 16, 20),
    28: (ROM, 20, 25),
    29: (ROM, 23, 28),
    30: (ROM, 29, 36),
    31: (ROM, 40, 49),
}
