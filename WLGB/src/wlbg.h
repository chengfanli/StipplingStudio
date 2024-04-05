#pragma once

#include "graphics/shape.h"
#include "Eigen/StdList"
#include "Eigen/StdVector"
#include "Eigen/Sparse"

#include <set>
#include <omp.h>
#include <QImage>
#include <QVector>

struct Stipple
{
    Eigen::Vector2f pos;
    float size;
    QColor color;
};

struct Voronoi
{
};

struct Cell
{
    Eigen::Vector2f centroid;
    float orientation;
    float area;
    float total_density;
};

class WLBG
{
public:
    QImage m_image;
    QImage m_density;
    Voronoi m_voronoi;
public:
    WLBG();
    std::vector<Stipple> stippling();
    void paint(std::vector<Stipple> points);

    // stipples
    std::vector<Stipple> init_stipples();
    float current_stipple_size(Cell cell);

    // voronoi
    std::vector<Cell> generate_voronoi_cells(std::vector<Stipple> points);
    QImage generate_voronoi_diagram();
    void split_cell(std::vector<Stipple>& stipples, Cell cell, float point_size);

    // utils
    int find_nearest_point(const std::vector<Stipple>& points, float pos_x, float pos_y);
    float calculate_lower_density_bound(float point_size, float hysteresis);
    float calculate_upper_density_bound(float point_size, float hysteresis);
    Eigen::Vector2f jitter(Eigen::Vector2f s);
};
