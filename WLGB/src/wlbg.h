#pragma once

#include "graphics/shape.h"
#include "Eigen/StdList"
#include "Eigen/StdVector"
#include "Eigen/Sparse"

#include <set>
//#include <omp.h>
#include "/opt/homebrew/Cellar/libomp/18.1.3/include/omp.h"

#include "mainwindow.h"

#include <QImage>
#include <QVector>
#include <QScreen>
//#include <QMainWindow>

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

struct Stipple
{
    Eigen::Vector2f pos;
    float size;
    QColor color;
};

struct Cell
{
    Eigen::Vector2f centroid;
    float orientation;
    float area;
    float total_density;
};

class IndexMap;

class WLBG
{
public:
    QImage m_image;
    QImage m_density;
    QSize m_size;

    // voronoi gpu
    QOpenGLContext* m_context;
    QOffscreenSurface* m_surface;
    QOpenGLVertexArrayObject* m_vao;
    QOpenGLShaderProgram* m_shaderProgram;
    QOpenGLFramebufferObject* m_fbo;
    int m_coneVertices;

    QMainWindow m_window;

public:
    WLBG();
    std::vector<Stipple> stippling(Canvas *m_canvas, WLBG *wlbg);
    void paint(Canvas *m_canvas, std::vector<Stipple> points, int iteration);

    // stipples
    std::vector<Stipple> init_stipples();
    float current_stipple_size(Cell cell);

    // voronoi
    std::vector<Cell> generate_voronoi_cells(std::vector<Stipple> points);
    QImage generate_voronoi_diagram();
    void split_cell(std::vector<Stipple>& stipples, Cell cell, float point_size);

    // voronoi gpu
    void init_voronoi_context();
    IndexMap calculate(const QVector<QVector2D>& points);
    std::vector<Cell> generate_voronoi_cells_gpu(std::vector<Stipple> points);
    std::vector<Cell> accumulateCells(const IndexMap& map);

    // utils
    int find_nearest_point(const std::vector<Stipple>& points, float pos_x, float pos_y);
    float calculate_lower_density_bound(float point_size, float hysteresis);
    float calculate_upper_density_bound(float point_size, float hysteresis);
    Eigen::Vector2f jitter(Eigen::Vector2f s);


};

class IndexMap {
public:
    int32_t width;
    int32_t height;

    IndexMap(int32_t w, int32_t h, int32_t count);
    void set(const int32_t x, const int32_t y, const uint32_t value);
    uint32_t get(int32_t x, const int32_t y) const;
    int32_t count() const;

private:
    int32_t m_numEncoded;
    QVector<uint32_t> m_data;
};

struct Moments {
    float moment00;
    float moment10;
    float moment01;
    float moment11;
    float moment20;
    float moment02;
};
