#include "wlbg.h"
#include "settings.h"
#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"

#include <iostream>
#include <set>
#include <vector>
#include <random>
#include <omp.h>
#include <QPainter>

jcv_point* construct_jcv(const std::vector<Stipple> &points) {
    if (points.empty()) return nullptr; // Safety check

    // Allocate memory for jcv_point array
    jcv_point* jcv_points = new jcv_point[points.size()];

    // Copy data from Stipple vector to jcv_point array in parallel
    #pragma omp parallel for
    for (size_t i = 0; i < points.size(); ++i) {
        jcv_points[i].x = points[i].pos.x();
        jcv_points[i].y = points[i].pos.y();
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


std::vector<Cell> WLBG::generate_voronoi_cells_gpu(std::vector<Stipple> points)
{
    jcv_diagram diagram;
    auto count = points.size();
    jcv_clipper* clipper = 0;
    jcv_rect* rect = 0;

    memset(&diagram, 0, sizeof(jcv_diagram));
    jcv_point* temp = construct_jcv(points);
    jcv_diagram_generate(count, temp, rect, clipper, &diagram);

    QImage image(m_density.width(), m_density.height(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white); // Fill background with white

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);

    std::cout << "Generating Voronoi Diagram" << std::endl;
    const jcv_site* sites = jcv_diagram_get_sites( &diagram );

    std::cout << "Painting Voronoi Diagram" << std::endl;
    for( int i = 0; i < diagram.numsites; ++i )
    {
        const jcv_site* site = &sites[i];

        jcv_point s = site->p;

        // auto c = randomColor();
        auto c = encodeIndexToQColor(i);

        const jcv_graphedge* e = site->edges;
        while( e )
        {
            jcv_point p0 = e->pos[0];
            jcv_point p1 = e->pos[1];

            QPolygonF polygon(convertToQVector({s, p0, p1}, QSize{m_density.width(), m_density.height()}));
            // painter.setBrush(encodeIndexToQColor(i));
            painter.setBrush(c);
            painter.setPen(Qt::NoPen);
            painter.drawPolygon(polygon);

            e = e->next;
        }
    }

    image.save("./output/voronoi.png");

    std::cout << "Generating Index Map" << std::endl;
    IndexMap idxMap(m_density.width(), m_density.height(), points.size());

    int bad_count = 0;

    #pragma omp parallel for collapse(2)
    for (int y = 0; y < m_density.height(); ++y) {
        for (int x = 0; x < m_density.width(); ++x) {
            QColor color = image.pixelColor(x, y);
            int index = decodeQColorToIndex(color);

            if(index > points.size())
            {
                index = find_nearest_point(points, float(x) / float(m_density.width()), float(y) / float(m_density.height()));
                bad_count++;
            }


            // Make sure 'set' operation on idxMap is thread-safe or does not cause race conditions
            idxMap.set(x, y, (uint32_t)index);
        }
    }

    std::cout << "Bad Point: " << bad_count << std::endl;

    std::cout << "Calculating Voronoi Cell" << std::endl;
    auto res = accumulateCells(idxMap);
    return res;
}
