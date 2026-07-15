#pragma once
#include <vector>

struct SplinePoint {
    float noiseValue, heightValue;
};

class TerrainSpline {

    private:

    std::vector<SplinePoint> splinePoint;

    public:
    
    TerrainSpline();
    TerrainSpline(std::vector<SplinePoint> points);
    void addPoint(SplinePoint point);
    float evaluateHeight(float noiseValue);

};