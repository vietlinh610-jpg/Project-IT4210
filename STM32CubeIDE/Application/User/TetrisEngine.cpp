/*
 * TetrisEngine.cpp
 *
 *  Created on: Jun 3, 2025
 *      Author: admin
 */

#include "TetrisEngine.hpp"
#include "cmsis_os.h"

// Macro chuyển đổi RGB888 sang RGB565 chuẩn cho STM32
#define RGB565(r, g, b) __builtin_bswap16((uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)))

namespace {
const int Tetrominoes[11][4][4] = {
        // --- 7 KHỐI CƠ BẢN ---
        { { 0, 0, 0, 0 }, { 1, 1, 1, 1 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }, // 0: I
        { { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }, // 1: O
        { { 0, 1, 0, 0 }, { 1, 1, 1, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }, // 2: T
        { { 0, 1, 1, 0 }, { 1, 1, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }, // 3: S
        { { 1, 1, 0, 0 }, { 0, 1, 1, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }, // 4: Z
        { { 1, 0, 0, 0 }, { 1, 1, 1, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }, // 5: J
        { { 0, 0, 1, 0 }, { 1, 1, 1, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }, // 6: L

        // --- 4 ĐẠO CỤ MỚI (1x1) ---
        { { 1, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }, // 7: Bom
        { { 1, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }, // 8: Clear cột
        { { 1, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }, // 9: Clear hàng
        { { 1, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }, // 10: Khối đá
};

const uint16_t ColorPallette[11] = {
        RGB565(230, 40, 40),    // 0: Red
        RGB565(40, 200, 40),    // 1: Green
        RGB565(40, 100, 240),   // 2: Blue
        RGB565(255, 140, 0),    // 3: Orange-ish
        RGB565(240, 220, 20),   // 4: Yellow
        RGB565(200, 40, 200),   // 5: Magenta
        RGB565(240, 240, 240),  // 6: White
        RGB565(255, 69, 0),     // 7: Bom (Đỏ cam lửa cháy)
        RGB565(0, 255, 255),    // 8: Clear Cột (Cyan)
        RGB565(127, 255, 212),  // 9: Clear hàng (Aquamarine - Xanh ngọc)
        RGB565(128, 128, 128)   // 10: Khối đá (Xám trung tính)
};
}

TetrisEngine::TetrisEngine() {
    init();
}

uint32_t my_rand() {
    static uint32_t seed = 0;

    // Chỉ khởi tạo seed 1 lần duy nhất bằng TickCount của RTOS
    if (seed == 0) {
        seed = osKernelGetTickCount();
        if (seed == 0) seed = 123456789; // Đề phòng TickCount trả về 0 lúc khởi động
    }

    // Thuật toán LCG chuẩn
    seed = seed * 1664525 + 1013904223;
    return seed;
}

/**
 * @brief	Khởi tạo giá trị ban đầu cho các thuộc tính, tạo khối mới
 */
void TetrisEngine::init() {
    for (auto &row : grid) {
        row.fill(0);
    }
    gameOver = false;
    score = 0;
    nextBlockId = -1;
    nextBlockColor = 0;

    generateNextBlock();
    spawnBlock();
}

/**
 * @brief	Tạo ID, Color và Matrix cho khối tiếp theo
 */
void TetrisEngine::generateNextBlock() {
    int r = my_rand() % 100;

    if (r < 85) {
        // 85% cơ hội ra gạch bình thường (0 đến 6)
        nextBlockId = my_rand() % 7;
        nextBlockColor = nextBlockId;
    } else {
        // 15% cơ hội rớt ra đạo cụ (7, 8, 9, hoặc 10)
        // Mình nới %4 thay vì %3 để có thể ra cả Khối đá (10)
        nextBlockId = 7 + (my_rand() % 4);
        nextBlockColor = nextBlockId;
    }

    // Cập nhật Size chuẩn để xoay tâm
    if (nextBlockId >= 7) {
        nextBlockSize = 1;
    } else if (nextBlockId == 0) { // Khối I
        nextBlockSize = 4;
    } else if (nextBlockId == 1) { // Khối O
        nextBlockSize = 2;
    } else {
        nextBlockSize = 3;
    }

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            nextBlock[i][j] = Tetrominoes[nextBlockId][i][j];
        }
    }
}

/**
 * @brief	Get next block
 */
void TetrisEngine::getNextBlock(BlockMatrix &block, int &size, uint16_t &color, int &id) const {
    block = nextBlock;
    size = nextBlockSize;
    color = ColorPallette[nextBlockColor];
    id = nextBlockId;
}

/**
 * @brief	Đẩy NextBlock thành CurrBlock và đưa lên đỉnh màn hình
 */
void TetrisEngine::spawnBlock() {
    if (nextBlockId == -1) {
        generateNextBlock();
    }

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            currBlock[i][j] = nextBlock[i][j];
        }
    }

    blockSize = nextBlockSize;
    currBlockColor = nextBlockColor;

    // Xuất hiện ở giữa cạnh trên
    currX = (GRID_WIDTH - blockSize) / 2;
    currY = 0;

    generateNextBlock();
}

/**
 * @brief	Thuật toán xoay ma trận 90 độ
 */
void TetrisEngine::rotateMatrix(BlockMatrix &mat) {
    BlockMatrix temp = {0}; // Đảm bảo clear rác
    for (int i = 0; i < blockSize; ++i) {
        for (int j = 0; j < blockSize; ++j) {
            temp[j][blockSize - 1 - i] = mat[i][j];
        }
    }
    mat = temp;
}

/**
 * @brief	Lấy giới hạn hình học của khối
 */
void TetrisEngine::getBlockBounds(const BlockMatrix &block, int &minX, int &maxX, int &minY, int &maxY) {
	minX = 4; // Khởi tạo bằng kích thước tối đa
	    maxX = 0;
	    minY = 4;
	    maxY = 0;

	    // ÉP BUỘC QUÉT TOÀN BỘ MA TRẬN 4x4 (Không dùng blockSize nữa)
	    for (int i = 0; i < 4; ++i) {
	        for (int j = 0; j < 4; ++j) {
	            if (block[i][j]) {
	                if (j < minX) minX = j;
	                if (j > maxX) maxX = j;
	                if (i < minY) minY = i;
	                if (i > maxY) maxY = i;
	            }
	        }
	    }
}

/**
 * @brief	Kiểm tra va chạm
 */
bool TetrisEngine::checkCollision(int newX, int newY, const BlockMatrix &block) {
    int minX, maxX, minY, maxY;
    getBlockBounds(block, minX, maxX, minY, maxY);

    for (int i = minY; i <= maxY; ++i) {
        for (int j = minX; j <= maxX; ++j) {
            if (block[i][j]) {
                int gx = newX + j;
                int gy = newY + i;

                if (gx < 0 || gx >= GRID_WIDTH || gy < 0 || gy >= GRID_HEIGHT)
                    return true;
                if (grid[gy][gx] != 0) // Nếu ô đã có gạch
                    return true;
            }
        }
    }
    return false;
}

/**
 * @brief	Khóa khối hoặc thực thi logic Đạo cụ đặc biệt
 */
void TetrisEngine::lockBlock() {
    // 1. Kiểm tra xem có phải là đạo cụ đặc biệt không (Bom, Clear Col, Clear Row)
    if (currBlockColor >= 7 && currBlockColor <= 9) {
        int itemX = currX;
        int itemY = currY;

        // Thực thi hiệu ứng
        if (currBlockColor == 7) { // 7: Bom nổ 3x3
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = itemX + dx;
                    int ny = itemY + dy;
                    if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                        grid[ny][nx] = 0; // Clear ô
                    }
                }
            }

        }
        else if (currBlockColor == 8) { // 8: Clear cột
            for (int y = 0; y < GRID_HEIGHT; y++) {
                grid[y][itemX] = 0;
            }

        }
        else if (currBlockColor == 9) { // 9: Clear hàng
            for (int x = 0; x < GRID_WIDTH; x++) {
                grid[itemY][x] = 0;
            }

        }
        takeScore = true; // Kích hoạt còi báo hiệu ăn điểm
        score += point;
    }
    else {
        // 2. Nếu là khối bình thường hoặc Khối Đá (10) thì cố định vào Grid
        for (int i = 0; i < blockSize; ++i) {
            for (int j = 0; j < blockSize; ++j) {
                if (currBlock[i][j]) {
                    int gx = currX + j;
                    int gy = currY + i;
                    if (gy >= 0 && gy < GRID_HEIGHT && gx >= 0 && gx < GRID_WIDTH) {
                        grid[gy][gx] = currBlockColor + 1; // +1 để khác 0
                    }
                }
            }
        }
    }

    // --- CHU TRÌNH RỤNG VÀ QUÉT HÀNG CHUẨN ---
            bool actionHappened;
            do {
                actionHappened = false;

                // 1. Luôn ưu tiên kéo các mảng lơ lửng rụng xuống trước
                if (applyGravity()) {
                    actionHappened = true;
                }

                // 2. Quét xem sau khi rơi có tạo thành hàng ngang nào đầy không
                int oldScore = score;
                clearLines();
                if (score > oldScore) {
                    actionHappened = true;
                }

            } while (actionHappened); // Tiếp tục nếu vẫn còn rơi hoặc vẫn còn hàng ăn được

            spawnBlock(); // Sinh khối mới
}

/**
 * @brief Cơ chế Sticky Gravity (Trọng lực kết dính)
 * @retval bool: Trả về true nếu có bất kỳ khối nào bị kéo rụng xuống
 */
bool TetrisEngine::applyGravity() {
    bool everMoved = false;
    bool moved;

    do {
        moved = false;

        // Bước 1: Thuật toán Loang tìm các ô vững chắc (Stable)
        bool stable[GRID_HEIGHT][GRID_WIDTH] = {false};
        bool stableChanged = true;

        while (stableChanged) {
            stableChanged = false;
            for (int y = 0; y < GRID_HEIGHT; ++y) {
                for (int x = 0; x < GRID_WIDTH; ++x) {
                    if (grid[y][x] != 0 && !stable[y][x]) {
                        bool isStable = false;

                        if (y == GRID_HEIGHT - 1) isStable = true;
                        else if (grid[y + 1][x] != 0 && stable[y + 1][x]) isStable = true;
                        else if (x > 0 && grid[y][x - 1] != 0 && stable[y][x - 1]) isStable = true;
                        else if (x < GRID_WIDTH - 1 && grid[y][x + 1] != 0 && stable[y][x + 1]) isStable = true;
                        else if (y > 0 && grid[y - 1][x] != 0 && stable[y - 1][x]) isStable = true;

                        if (isStable) {
                            stable[y][x] = true;
                            stableChanged = true;
                        }
                    }
                }
            }
        }

        // Bước 2: Kéo TẤT CẢ các cụm gạch không vững chắc rơi xuống 1 ô
        for (int y = GRID_HEIGHT - 2; y >= 0; --y) {
            for (int x = 0; x < GRID_WIDTH; ++x) {
                if (grid[y][x] != 0 && !stable[y][x]) {
                    grid[y + 1][x] = grid[y][x]; // Kéo gạch xuống
                    grid[y][x] = 0;              // Xóa ô cũ
                    moved = true;
                    everMoved = true;            // Ghi nhận là có xảy ra sự rơi
                }
            }
        }
    } while (moved); // Lặp liên tục cho đến khi tất cả chạm đáy

    return everMoved;
}

/**
 * @brief	Quét và xóa hàng khi đầy
 */
void TetrisEngine::clearLines() {
    for (int y = GRID_HEIGHT - 1; y >= 0; --y) {
        bool full = true;
        bool hasStone = false; // Biến cờ kiểm tra xem hàng có khối đá không

        for (int x = 0; x < GRID_WIDTH; ++x) {
            if (grid[y][x] == 0) {
                full = false;
                break; // Có ô trống -> Chưa đầy -> Thoát sớm
            }
            if (grid[y][x] == 11) {
                // 11 chính là ID của Khối đá (10 + 1)
                hasStone = true;
            }
        }

        // CHỈ xóa hàng khi ĐẦY và KHÔNG CÓ KHỐI ĐÁ
        if (full && !hasStone) {
            takeScore = true;
            score += point;

            // Dịch các hàng phía trên xuống
            for (int row = y; row > 0; --row) {
                grid[row] = grid[row - 1];
            }
            grid[0].fill(0); // Làm trống hàng trên cùng

            ++y; // Re-check hàng hiện tại vì vừa bị dịch xuống
        }
    }
}
/**
 * @brief	Cập nhật game loop (rơi tự do)
 */
void TetrisEngine::update() {
    if (gameOver) return;

    if (!checkCollision(currX, currY + 1, currBlock)) {
        currY++;
    } else {
        lockBlock();
        // Check thua: nếu hàng trên cùng bị chiếm
        for (int i = 0; i < GRID_WIDTH; i++) {
            if (grid[0][i]) {
                gameOver = true;
                break;
            }
        }
    }
}

void TetrisEngine::moveLeft() {
    if (!gameOver && !checkCollision(currX - 1, currY, currBlock)) {
        currX--;
    }
}

void TetrisEngine::moveRight() {
    if (!gameOver && !checkCollision(currX + 1, currY, currBlock)) {
        currX++;
    }
}

void TetrisEngine::drop() {
    if (gameOver) return;

    while (!checkCollision(currX, currY + 1, currBlock)) {
        currY++;
    }
    lockBlock();
}

void TetrisEngine::rotate() {
    if (gameOver) return;

    BlockMatrix temp = currBlock;
    rotateMatrix(temp);

    if (!checkCollision(currX, currY, temp)) {
        currBlock = temp;
    }
}

uint16_t TetrisEngine::getCurrentBlockColor() const {
    return ColorPallette[currBlockColor];
}

uint16_t TetrisEngine::getGridColor(int x, int y) const {
    if (grid[y][x] == 0) return 0x0000; // Nền đen
    return ColorPallette[grid[y][x] - 1];
}
void TetrisEngine::selectLevel(char res)
{
	if(res == 'e') point = 1;
	else if(res == 'm') point = 2;
	else if(res == 'h') point = 3;
	else point = -1;
}
