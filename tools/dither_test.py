import random

# Assuming you have a lookup table for RGB565 to grayscale conversion
rgb565_to_gray_lut = [random.randint(0, 255) for _ in range(65536)]  # Dummy LUT for demonstration

def _calc2x2Block(image, width, height, row, column):
    base0 = row * width + column
    base1 = base0 + width
    return (
        rgb565_to_gray_lut[image[base0]] + 
        rgb565_to_gray_lut[image[base1]] + 
        rgb565_to_gray_lut[image[base0 + 1]] + 
        rgb565_to_gray_lut[image[base1 + 1]]) >> 2

def _applyDitheredVal(grayscale):
    line0, line1 = 0, 0
    line0 <<= 2
    line1 <<= 2
    if grayscale < 51:
        line0 |= 0b00
        line1 |= 0b00
    elif grayscale < 102:
        line0 |= 0b01
        line1 |= 0b00
    elif grayscale < 153:
        line0 |= 0b01
        line1 |= 0b10
    elif grayscale < 204:
        line0 |= 0b01
        line1 |= 0b11
    else:
        line0 |= 0b11
        line1 |= 0b11
    return line0, line1

def ditheringImg(image, width, height):
    dst = [0] * (width * height // 8)
    dst_width = width >> 3
    for row in range(0, height, 2):
        base = row * dst_width
        for column in range(0, width, 8):
            g0 = _calc2x2Block(image, width, height, row, column + 0)
            g1 = _calc2x2Block(image, width, height, row, column + 2)
            g2 = _calc2x2Block(image, width, height, row, column + 4)
            g3 = _calc2x2Block(image, width, height, row, column + 6)

            line0_0, line1_0 = _applyDitheredVal(g0)
            line0_1, line1_1 = _applyDitheredVal(g1)
            line0_2, line1_2 = _applyDitheredVal(g2)
            line0_3, line1_3 = _applyDitheredVal(g3)

            line0 = (line0_0 << 6) | (line0_1 << 4) | (line0_2 << 2) | line0_3
            line1 = (line1_0 << 6) | (line1_1 << 4) | (line1_2 << 2) | line1_3

            idx_0 = base + (column >> 3)
            idx_1 = idx_0 + dst_width

            dst[idx_0] = line0
            dst[idx_1] = line1
    return dst

# Generate a random RGB565 image of size 16x4
image = [random.randint(0, 65535) for _ in range(16*4)]

# Dither the image
dithered_image = ditheringImg(image, 16, 4)

# Print the dithered image
for i in range(0, len(dithered_image), 2):
    print(f"{dithered_image[i]:08b} {dithered_image[i+1]:08b}")