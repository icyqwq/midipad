def rgb565_to_gray(r, g, b):
    # 将5位和6位值转换为8位值
    r = (r << 3) | (r >> 2)
    g = (g << 2) | (g >> 4)
    b = (b << 3) | (b >> 2)
    # 计算灰度值
    return int(0.299 * r + 0.587 * g + 0.114 * b)

def generate_lut():
    lut = []
    for i in range(65536):
        c = (i >> 8 )| (i << 8) # swap byte
        b = (c >> 11) & 0x1F  # 提取5位红色
        g = (c >> 5) & 0x3F   # 提取6位绿色
        r = c & 0x1F          # 提取5位蓝色
        gray = rgb565_to_gray(r, g, b)
        lut.append(gray)
    return lut

lut = generate_lut()

# 将输出写入到.h文件中
with open("rgb565_to_gray_lut.h", "w") as f:
    f.write("#ifndef RGB565_TO_GRAY_LUT_H\n")
    f.write("#define RGB565_TO_GRAY_LUT_H\n\n")
    f.write("const unsigned char rgb565_to_gray_lut[65536] = {\n")
    for i in range(0, len(lut), 8):  # 每行输出8个值
        f.write("    " + ", ".join(map(str, lut[i:i+8])) + ",\n")
    f.write("};\n\n")
    f.write("#endif // RGB565_TO_GRAY_LUT_H\n")