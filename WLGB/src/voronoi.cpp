#include "wlbg.h"
#include "settings.h"

#include <iostream>
#include <set>
#include <vector>
#include <random>
//#include <omp.h>
#include "/opt/homebrew/Cellar/libomp/18.1.3/include/omp.h"

using namespace Eigen;

std::vector<Cell> WLBG::generate_voronoi_cells(std::vector<Stipple> points)
{
    std::cout << "Build Voronoi Cells" << std::endl;

    std::vector<Cell> cells = std::vector<Cell>(points.size());
    std::vector<Moments> moments = std::vector<Moments>(points.size());

    #pragma omp parallel for collapse(2)
    for (int x = 0; x < m_density.width(); ++x)
    {
//        std::cout << x << " " << std::endl;
        for (int y = 0; y < m_density.height(); ++y)
        {
            QRgb densityPixel = m_density.pixel(x, y);
            float density = std::max(1.0f - qGray(densityPixel) / 255.0f,
                                     std::numeric_limits<float>::epsilon());

            int index = find_nearest_point(points, float(x) / float(m_density.width()), float(y) / float(m_density.height()));

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
    for (size_t i = 0; i < cells.size(); ++i)
    {
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

    std::cout << "Build Successfully" << std::endl;
    return cells;
}

void WLBG::split_cell(std::vector<Stipple>& stipples, Cell cell, float point_size)
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

    stipples.push_back({jitter(splitSeed1), point_size, Qt::red});
    stipples.push_back({jitter(splitSeed2), point_size, Qt::red});
}
