#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

Screen1Presenter::Screen1Presenter(Screen1View& v)
    : view(v)
{

}

void Screen1Presenter::activate()
{

}

void Screen1Presenter::deactivate()
{

}

int Screen1Presenter::getHighestScore() const{
	return model->getHighestScore();
}
void Screen1Presenter::setHighestScore(int score){
	model->setHighestScore(score);
}
