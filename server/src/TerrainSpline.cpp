#include "WorldGeneration/TerrainSpline.hpp"
#include <algorithm>

TerrainSpline::TerrainSpline() {
}

TerrainSpline::TerrainSpline(std::vector<SplinePoint> points) {
    splinePoint = points;
}

void TerrainSpline::addPoint(SplinePoint point) {
    splinePoint.push_back(point);
}

float TerrainSpline::evaluateHeight(float noiseValue) {

    if (splinePoint.empty()) {
        return 0.0f;
    }
    if (noiseValue <= splinePoint.front().noiseValue) {
        return splinePoint.front().heightValue;
    }
    if (noiseValue >= splinePoint.back().noiseValue) {
        return splinePoint.back().heightValue;
    }

    /* probably slower for 5 points
    auto idx = std::upper_bound(splinePoint.begin(), splinePoint.end(), SplinePoint{noiseValue, 0.0f}) - splinePoint.begin() - 1;
    if (idx < 0) {         
        return 0.0f;     
    }     
    float t = (noiseValue - splinePoint[idx].noiseValue) / (splinePoint[idx + 1].noiseValue - splinePoint[idx].noiseValue);     
    return splinePoint[idx].heightValue + t * (splinePoint[idx + 1].heightValue - splinePoint[idx].heightValue);
    */

    for (int i = 0; i < splinePoint.size()-1; i++) {
        if (noiseValue >= splinePoint[i].noiseValue && noiseValue <= splinePoint[i+1].noiseValue) {
            float t = (noiseValue - splinePoint[i].noiseValue) / (splinePoint[i+1].noiseValue - splinePoint[i].noiseValue);
            return splinePoint[i].heightValue + t * (splinePoint[i+1].heightValue - splinePoint[i].heightValue);
        }
    }
    return 0.0f;
    
}
