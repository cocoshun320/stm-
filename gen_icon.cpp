// gen_icon.cpp - 生成 STM32 一键安装程序的 ICO 图标文件
// 图标设计: 蓝色渐变圆角背景 + 白色芯片图案 + 引脚 + "S"字母
// 编译: g++ -O2 -o gen_icon.exe gen_icon.cpp
// 运行: gen_icon.exe  -> 生成 icon.ico

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <vector>

// RGBA 像素
struct Pixel {
    unsigned char b, g, r, a;
};

// 颜色常量
static const Pixel BLUE_DARK   = {140, 60, 10, 255};   // 深蓝
static const Pixel BLUE_LIGHT  = {200, 100, 30, 255};   // 亮蓝
static const Pixel WHITE       = {255, 255, 255, 255};
static const Pixel CHIP_COLOR  = {245, 230, 220, 255};  // 芯片浅色
static const Pixel GRAY        = {180, 180, 180, 255};  // 引脚灰色
static const Pixel PIXEL_TRANSPARENT = {0, 0, 0, 0};

// 创建 size x size 的图像
static std::vector<std::vector<Pixel>> CreateImage(int size) {
    std::vector<std::vector<Pixel>> img(size, std::vector<Pixel>(size, PIXEL_TRANSPARENT));

    int radius = size / 6;
    if (radius < 2) radius = 2;

    // 1. 圆角矩形蓝色渐变背景
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int margin = 1;
            bool inCorner = false;
            int cx = 0, cy = 0;

            if (x < radius + margin && y < radius + margin) {
                cx = radius + margin; cy = radius + margin; inCorner = true;
            } else if (x >= size - radius - margin && y < radius + margin) {
                cx = size - radius - margin - 1; cy = radius + margin; inCorner = true;
            } else if (x < radius + margin && y >= size - radius - margin) {
                cx = radius + margin; cy = size - radius - margin - 1; inCorner = true;
            } else if (x >= size - radius - margin && y >= size - radius - margin) {
                cx = size - radius - margin - 1; cy = size - radius - margin - 1; inCorner = true;
            }

            if (inCorner) {
                int dx = x - cx, dy = y - cy;
                if (dx * dx + dy * dy > radius * radius) continue; // 透明
            }

            // 垂直渐变
            float t = (float)y / size;
            img[y][x].b = (unsigned char)(BLUE_DARK.b * (1 - t) + BLUE_LIGHT.b * t);
            img[y][x].g = (unsigned char)(BLUE_DARK.g * (1 - t) + BLUE_LIGHT.g * t);
            img[y][x].r = (unsigned char)(BLUE_DARK.r * (1 - t) + BLUE_LIGHT.r * t);
            img[y][x].a = 255;
        }
    }

    // 2. 芯片主体（中心浅色方块）
    int chipMargin = size / 4;
    int cx1 = chipMargin, cy1 = chipMargin;
    int cx2 = size - chipMargin, cy2 = size - chipMargin;

    for (int y = cy1; y < cy2 && y < size; y++) {
        for (int x = cx1; x < cx2 && x < size; x++) {
            img[y][x] = CHIP_COLOR;
        }
    }

    // 3. 芯片引脚
    int pinLen = size / 10; if (pinLen < 2) pinLen = 2;
    int pinWidth = size / 16; if (pinWidth < 1) pinWidth = 1;
    int pinCount = 4;

    for (int side = 0; side < 4; side++) {
        for (int p = 0; p < pinCount; p++) {
            int pos = (p + 1) * (cx2 - cx1) / (pinCount + 1) + cx1;

            if (side == 0) { // 上
                for (int dy = 0; dy < pinLen; dy++)
                    for (int dx = 0; dx < pinWidth; dx++) {
                        int xx = pos + dx - pinWidth / 2;
                        int yy = cy1 - 1 - dy;
                        if (xx >= 0 && xx < size && yy >= 0 && yy < size)
                            img[yy][xx] = GRAY;
                    }
            } else if (side == 1) { // 下
                for (int dy = 0; dy < pinLen; dy++)
                    for (int dx = 0; dx < pinWidth; dx++) {
                        int xx = pos + dx - pinWidth / 2;
                        int yy = cy2 + dy;
                        if (xx >= 0 && xx < size && yy >= 0 && yy < size)
                            img[yy][xx] = GRAY;
                    }
            } else if (side == 2) { // 左
                int py = (p + 1) * (cy2 - cy1) / (pinCount + 1) + cy1;
                for (int dx = 0; dx < pinLen; dx++)
                    for (int dy = 0; dy < pinWidth; dy++) {
                        int xx = cx1 - 1 - dx;
                        int yy = py + dy - pinWidth / 2;
                        if (xx >= 0 && xx < size && yy >= 0 && yy < size)
                            img[yy][xx] = GRAY;
                    }
            } else { // 右
                int py = (p + 1) * (cy2 - cy1) / (pinCount + 1) + cy1;
                for (int dx = 0; dx < pinLen; dx++)
                    for (int dy = 0; dy < pinWidth; dy++) {
                        int xx = cx2 + dx;
                        int yy = py + dy - pinWidth / 2;
                        if (xx >= 0 && xx < size && yy >= 0 && yy < size)
                            img[yy][xx] = GRAY;
                    }
            }
        }
    }

    // 4. 中心绘制 "S" 字母
    if (size >= 32) {
        // S 字母 7x6 像素图案
        static const char* sPattern[] = {
            " 11111",
            "11    ",
            "11    ",
            " 1111 ",
            "    11",
            "    11",
            "11111 ",
        };
        int sW = 6, sH = 7;
        int scale = size / 32; if (scale < 1) scale = 1;
        int sX = (size - sW * scale) / 2;
        int sY = (size - sH * scale) / 2;

        for (int row = 0; row < sH; row++) {
            for (int col = 0; col < sW; col++) {
                if (sPattern[row][col] == '1') {
                    for (int dy = 0; dy < scale; dy++)
                        for (int dx = 0; dx < scale; dx++) {
                            int xx = sX + col * scale + dx;
                            int yy = sY + row * scale + dy;
                            if (xx >= 0 && xx < size && yy >= 0 && yy < size)
                                img[yy][xx] = BLUE_DARK;
                        }
                }
            }
        }
    }

    return img;
}

// 将图像转为 ICO 内嵌的 BMP 数据
static std::vector<unsigned char> ImageToBmpData(const std::vector<std::vector<Pixel>>& img, int size) {
    std::vector<unsigned char> data;

    int headerSize = 40;
    int width = size;
    int height = size * 2; // ICO 要求高度翻倍
    int bpp = 32;
    int xorSize = size * size * 4;
    int andRowBytes = ((size + 31) / 32) * 4;
    int andSize = andRowBytes * size;

    // BMP 信息头 (40 bytes, little-endian)
    auto push32 = [&](unsigned int v) {
        data.push_back(v & 0xFF);
        data.push_back((v >> 8) & 0xFF);
        data.push_back((v >> 16) & 0xFF);
        data.push_back((v >> 24) & 0xFF);
    };
    auto push16 = [&](unsigned short v) {
        data.push_back(v & 0xFF);
        data.push_back((v >> 8) & 0xFF);
    };

    push32(headerSize);
    push32(width);
    push32(height);
    push16(1);          // planes
    push16(bpp);        // bpp
    push32(0);          // compression
    push32(xorSize + andSize);  // image size
    push32(0);          // x ppm
    push32(0);          // y ppm
    push32(0);          // colors used
    push32(0);          // important colors

    // XOR 位图 (从下到上, BGRA)
    for (int y = size - 1; y >= 0; y--) {
        for (int x = 0; x < size; x++) {
            data.push_back(img[y][x].b);
            data.push_back(img[y][x].g);
            data.push_back(img[y][x].r);
            data.push_back(img[y][x].a);
        }
    }

    // AND 掩码 (1bit, 透明=1)
    for (int y = size - 1; y >= 0; y--) {
        int bitCount = 0;
        unsigned char byteVal = 0;
        int bytesInRow = 0;
        for (int x = 0; x < size; x++) {
            if (img[y][x].a < 128) byteVal |= (1 << (7 - bitCount));
            bitCount++;
            if (bitCount == 8) {
                data.push_back(byteVal);
                byteVal = 0;
                bitCount = 0;
                bytesInRow++;
            }
        }
        if (bitCount > 0) {
            data.push_back(byteVal);
            bytesInRow++;
        }
        // 补齐到 4 字节对齐
        while (bytesInRow < andRowBytes) {
            data.push_back(0);
            bytesInRow++;
        }
    }

    return data;
}

int main() {
    int sizes[] = {16, 32, 48, 64, 128, 256};
    int numSizes = 6;

    std::vector<std::vector<unsigned char>> bmpDatas;
    std::vector<int> sizeList;

    for (int i = 0; i < numSizes; i++) {
        auto img = CreateImage(sizes[i]);
        auto bmp = ImageToBmpData(img, sizes[i]);
        bmpDatas.push_back(bmp);
        sizeList.push_back(sizes[i]);
    }

    // 写 ICO 文件
    FILE* fp = fopen("icon.ico", "wb");
    if (!fp) {
        printf("Error: cannot create icon.ico\n");
        return 1;
    }

    // ICO 文件头
    unsigned short reserved = 0;
    unsigned short icoType = 1;
    unsigned short count = numSizes;
    fwrite(&reserved, 2, 1, fp);
    fwrite(&icoType, 2, 1, fp);
    fwrite(&count, 2, 1, fp);

    // 计算数据偏移
    int offset = 6 + numSizes * 16;

    // 目录条目
    for (int i = 0; i < numSizes; i++) {
        unsigned char w = (sizeList[i] == 256) ? 0 : (unsigned char)sizeList[i];
        unsigned char h = w;
        unsigned char colors = 0;
        unsigned char rsv = 0;
        unsigned short planes = 1;
        unsigned short bpp = 32;
        unsigned int dataSize = (unsigned int)bmpDatas[i].size();
        unsigned int off = (unsigned int)offset;

        fwrite(&w, 1, 1, fp);
        fwrite(&h, 1, 1, fp);
        fwrite(&colors, 1, 1, fp);
        fwrite(&rsv, 1, 1, fp);
        fwrite(&planes, 2, 1, fp);
        fwrite(&bpp, 2, 1, fp);
        fwrite(&dataSize, 4, 1, fp);
        fwrite(&off, 4, 1, fp);

        offset += dataSize;
    }

    // 图像数据
    for (int i = 0; i < numSizes; i++) {
        fwrite(bmpDatas[i].data(), 1, bmpDatas[i].size(), fp);
    }

    fclose(fp);

    // 报告
    printf("icon.ico created successfully!\n");
    printf("Sizes: ");
    for (int i = 0; i < numSizes; i++) {
        printf("%dx%d", sizeList[i], sizeList[i]);
        if (i < numSizes - 1) printf(", ");
    }
    printf("\n");

    return 0;
}
