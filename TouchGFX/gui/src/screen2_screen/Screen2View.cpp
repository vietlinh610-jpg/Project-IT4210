#include <gui/screen2_screen/Screen2View.hpp>
#include "cmsis_os.h"
#include <touchgfx/Unicode.hpp>

extern osMessageQueueId_t levelQueueHandle;
Screen2View::Screen2View()
{

}

void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();
}

void Screen2View::tearDownScreen()
{
    Screen2ViewBase::tearDownScreen();
}

void Screen2View::easyBtn(){
	char res = 'a';
	while(osMessageQueueGetCount(levelQueueHandle) > 0){
		osMessageQueueGet(levelQueueHandle, &res, NULL, 100);
	}

	res = 'e';
	osMessageQueuePut(levelQueueHandle, &res, 0, 10);
}
void Screen2View::mediumBtn(){
	char res = 'a';
	while(osMessageQueueGetCount(levelQueueHandle) > 0){
		osMessageQueueGet(levelQueueHandle, &res, NULL, 100);
	}

	res = 'm';
	osMessageQueuePut(levelQueueHandle, &res, 0, 10);
}
void Screen2View::hardBtn(){
	char res = 'a';
	while(osMessageQueueGetCount(levelQueueHandle) > 0){
		osMessageQueueGet(levelQueueHandle, &res, NULL, 100);
	}

	res = 'h';
	osMessageQueuePut(levelQueueHandle, &res, 0, 10);
}

void Screen2View::handleTickEvent()
{
    Unicode::snprintf(textArea2Buffer, TEXTAREA2_SIZE, "%d", presenter->getHighestScore());
    textArea2.invalidate();
}
