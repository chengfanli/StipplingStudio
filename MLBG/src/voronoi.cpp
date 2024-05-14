#include "wlbg.h"

#include <iostream>
#include <vector>

#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"
#include "src/draw.h"

#if defined(_WIN32) || defined(_WIN64)
#include <omp.h>

// Check for Mac OS
#elif defined(__APPLE__) && defined(__MACH__)
#include "/opt/homebrew/Cellar/libomp/18.1.3/include/omp.h"

// Check for Linux
#elif defined(__linux__)
#include <omp.h>

#else
#error "Unknown operating system"
#endif

using namespace Eigen;

jcv_point* WLBG::construct_jcv(const std::vector<Stipple> &points, const jcv_point* min, const jcv_point* max, const jcv_point* scale) {
    if (points.empty()) return nullptr; // Safety check

    // Allocate memory for jcv_point array
    jcv_point* jcv_points = new jcv_point[points.size()];

    // Copy data from Stipple vector to jcv_point array in parallel
    #pragma omp parallel for
    for (size_t i = 0; i < points.size(); ++i) {
//        jcv_point p(points[i].pos[0] * scale->x, points[i].pos[1] * scale->y);
        jcv_point p;
        p.x = points[i].pos[0] * scale->x;
        p.y = points[i].pos[1] * scale->y;
        //jcv_point r = remap(&p, min, max, scale);
        jcv_points[i] = p;
    }

    return jcv_points;
}


std::vector<Cell> WLBG::generate_voronoi_cells(std::vector<Stipple> points, draw &d, bool inverse)
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
    jcv_point* temp = construct_jcv(points, &diagram.min, &dimensions, &dimensions);
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
            auto maxX = min2(max3((int)v0->x, (int)v1->x, (int)v2->x), m_density.width() - 1);
            auto maxY = min2(max3((int)v0->y, (int)v1->y, (int)v2->y), m_density.height() - 1);

            // Rasterize
            jcv_point p;
            for (int y = minY; y <= maxY; y++)
            {
                for (int x = minX; x <= maxX; x++)
                {
                    //                    p = jcv_point(x, y);
                    p.x = x;
                    p.y = y;

                    if (isInsideTriangle(p0, p1, s, p))
                        idxMap.set(p.x, p.y, (uint32_t)i);
                }
            }
            e = e->next;
        }
    }

    jcv_diagram_free( &diagram );

    std::cout << "Calculating Voronoi Cell" << std::endl;
    auto res = accumulateCells(idxMap, inverse);
    return res;
}

void WLBG::split_cell(std::vector<Stipple>& stipples, Cell cell, float point_size, QColor color, bool inverse)
{
    const float area = std::max(1.0f, cell.area);
    const float circleRadius = std::sqrt(area / M_PI);
    Vector2f splitVector = Vector2f(0.5f * circleRadius, 0.0f);

    const float a = cell.orientation;
    Vector2f splitVectorRotated = Vector2f(
        splitVector.x() * std::cos(a) - splitVector.y() * std::sin(a),
        splitVector.y() * std::cos(a) + splitVector.x() * std::sin(a));

    splitVectorRotated[0] = (splitVectorRotated[0] / m_density.width());
    splitVectorRotated[1] = (splitVectorRotated[1] / m_density.height());

    Vector2f splitSeed1 = cell.centroid - splitVectorRotated;
    Vector2f splitSeed2 = cell.centroid + splitVectorRotated;

    // check boundaries
    splitSeed1[0] = (std::max(0.0f, std::min(splitSeed1.x(), 1.0f)));
    splitSeed1[1] = (std::max(0.0f, std::min(splitSeed1.y(), 1.0f)));

    splitSeed2[0] = (std::max(0.0f, std::min(splitSeed2.x(), 1.0f)));
    splitSeed2[1] = (std::max(0.0f, std::min(splitSeed2.y(), 1.0f)));

    stipples.push_back({jitter(splitSeed1), point_size, color, inverse});
    stipples.push_back({jitter(splitSeed2), point_size, color, inverse});
}

std::vector<Cell> WLBG::accumulateCells(const IndexMap& map, bool inverse)
{
    // compute voronoi cell moments
    std::vector<Cell> cells = std::vector<Cell>(map.count());
    std::vector<Moments> moments = std::vector<Moments>(map.count());

    #pragma omp parallel for collapse(2)
    for (int x = 0; x < map.width; ++x) {
        for (int y = 0; y < map.height; ++y) {
            uint32_t index = map.get(x, y);

            QRgb densityPixel = m_density.pixel(x, y);
            float density = std::max(1.0f - qGray(densityPixel) / 255.0f * qAlpha(densityPixel) / 255.0f,
                                     std::numeric_limits<float>::epsilon());
            if(inverse) {
                density = 1.f - density;
            }
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

        //density
        cell.average_density = cell.total_density / cell.area;
        std::clamp(cell.average_density, 0.f, 1.f);
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


