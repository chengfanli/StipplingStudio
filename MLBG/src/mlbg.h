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
    std::vector<Cell> generate_voronoi_cells_withDiffBGImage(std::vector<Stipple> points, draw &d, QImage newDensity);
    std::vector<Cell> accumulateCells_withDiffImage(const IndexMap& map, QImage density);
    void split_cell(std::vector<Stipple>& stipples, Cell cell, float point_size, Stipple stipple);
    void split_cell(std::vector<Stipple>& stipples, Cell cell, float point_size, QColor color, bool inverse) {
        return WLBG::split_cell(stipples, cell,  point_size,  color,  inverse);
    }

    std::vector<Stipple> filling(std::vector<Stipple> foregroundStipples, Canvas *m_canvas, MLBG *m_mlbg);

    //for color layers
    std::vector<Stipple> stippling_color(Canvas *m_canvas, MLBG *m_mlbg, bool isVoronoi);
    std::vector<Cell> generate_voronoi_cells_color(std::vector<Stipple> points, std::vector<int> &indices, int id);

    std::vector<Cell> accumulateCells_color(const IndexMap& map, int id);
    std::vector<Stipple> init_stipples_color(QColor color, int id);
    void init_config();


private:
    int color_size;
    std::vector<QImage> m_densities;
    std::vector<QColor> colors;
    std::map<std::pair<int, int>, float> sum_density_pixel;
};

#endif // MLBG_H
