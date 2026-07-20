#include <gui/screen3_screen/Screen3View.hpp>
#include <touchgfx/Color.hpp>
#include "cmsis_os.h"
#include "main.h"

extern osMessageQueueId_t movingQueueHandle;
extern osMessageQueueId_t levelQueueHandle;
extern uint8_t currScreen;
extern "C" {
extern volatile uint8_t triggerVibrate;
}
/**
 * @brief	TouchGFX yêu cầu RGB888 để xử lý đúng màu (TetrisEngine dùng RGB565 để tiết kiệm bộ nhớ)
 * @param	rgb565: uint16_t màu cần chuyển sang RGB888
 * @param	r: uint8_t& biểu diễn màu đỏ
 * @param	g: uint8_t& biểu diễn màu xanh lá cây
 * @param	b: uint8_t& biểu diễn màu xanh nước biển
 * @retval	None
 */
static void convertRGB565ToRGB888(uint16_t rgb565, uint8_t &r, uint8_t &g,
		uint8_t &b) {
	r = ((rgb565 >> 11) & 0x1F) << 3; // 5-bit red => 8-bit
	g = ((rgb565 >> 5) & 0x3F) << 2;  // 6-bit green => 8-bit
	b = (rgb565 & 0x1F) << 3;         // 5-bit blue => 8-bit
}

Screen3View::Screen3View() {
	int gridOffsetX = 29;   // tọa độ X góc trên-trái của khung lưới
	int gridOffsetY = 46;   // tọa độ Y góc trên-trái của khung lưới
	int cellSize = 12; // kích thước mỗi ô (px), giảm từ 16 xuống để khung nhỏ lại
	gamePaused = false;
	for (int y = 0; y < GRID_HEIGHT; y++) {
		for (int x = 0; x < GRID_WIDTH; x++) {
			int px = gridOffsetX + x * cellSize;
			int py = gridOffsetY + y * cellSize;
			colBoxes[y][x].setBorderColor(Color::getColorFromRGB(0, 0, 0));
			colBoxes[y][x].setBorderSize(1);
			colBoxes[y][x].setPosition(px, py, cellSize, cellSize);
			colBoxes[y][x].setVisible(true);
			add(colBoxes[y][x]);
		}
	}

	for (int y = 0; y < GRID_HEIGHT; y++) {
		for (int x = 0; x < GRID_WIDTH; x++) {
			int px = gridOffsetX + x * cellSize;
			int py = gridOffsetY + y * cellSize;

			// Xếp vị trí trùng khớp với ô lưới
			itemIcons[y][x].setPosition(px, py, cellSize, cellSize);
			itemIcons[y][x].setVisible(false); // Ẩn đi chờ khi nào có đạo cụ mới bật
			add(itemIcons[y][x]); // Lệnh add này gọi sau colBoxes nên ảnh sẽ nằm bên trên
		}
	}

	//khởi tạo lưới để hiện thị next block
	int previewX = 177;
	int previewY = 60;
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			int px = previewX + x * 8;
			int py = previewY + y * 8;
			previewBoxes[y][x].setAlpha(0);
			previewBoxes[y][x].setBorderSize(1);
			previewBoxes[y][x].setPosition(px, py, 8, 8);
			previewBoxes[y][x].setVisible(true);
			add(previewBoxes[y][x]);
		}
	}

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			int px = previewX + x * 8;
			int py = previewY + y * 8;
			previewItemIcons[y][x].setPosition(px, py, 8, 8);
			previewItemIcons[y][x].setVisible(false);
			add(previewItemIcons[y][x]);
		}
	}

	currScreen = 3;
	image2.setVisible(false);

	char res = 'a';
	//loại bỏ hết các phần tử trong moving queue
	osMessageQueueReset(movingQueueHandle);

	level = 20;
	while (osMessageQueueGet(levelQueueHandle, &res, NULL, 0) == osOK) {
		// Không cần code bên trong, biến res tự động được cập nhật liên tục
	}

	const char *levelName = "Easy";

	if (res == 'e') {
		level = 20;
		levelName = "Easy";
	} else if (res == 'm') {
		level = 15;
		levelName = "Medium";
	} else if (res == 'h') {
		level = 10;
		levelName = "Hard";
	}
	engine.selectLevel(res);

	Unicode::strncpy(textArea3Buffer, levelName, TEXTAREA3_SIZE);
	textArea3.invalidate();
}

void Screen3View::setupScreen() {
	Screen3ViewBase::setupScreen();
	engine.init();
	tickCount = 0;
	gamePaused = false;
	image2.setVisible(false);
	image2.invalidate();
}

void Screen3View::tearDownScreen() {
	Screen3ViewBase::tearDownScreen();
}

/**
 * @brief	Xử lý game sau mỗi tick event (tương đương vòng loop để xử lý game)
 * @retval	None
 */
void Screen3View::handleTickEvent() {
	if (gamePaused) {
		return;
	}
	if (++tickCount % level == 0) {

		if (engine.isGameOver()) {	//kiểm tra gameover
			remove(image2);
			add(image2);
			image2.setVisible(true);
			image2.invalidate();
			presenter->setHighestScore(engine.getScore());//cập nhật highest score
			return;
		}

		//kiểm tra queue -> có phần tử -> có tín hiệu nút bấm điều khiển
		if (osMessageQueueGetCount(movingQueueHandle) > 0) {
			char res;
			osMessageQueueGet(movingQueueHandle, &res, NULL, 10);
			if (res == 'L')
				engine.moveLeft();
			else if (res == 'R')
				engine.moveRight();
			else if (res == 'T')
				engine.rotate();
			else if (res == 'D')
				engine.drop();

		}
		//update hiển thị
		engine.update();
		Unicode::snprintf(scoreBuffer, SCORE_SIZE, "%d", engine.getScore());
		score.invalidate();
		presenter->setHighestScore(engine.getScore());

		if (engine.getTakeScore()) {
			triggerVibrate = 1; // Bật cờ ra lệnh rung
			engine.setTakeScore(false);
		}
		if (engine.isGameOver()) {	// game over -> ghi điểm vào flash
			triggerVibrate = 1;	// Bật cờ ra lệnh rung
			uint32_t tmp = engine.getScore();
			if ((uint32_t) presenter->getHighestScore() <= tmp) {
				Flash_Write_Data(0x08104000, &tmp, 1);
			}
		}
		//vẽ lại lưới chính
		drawGrid();

		//vẽ lại lưới cho pre block
		drawPreview();
	}
}

/**
 * @brief	Vẽ lưới game
 * @retval	None
 */
void Screen3View::drawGrid() {
//    const auto& grid = engine.getGrid();
	const auto &block = engine.getCurrentBlock();
	int currX = engine.getCurrX();
	int currY = engine.getCurrY();

	// --- VẼ LƯỚI CHÍNH ---
	for (int y = 0; y < GRID_HEIGHT; ++y) {
		for (int x = 0; x < GRID_WIDTH; ++x) {

			int cellValue = engine.getGrid()[y][x];
			itemIcons[y][x].setVisible(false); // Ẩn icon trước mỗi lần quét

			if (cellValue == 0) {
				// Ô trống
				colBoxes[y][x].setAlpha(0);
			} else if (cellValue >= 1 && cellValue <= 7) {
				// GẠCH THƯỜNG (Giá trị 1 đến 7)
				uint16_t gridColor = engine.getGridColor(x, y);
				uint8_t r, g, b;
				convertRGB565ToRGB888(gridColor, r, g, b);
				colBoxes[y][x].setColor(Color::getColorFromRGB(r, g, b));
				colBoxes[y][x].setAlpha(255);
			} else {
				// ĐẠO CỤ (Giá trị từ 8 trở đi - vì màu lưu ở engine là ID + 1)
				colBoxes[y][x].setAlpha(0); // Tắt màu nền
				itemIcons[y][x].setVisible(true); // Bật icon

				// Đổi BITMAP_..._ID bên dưới theo đúng tên file bạn đã nạp ở Bước 2
				if (cellValue == 8)
					itemIcons[y][x].setBitmap(
							touchgfx::Bitmap(BITMAP_ICON_BOMB_ID));
				else if (cellValue == 9)
					itemIcons[y][x].setBitmap(
							touchgfx::Bitmap(BITMAP_ICON_LIGHTNING_V_ID));
				else if (cellValue == 10)
					itemIcons[y][x].setBitmap(
							touchgfx::Bitmap(BITMAP_ICON_LIGHTNING_H_ID));
				else if (cellValue == 11)
					itemIcons[y][x].setBitmap(
							touchgfx::Bitmap(BITMAP_ICON_STONE_ID));
			}
			colBoxes[y][x].invalidate();
			itemIcons[y][x].invalidate();
		}
	}

	// Vẽ block đang rơi
	int currType = engine.getCurrentBlockType();  // Lấy ID loại gạch (0 đến 10)
	uint16_t blockColor = engine.getCurrentBlockColor(); // Lấy mã màu thực tế (0xF800...)
	uint8_t r, g, b;
	convertRGB565ToRGB888(blockColor, r, g, b); // Truyền đúng mã màu vào hàm convert

	//lấy bound của block (hình chữ nhật nhỏ nhất mà chứa được toàn bộ block)
	int minX, maxX, minY, maxY;
	engine.getBlockBounds(block, minX, maxX, minY, maxY);
	for (int i = minY; i <= maxY; ++i) {
		for (int j = minX; j <= maxX; ++j) {
			if (block[i][j]) {
				int gx = currX + j;
				int gy = currY + i;
				if (gx >= 0 && gx < GRID_WIDTH && gy >= 0 && gy < GRID_HEIGHT) {

					if (currType >= 0 && currType <= 6) {
						// Khối đang rơi là Gạch
						colBoxes[gy][gx].setColor(
								Color::getColorFromRGB(r, g, b));
						colBoxes[gy][gx].setAlpha(255);
					} else {
						// Khối đang rơi là Đạo cụ
						colBoxes[gy][gx].setAlpha(0);
						itemIcons[gy][gx].setVisible(true);

						if (currType == 7)
							itemIcons[gy][gx].setBitmap(
									touchgfx::Bitmap(BITMAP_ICON_BOMB_ID));
						else if (currType == 8)
							itemIcons[gy][gx].setBitmap(
									touchgfx::Bitmap(
											BITMAP_ICON_LIGHTNING_V_ID));
						else if (currType == 9)
							itemIcons[gy][gx].setBitmap(
									touchgfx::Bitmap(
											BITMAP_ICON_LIGHTNING_H_ID));
						else if (currType == 10)
							itemIcons[gy][gx].setBitmap(
									touchgfx::Bitmap(BITMAP_ICON_STONE_ID));
					}
					colBoxes[gy][gx].invalidate();
					itemIcons[gy][gx].invalidate();
				}
			}
		}
	}
}

/**
 * @brief	Vẽ pre block
 * @retval	None
 */
void Screen3View::drawPreview() {
	// Lấy khối tiếp theo
	TetrisEngine::BlockMatrix nextBlock;
	int nextBlockSize;
	uint16_t nextBlockColor;
	int nextBlockId;
	engine.getNextBlock(nextBlock, nextBlockSize, nextBlockColor, nextBlockId);

	// Đặt lại previewBox trước về trạng thái trong suốt
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			previewBoxes[i][j].setAlpha(0);
			previewItemIcons[i][j].setVisible(false); // Ẩn icon trước
			previewBoxes[i][j].invalidate();
			previewItemIcons[i][j].invalidate();
		}
	}

	// Vẽ preview block
	uint8_t r, g, b;
	convertRGB565ToRGB888(nextBlockColor, r, g, b);

	int minX, maxX, minY, maxY;
	engine.getBlockBounds(nextBlock, minX, maxX, minY, maxY);

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (nextBlock[i][j]) {
				int px = j - minX;
				int py = i - minY;
				if (px < 4 && py < 4) {
					if (nextBlockId < 7) {
						// Khối thường: vẽ Box màu
						previewBoxes[py][px].setColor(
								Color::getColorFromRGB(r, g, b));
						previewBoxes[py][px].setAlpha(255);
					} else {
						// Đạo cụ: vẽ Icon
						previewBoxes[py][px].setAlpha(0);
						previewItemIcons[py][px].setVisible(true);
						if (nextBlockId == 7)
							previewItemIcons[py][px].setBitmap(
									Bitmap(BITMAP_ICON_BOMB_ID));
						else if (nextBlockId == 8)
							previewItemIcons[py][px].setBitmap(
									Bitmap(BITMAP_ICON_LIGHTNING_V_ID));
						else if (nextBlockId == 9)
							previewItemIcons[py][px].setBitmap(
									Bitmap(BITMAP_ICON_LIGHTNING_H_ID));
						else if (nextBlockId == 10)
							previewItemIcons[py][px].setBitmap(
									Bitmap(BITMAP_ICON_STONE_ID));
					}
					previewBoxes[py][px].invalidate();
					previewItemIcons[py][px].invalidate();
				}
			}
		}
	}
}
void Screen3View::pauseBtn() {
	gamePaused = flexButton2.getPressed();
}
