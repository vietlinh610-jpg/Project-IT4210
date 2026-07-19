/*
 * TetrisEngine.cpp
 *
 *  Created on: Jun 3, 2025
 *      Author: admin
 */

#include "TetrisEngine.hpp"
#include "cmsis_os.h"

namespace {
    const int Tetrominoes[7][4][4] = {
        // I
        {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
        // O
        {{1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},
        // T
        {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        // S
        {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},
        // Z
        {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        // J
        {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        // L
        {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
    };

    // Thêm màu cho khối
    const uint16_t ColorPallette[7] = {
		0xF800, // Red
		0x07E0, // Green
		0x001F, // Blue
		0xFC00,
		0xFFE0, // Yellow
		0xF81F, // Magenta
		0xFFFF, // White
    };
}

TetrisEngine::TetrisEngine() {
	init();
}

uint32_t my_rand() {
    // Tham số chuẩn của LCG: a = 1664525, c = 1013904223, m = 2^32
	static uint32_t seed = 123456789;
    seed = seed * 1664525 + osKernelGetTickCount();

    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;

    return seed;
}

/**
 * @brief	Khởi tạo giá trị ban đầu cho các thuộc tính, tạo khối mới
 * @param	None
 * @retval	None
 */
void TetrisEngine::init() {
	//Khởi tạo giá trị ban đầu cho grid
    for (auto& row : grid) row.fill(0);
    gameOver = false;
    score = 0
    		;

	// Dùng cho tạo khối tiếp theo
	nextBlockId = -1;
	nextBlockColor = 0;
	generateNextBlock();

    //random khối mới
    spawnBlock();
}

/**
 * @brief	Tạo khối mới và color cho khối
 * @param	None
 * @retval	None
 */
void TetrisEngine::generateNextBlock() {
    nextBlockId = my_rand() % 7;	//lấy next box dựa trên tick hệ thống
    nextBlockSize = (nextBlockId == 0) ? 4 : 3;
    nextBlockSize = (nextBlockId == 1) ? 2 : nextBlockSize;
    nextBlockColor = my_rand() % 7;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            nextBlock[i][j] = Tetrominoes[nextBlockId][i][j]; //đánh dấu các ô có thể hiển thị cho next block
}

/**
 * @brief	Get next block (gán nextBlock, size nextBlock và color cho tham số truyền vào)
 * @param	block: BlockMatrix& khối cần thay đổi
 * @param	size: int& biểu diễn kích thước của khối
 * @param	color: uint16_t& biểu diễn màu của khối
 * @retval	None
 */
void TetrisEngine::getNextBlock(BlockMatrix& block, int& size, uint16_t& color) const {
	block = nextBlock;
	size = nextBlockSize;
	color = ColorPallette[nextBlockColor];
}

/**
 * @brief	Gán khối mới cho khối hiện tại
 * @param	None
 * @retval	None
 */
void TetrisEngine::spawnBlock() {
    if (nextBlockId == -1) generateNextBlock(); // Spawn đầu
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            currBlock[i][j] = nextBlock[i][j];	//gán nextBlock cho currBlock
    blockSize = nextBlockSize;
    currBlockColor = nextBlockColor;

    //bắt đầu rơi tại vị trí giữa trên cùng
    currX = (GRID_WIDTH - blockSize) / 2;
    currY = 0;
    generateNextBlock(); // Tạo khối tiếp theo
}

/**
 * @brief	Xoay block
 * @param	None
 * @retval	None
 */
void TetrisEngine::rotateMatrix(BlockMatrix& mat) {
    BlockMatrix temp = {};
    for (int i = 0; i < blockSize; ++i)
        for (int j = 0; j < blockSize; ++j)
            temp[j][blockSize - 1 - i] = mat[i][j];
    mat = temp;
}

/**
 * @brief	Lấy đường biên của block (hình chữ nhật nhỏ nhất chứa được toàn bộ block)
 * @param	block: BlockMatrix& khối cần lấy đường biên
 * @param	minX: int& lưu tọa độ x nhỏ nhất
 * @param	maxX: int& lưu tọa độ x lớn nhất
 * @param	minY: int& lưu tọa độ y nhỏ nhất
 * @param	maxY: int& lưu tọa độ y lớn nhất
 * @retval	None
 */
void TetrisEngine::getBlockBounds(const BlockMatrix& block, int& minX, int& maxX, int& minY, int& maxY) {
    minX = blockSize; maxX = 0; minY = blockSize; maxY = 0;
    for (int i = 0; i < blockSize; ++i)
        for (int j = 0; j < blockSize; ++j)
            if (block[i][j]) {
                if (j < minX) minX = j;
                if (j > maxX) maxX = j;
                if (i < minY) minY = i;
                if (i > maxY) maxY = i;
            }
}

/**
 * @brief	Kiểm tra va trạm của khối với tọa độ (x, y) mới
 * @param	newX: int mô tả tọa độ x cần kiểm tra
 * @param	newY: int mô tả tọa độ y cần kiểm tra
 * @param	block: BlockMatrix& khối cần kiểm tra
 * @retval	boolean True - va chạm, False - không va chạm
 */
bool TetrisEngine::checkCollision(int newX, int newY, const BlockMatrix& block) {
    int minX, maxX, minY, maxY;

    //lấy bao ngoài của block
    getBlockBounds(block, minX, maxX, minY, maxY);
    for (int i = minY; i <= maxY; ++i)
        for (int j = minX; j <= maxX; ++j)
            if (block[i][j]) {
                int gx = newX + j;
                int gy = newY + i;

                //ra ngoài hoặc ô đã được đặt -> va chạm -> return true
                if (gx < 0 || gx >= GRID_WIDTH || gy < 0 || gy >= GRID_HEIGHT) return true;
                if (grid[gy][gx]) return true;
            }
    return false;
}

/**
 * @brief	Cố định lại block trên lưới và tạo khối mới
 * @param	None
 * @retval	None
 */
void TetrisEngine::lockBlock() {
    for (int i = 0; i < blockSize; ++i)
        for (int j = 0; j < blockSize; ++j)
            if (currBlock[i][j]) {
                int gx = currX + j;
                int gy = currY + i;
                if (gy >= 0 && gy < GRID_HEIGHT && gx >= 0 && gx < GRID_WIDTH)
                    grid[gy][gx] = currBlockColor + 1;
            }

    //xóa đường nếu full + gen khối mới
    clearLines();
    spawnBlock();
}

/**
 * @brief	Xóa line nếu cả hàng đã full
 * @param	None
 * @retval	None
 */
void TetrisEngine::clearLines() {
	//kiểm tra các hàng từ dưới lên trên
    for (int y = GRID_HEIGHT - 1; y >= 0; --y) {
        bool full = true;
        for (int x = 0; x < GRID_WIDTH; ++x)
            if (!grid[y][x]) full = false; //-> có 1 ô chưa được đánh dấu -> chưa đầy hàng

        if (full) {
        	takeScore = true; //đánh dấu được tăng điểm -> bật buzzer sau đó
        	score++; //tăng điểm
            for (int row = y; row > 0; --row)
                grid[row] = grid[row - 1];
            grid[0].fill(0);
            ++y; // re-check this row
        }
    }
}

/**
 * @brief	Check va chạm + khóa khối nếu được
 * @param	None
 * @retval	None
 */
void TetrisEngine::update() {
	if(!gameOver){
		if (!checkCollision(currX, currY + 1, currBlock))
			currY++;
		else{
			lockBlock();
			for(int i = 0; i < GRID_WIDTH; i++)
				if(grid[0][i]) gameOver = true; //hàng trên cùng có khối -> game over
		}
	}

}

/**
 * @brief	Di chuyển trái
 * @param	None
 * @retval	None
 */
void TetrisEngine::moveLeft() {
	if(gameOver) return;
	//kiểm tra trước khi di chuyển
    if (!checkCollision(currX - 1, currY, currBlock)) currX--;
}

/**
 * @brief	Di chuyển phải
 * @param	None
 * @retval	None
 */
void TetrisEngine::moveRight() {
	if(gameOver) return;
	//kiểm tra trước khi di chuyển
    if (!checkCollision(currX + 1, currY, currBlock)) currX++;
}

/**
 * @brief	Thả block
 * @param	None
 * @retval	None
 */
void TetrisEngine::drop() {
	if(gameOver) return;
	//kiểm tra trước khi di chuyển
    while (!checkCollision(currX, currY + 1, currBlock)) currY++;
    lockBlock();
}

/**
 * @brief	Xoay block
 * @param	None
 * @retval	None
 */
void TetrisEngine::rotate() {
	if(gameOver) return;
    BlockMatrix temp = currBlock;
    rotateMatrix(temp);
    //kiểm tra trước khi di chuyển
    if (!checkCollision(currX, currY, temp))
        currBlock = temp;
}

/**
 * @brief	Lấy màu của khối đang rơi
 * @param	None
 * @retval	uint16_t màu của khối
 */
uint16_t TetrisEngine::getCurrentBlockColor() const {
	return ColorPallette[currBlockColor];
}

/**
 * @brief	Lấy màu của lưới
 * @param	x: int biểu diễn tọa độ cần lấy màu
 * @param	y: int biểu diễn tọa độ cần lấy màu
 * @retval	uint16_t màu của ô
 */
uint16_t TetrisEngine::getGridColor(int x, int y) const {
	if(grid[y][x] == 0) return 0x0000;
	return ColorPallette[grid[y][x] - 1];
}
