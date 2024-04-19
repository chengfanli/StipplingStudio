#ifndef MLBG_H
#define MLBG_H

#include "src/wlbg.h"
#include <unordered_map>

class MLBG: public WLBG
{
public:
    MLBG();
    float density_sum = 1.f; // 1.0 when it's black-and-white
    std::vector<Stipple> stippling(Canvas *m_canvas, MLBG *m_mlbg, bool isVoronoi);
    float computeAveDensity(Cell cell, QColor color);
    std::unordered_map<int, int> computeMap(std::vector<Stipple> points, std::vector<Stipple> newPoints);
    std::vector<int> computeIdxMap(std::vector<int> indices, int size);
    std::vector<Cell> generate_voronoi_cells(std::vector<Stipple> points, std::vector<int> &indices, draw &d, bool inverse);
    std::vector<Cell> generate_voronoi_cells(std::vector<Stipple> points, draw &d, bool inverse){
        return WLBG::generate_voronoi_cells(points, d, inverse);
    }
    void split_cell(std::vector<Stipple>& stipples, Cell cell, float point_size, Stipple stipple);

    std::vector<Stipple> filling(std::vector<Stipple> foregroundStipples, Canvas *m_canvas, MLBG *m_mlbg);

};

#endif // MLBG_H
