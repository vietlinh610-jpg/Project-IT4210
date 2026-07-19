#ifndef SCREEN3VIEW_HPP
#define SCREEN3VIEW_HPP

#include <gui_generated/screen3_screen/Screen3ViewBase.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>
#include "TetrisEngine.hpp"

class Screen3View : public Screen3ViewBase
{
public:
    Screen3View();
    virtual ~Screen3View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void handleTickEvent();
    virtual void pauseBtn();

    void drawGrid();	//vẽ lưới chính và block đang rơi
    void drawPreview();	//vẽ pre block

protected:
    TetrisEngine engine;				//game engine
    BoxWithBorder colBoxes[20][10];		//lưới box chính hiển thị
    BoxWithBorder previewBoxes[4][4];	//next box
    int tickCount;						//biếm đếm
    bool musicGameOver;					//trạng thái music game over
    uint8_t level;
    bool gamePaused;
};

#endif // SCREEN3VIEW_HPP
