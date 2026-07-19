#ifndef MODEL_HPP
#define MODEL_HPP

class ModelListener;

class Model
{
public:
    Model();
    long modelTickCount = 0;

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    int getHighestScore() const;
    void setHighestScore(int score);
protected:
    ModelListener* modelListener;
    int highestScore = 0;
};

#endif // MODEL_HPP
