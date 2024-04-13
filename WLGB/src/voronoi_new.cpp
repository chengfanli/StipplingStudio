#include "wlbg.h"
#include "settings.h"

#include <iostream>
#include <set>
#include <vector>
#include <random>
#include <omp.h>
#include <QPainter>

#include <stdlib.h>

#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"

static inline jcv_point remap(const jcv_point* pt, const jcv_point* min, const jcv_point* max, const jcv_point* scale)
{
    jcv_point p;
    // p.x = (pt->x - min->x)/(max->x - min->x) * scale->x;
    // p.y = (pt->y - min->y)/(max->y - min->y) * scale->y;

    p.x = pt->x * scale->x;
    p.y = pt->y * scale->y;
    return p;
}

jcv_point* construct_jcv(const std::vector<Stipple> &points, const jcv_point* min, const jcv_point* max, const jcv_point* scale) {
    if (points.empty()) return nullptr; // Safety check

    // Allocate memory for jcv_point array
    jcv_point* jcv_points = new jcv_point[points.size()];

    // Copy data from Stipple vector to jcv_point array in parallel
    #pragma omp parallel for
    for (size_t i = 0; i < points.size(); ++i) {
        jcv_point p(points[i].pos[0] * scale->x, points[i].pos[1] * scale->y);
        //jcv_point r = remap(&p, min, max, scale);
        jcv_points[i] = p;
    }

    return jcv_points;
}
QColor encodeIndexToQColor(int index) {
    int r = (index / (256 * 256)) % 256;
    int g = (index / 256) % 256;
    int b = index % 256;

    return QColor(r, g, b);
}


QVector<QPointF> convertToQVector(const std::vector<jcv_point>& points, QSize imageSize) {
    QVector<QPointF> qPoints;
    qPoints.reserve(points.size()); // Pre-allocate memory for efficiency

    for (const auto& point : points) {
        qPoints.append(QPointF(point.x * imageSize.width(), point.y * imageSize.height()));
    }

    return qPoints;
}

int decodeQColorToIndex(const QColor &color) {
    int r = color.red();
    int g = color.green();
    int b = color.blue();

    return (r * 256 * 256) + (g * 256) + b;
}

QColor randomColor()
{
    int base = 120;
    static std::random_device rd; // Seed with a real random value, if available
    static std::mt19937 rng(rd()); // Use Mersenne Twister
    static std::uniform_int_distribution<> dist(-67, 67); // Range for variation

    // Calculate the color component values with variations
    int r = std::min(std::max(0, base + dist(rng)), 255);
    int g = std::min(std::max(0, base + dist(rng)), 255);
    int b = std::min(std::max(0, base + dist(rng)), 255);

    return QColor(r, g, b);
}

static inline int min2(int a, int b)
{
    return (a < b) ? a : b;
}

static inline int max2(int a, int b)
{
    return (a > b) ? a : b;
}

static inline int min3(int a, int b, int c)
{
    return min2(a, min2(b, c));
}
static inline int max3(int a, int b, int c)
{
    return max2(a, max2(b, c));
}

static inline int orient2d(const jcv_point* a, const jcv_point* b, const jcv_point* c)
{
    return ((int)b->x - (int)a->x)*((int)c->y - (int)a->y) - ((int)b->y - (int)a->y)*((int)c->x - (int)a->x);
}

std::vector<Cell> WLBG::accumulateCells(const IndexMap& map)
{
    // compute voronoi cell moments
    std::vector<Cell> cells = std::vector<Cell>(map.count());
    std::vector<Moments> moments = std::vector<Moments>(map.count());

    #pragma omp parallel for collapse(2)
    for (int x = 0; x < map.width; ++x) {
        for (int y = 0; y < map.height; ++y) {
            uint32_t index = map.get(x, y);

            QRgb densityPixel = m_density.pixel(x, y);
            float density = std::max(1.0f - qGray(densityPixel) / 255.0f,
                                     std::numeric_limits<float>::epsilon());

            #pragma omp critical
            {
                Cell& cell = cells[index];
                cell.area++;
                cell.total_density += density;

                Moments& m = moments[index];
                m.moment00 += density;
                m.moment10 += x * density;
                m.moment01 += y * density;
                m.moment11 += x * y * density;
                m.moment20 += x * x * density;
                m.moment02 += y * y * density;
            }
        }
    }

    // compute cell quantities
    #pragma omp parallel for
    for (size_t i = 0; i < cells.size(); ++i) {
        Cell& cell = cells[i];
        if (cell.total_density <= 0.0f) continue;

        auto [m00, m10, m01, m11, m20, m02] = moments[i];

        // centroid
        cell.centroid[0] = (m10 / m00);
        cell.centroid[1] = (m01 / m00);

        // orientation
        float x = m20 / m00 - cell.centroid.x() * cell.centroid.x();
        float y = 2.0f * (m11 / m00 - cell.centroid.x() * cell.centroid.y());
        float z = m02 / m00 - cell.centroid.y() * cell.centroid.y();
        cell.orientation = std::atan2(y, x - z) / 2.0f;

        cell.centroid[0] = ((cell.centroid.x() + 0.5f) / m_density.width());
        cell.centroid[1] = ((cell.centroid.y() + 0.5f) / m_density.height());
    }
    return cells;
}

int orientation(jcv_point p, jcv_point q, jcv_point r) {
    return (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
}

// Function to check if point p lies on the line segment 'pr'
bool onSegment(jcv_point p, jcv_point q, jcv_point r) {
    if (q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) &&
        q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y))
        return true;
    return false;
}

bool isInsideTriangle(jcv_point a, jcv_point b, jcv_point c, jcv_point p) {
    // Check the orientation of the ordered triples
    int o1 = orientation(a, b, p);
    int o2 = orientation(b, c, p);
    int o3 = orientation(c, a, p);

    // General case
    if (o1 == 0 && onSegment(a, p, b)) return true; // Check if p is on segment ab
    if (o2 == 0 && onSegment(b, p, c)) return true; // Check if p is on segment bc
    if (o3 == 0 && onSegment(c, p, a)) return true; // Check if p is on segment ca

    // Check for general position of p inside the triangle
    if ((o1 > 0 && o2 > 0 && o3 > 0) || (o1 < 0 && o2 < 0 && o3 < 0))
        return true;

    return false; // If none of the above cases, then p is not inside abc
}


std::vector<Cell> WLBG::generate_voronoi_cells_gpu(std::vector<Stipple> points)
{
    jcv_diagram diagram;
    auto count = points.size();
    jcv_clipper* clipper = 0;
    jcv_rect rect;

    jcv_point dimensions;
    dimensions.x = (jcv_real)m_density.width();
    dimensions.y = (jcv_real)m_density.height();

    jcv_point m;
    m.x = 0.0;
    m.y = 0.0;
    rect.min = m;
    rect.max = dimensions;

    memset(&diagram, 0, sizeof(jcv_diagram));
    jcv_point* temp = construct_jcv(points, &diagram.min, &dimensions, &dimensions );
    jcv_diagram_generate(count, temp, &rect, clipper, &diagram);

    std::cout << "Generating Voronoi Diagram" << std::endl;
    const jcv_site* sites = jcv_diagram_get_sites( &diagram );

    IndexMap idxMap(m_density.width(), m_density.height(), points.size());

    std::cout << "Painting Voronoi Diagram" << std::endl;
    for( int i = 0; i < diagram.numsites; ++i )
    {
        const jcv_site* site = &sites[i];
        jcv_point s = site->p;

        const jcv_graphedge* e = site->edges;
        while( e )
        {
            jcv_point p0 = e->pos[0];
            jcv_point p1 = e->pos[1];

            // Compute triangle bounding box
            jcv_point* v0 = &s;
            jcv_point* v1 = &p0;
            jcv_point* v2 = &p1;

            // Clip against screen bounds
            auto minX = max2(min3((int)v0->x, (int)v1->x, (int)v2->x), 0);
            auto minY = max2(min3((int)v0->y, (int)v1->y, (int)v2->y), 0);
            auto maxX = min2( max3((int)v0->x, (int)v1->x, (int)v2->x), m_density.width() - 1);
            auto maxY = min2(max3((int)v0->y, (int)v1->y, (int)v2->y), m_density.height() - 1);

            // Rasterize
            jcv_point p;
            for (int y = minY; y <= maxY; y++)
            {
                for (int x = minX; x <= maxX; x++)
                {
                    p = jcv_point(x, y);

                    if (isInsideTriangle(p0, p1, s, p))
                        idxMap.set(p.x, p.y, (uint32_t)i);
                }
            }
            e = e->next;
        }
    }

    jcv_diagram_free( &diagram );

    std::cout << "Calculating Voronoi Cell" << std::endl;
    auto res = accumulateCells(idxMap);
    return res;
}
