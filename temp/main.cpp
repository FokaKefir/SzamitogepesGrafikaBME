#include <iostream>
#include <vector>


using namespace std;

#define SPLIT_INTEGRAL 1000

struct DataPoint {
    float time;
    float value;
};

class HermiteInterpolation {
private:
    std::vector<DataPoint> data;
    float minTime, maxTime;
    float minVal, maxVal;

public:
    HermiteInterpolation(std::vector<DataPoint> data, float maxVal, float minVal) {
        this->data = data;
        this->minVal = minVal;
        this->maxVal = maxVal;
        this->minTime = data.front().time;
        this->maxTime = data.back().time;
    }

    float interpolate(float time) {
        if (time <= minTime)
            return data.front().value;
        else if (time >= maxTime)
            return data.back().value;

        float result = 0.0;
        for (int i = 0; i < data.size(); i++) {
            float term = data[i].value;
            for (int j = 0; j < data.size(); j++) {
                if (i != j) {
                    term *= (time - data[j].time) / (data[i].time - data[j].time);
                }
            }
            result += term;
        }

        if (result > maxVal)
            result = maxVal;
        else if (result < minVal)
            result = minVal;

        return result;
    }

    float getMinTime() const {
        return this->minTime;
    }

    float getMaxTime() const {
        return this->maxTime;
    }

};

float calcComponent(HermiteInterpolation sInterpol, HermiteInterpolation rgbInterpol) {
    float sum = 0;

    float minTime = sInterpol.getMinTime();
    float maxTime = sInterpol.getMaxTime();

    float step = (maxTime - minTime) / SPLIT_INTEGRAL;

    for (int i = 0; i < SPLIT_INTEGRAL; ++i) {
        float t1 = minTime + (float) i * step;

        float val1 = sInterpol.interpolate(t1);
        float val2 = rgbInterpol.interpolate(t1);

        sum += (val1 * val2);
    }

    return sum / SPLIT_INTEGRAL;
}

int main() {
    std::vector<DataPoint> spectrumData = {
            {150.0, 0.0},
            {450.0, 1.0},
            {1600.0, 0.1}
    };
    HermiteInterpolation spectrumInterpolation(spectrumData, 1.0, 0.0);

    std::vector<DataPoint> redDetectorData = {
            {400.0, 0.0},
            {500.0, -0.2},
            {600.0, 2.5},
            {700.0, 0.0}
    };
    HermiteInterpolation redDetectorInterpolation(redDetectorData, 2.5, -0.2);

    std::vector<DataPoint> greenDetectorData = {
            {400.0, 0.0},
            {450.0, -0.1},
            {550.0, 1.2},
            {700.0, 0.0}
    };
    HermiteInterpolation greenDetectorInterpolation(greenDetectorData, 1.2, -0.1);

    std::vector<DataPoint> blueDetectorData = {
            {400.0, 0.0},
            {460.0, 1.0},
            {520.0, 0.0}
    };
    HermiteInterpolation blueDetectorInterpolation(blueDetectorData, 1.0, 0.0);

    float redComponent = calcComponent(spectrumInterpolation, redDetectorInterpolation);
    float greenComponent = calcComponent(spectrumInterpolation, greenDetectorInterpolation);
    float blueComponent = calcComponent(spectrumInterpolation, blueDetectorInterpolation);

    printf("redComponent: %f\n", redComponent);
    printf("greenComponent: %f\n", greenComponent);
    printf("blueComponent: %f\n", blueComponent);

    cout << redComponent / greenComponent;


    return 0;
}
