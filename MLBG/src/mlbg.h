#ifndef MLBG_H
#define MLBG_H

#include "src/wlbg.h"

class MLBG: public WLBG
{
public:
    MLBG();
    float density_sum = 1.f; // 1.0 when it's blacl-and-white
    std::vector<Stipple> stippling(Canvas *m_canvas, MLBG *m_mlbg, bool isVoronoi);
    float computeAveDensity(Cell cell, QColor color);
    std::unordered_map<int, int> computeMap(std::vector<Stipple> points, std::vector<Stipple> newPoints);

};

#endif // MLBG_H
