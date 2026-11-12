"""Font stand-ins for the host harness: the Tracker's fonts and the FT812 ROM
fonts approximated with DejaVu. Used by gen_metrics.py and render.py."""
DEJAVU = "/usr/share/fonts/truetype/dejavu/"
REG = DEJAVU + "DejaVuSansCondensed.ttf"
BOLD = DEJAVU + "DejaVuSansCondensed-Bold.ttf"
ROM = DEJAVU + "DejaVuSans.ttf"

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
