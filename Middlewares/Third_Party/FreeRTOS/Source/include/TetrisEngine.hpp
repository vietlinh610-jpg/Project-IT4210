/*
 * TetrisEngine.hpp
 *
 *  Created on: Jun 3, 2025
 *      Author: admin
 */

#ifndef APPLICATION_USER_GUI_TETRISENGINE_HPP_
#define APPLICATION_USER_GUI_TETRISENGINE_HPP_

#include <array>
#include <cstdint>
#include <cstdlib>

const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 20;

class TetrisEngine {
public:

	using Grid = std::array<std::array<int, GRID_WIDTH>, GRID_HEIGHT>;
	using BlockMatrix = std::array<std::array<int, 4>, 4>;

	TetrisEngine();

    void init();
    void update();
    void moveLeft();
    void moveRight();
    void moveDown();
    void rotate();
    void drop();

	void getBlockBounds(const BlockMatrix& block, int& minX, int& maxX, int& minY, int& maxY);
    const Grid& getGrid() const { return grid; }
    const BlockMatrix& getCurrentBlock() const { return currBlock; }
    int getBlockSize() const { return blockSize; }
    int getCurrX() const { return currX; }
    int getCurrY() const { return currY; }
    int getScore() const { return score; } 
    void setTakeScore(bool param) { takeScore = param; }
	bool getTakeScore() const { return takeScore; }
	void generateNextBlock();
	void getNextBlock(BlockMatrix& block, int& size, uint16_t& color) const;
	bool isGameOver() const { return gameOver; }
	uint16_t getCurrentBlockColor() const;
	uint16_t getGridColor(int x, int y) const;

private:
	Grid grid;					//~ int grid[20][10] -> đánh dấu các ô trên lưới
	BlockMatrix currBlock;		//~ int currBlock[4][4] -> đánh dấu các ô cho block hiện tại
	int currX, currY;			//vị trí của block
	int blockSize;				//size của block
	bool gameOver;				//trạng thái game
	int score;					//điểm
	bool takeScore;				//ghi điểm?
	int nextBlockSize;			//size khối tiếp theo
	int nextBlockId;			//id khối tiếp theo
	BlockMatrix nextBlock;		//khối tiếp theo
	int currBlockColor;
	int nextBlockColor;

	void spawnBlock();
	bool checkCollision(int newX, int newY, const BlockMatrix& block);
	void lockBlock();
	void clearLines();
	void rotateMatrix(BlockMatrix& mat);
};

#endif /* APPLICATION_USER_GUI_TETRISENGINE_HPP_ */
