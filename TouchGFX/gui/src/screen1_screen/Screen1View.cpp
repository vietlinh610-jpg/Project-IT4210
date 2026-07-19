#include <gui/screen1_screen/Screen1View.hpp>
#include "main.h"

extern uint8_t currScreen;

Screen1View::Screen1View()
{
	currScreen = 1;
    DF_SendCommand(0x0F, 0x02, 0x02);
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::handleTickEvent()
{
	tickCounter += 1;
	//hiển thị điểm cao nhất đạt được
	Unicode::snprintf(highestScoreBuffer, HIGHESTSCORE_SIZE, "%d", presenter->getHighestScore());
	highestScore.invalidate();
}
