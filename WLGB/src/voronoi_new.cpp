#include "wlbg.h"
#include "settings.h"

#include <iostream>
#include <set>
#include <vector>
#include <random>
#include <omp.h>
#include <QPainter>

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

void assign_pixel_to_tri(IndexMap& idxMap, const jcv_point* v0, const jcv_point* v1, const jcv_point* v2, int w, int h, int index, std::vector<Stipple> points)
{

    // // Compute triangle bounding box
    // int minX = min3((int)v0->x, (int)v1->x, (int)v2->x);
    // int minY = min3((int)v0->y, (int)v1->y, (int)v2->y);
    // int maxX = max3((int)v0->x, (int)v1->x, (int)v2->x);
    // int maxY = max3((int)v0->y, (int)v1->y, (int)v2->y);

    // // Clip against screen bounds
    // minX = max2(minX, 0);
    // minY = max2(minY, 0);
    // maxX = min2(maxX, w - 1);
    // maxY = min2(maxY, h - 1);

    // // Rasterize
    // jcv_point p;
    // for (p.y = (jcv_real)minY; p.y <= (jcv_real)maxY; p.y++) {
    //     for (p.x = (jcv_real)minX; p.x <= (jcv_real)maxX; p.x++) {
    //         // // Determine barycentric coordinates
    //         // int w0 = orient2d(v1, v2, &p);
    //         // int w1 = orient2d(v2, v0, &p);
    //         // int w2 = orient2d(v0, v1, &p);

    //         // // If p is on or inside all edges, render pixel.
    //         // if (w0 >= 0 && w1 >= 0 && w2 >= 0)
    //         // {
    //         //     // std::cout << "Assign" << std::endl;
    //         //     idxMap.set(p.x, p.y, (uint32_t)index);
    //         // }
    //         int index = find_nearest_point(points, float(p.x) / float(w), float(p.y) / float(h));
    //         idxMap.set(p.x, p.y, (uint32_t)index);
    //     }
    // }
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
    jcv_rect* rect;

    jcv_point dimensions;
    dimensions.x = (jcv_real)m_density.width();
    dimensions.y = (jcv_real)m_density.height();

    rect->min = jcv_point(0, 0);
    rect->max = dimensions;

    memset(&diagram, 0, sizeof(jcv_diagram));
    jcv_point* temp = construct_jcv(points, &diagram.min, &dimensions, &dimensions );
    jcv_diagram_generate(count, temp, rect, clipper, &diagram);

    // QImage image(m_density.width(), m_density.height(), QImage::Format_ARGB32_Premultiplied);
    // image.fill(Qt::white); // Fill background with white

    // QPainter painter(&image);
    // painter.setRenderHint(QPainter::Antialiasing, true);

    std::cout << "Generating Voronoi Diagram" << std::endl;
    const jcv_site* sites = jcv_diagram_get_sites( &diagram );

    IndexMap idxMap(m_density.width(), m_density.height(), points.size());

    // int t = 0;
    // for (int y = 0; y < m_density.height(); ++y) {
    //     for (int x = 0; x < m_density.width(); ++x) {
    //         idxMap.set(x, y, (uint32_t)t);
    //     }
    // }

    std::cout << "Painting Voronoi Diagram" << std::endl;
    for( int i = 0; i < diagram.numsites; ++i )
    {
        const jcv_site* site = &sites[i];

        jcv_point s = site->p;//remap(&site->p, &diagram.min, &diagram.max, &dimensions );

        auto c = randomColor();
        // auto c = encodeIndexToQColor(i);

        const jcv_graphedge* e = site->edges;
        while( e )
        {
            // jcv_point p0 = remap(&e->pos[0], &diagram.min, &diagram.max, &dimensions );
            // jcv_point p1 = remap(&e->pos[1], &diagram.min, &diagram.max, &dimensions );
            jcv_point p0 = e->pos[0];
            jcv_point p1 = e->pos[1];

            // assign_pixel_to_tri(idxMap, &p0, &p1, &s, m_density.width(), m_density.height(), i, points);
            // Compute triangle bounding box
            jcv_point* v0 = &s;
            jcv_point* v1 = &p0;
            jcv_point* v2 = &p1;

            int minX = min3((int)v0->x, (int)v1->x, (int)v2->x);
            int minY = min3((int)v0->y, (int)v1->y, (int)v2->y);
            int maxX = max3((int)v0->x, (int)v1->x, (int)v2->x);
            int maxY = max3((int)v0->y, (int)v1->y, (int)v2->y);

            // Clip against screen bounds
            minX = max2(minX, 0);
            minY = max2(minY, 0);
            maxX = min2(maxX, m_density.width() - 1);
            maxY = min2(maxY, m_density.height() - 1);

            // Rasterize
            jcv_point p;
            for (int y = minY; y <= maxY; y++) {
                for (int x = minX; x <= maxX; x++) {

            // for (int y = 0; y < m_density.height(); ++y) {
            //     for (int x = 0; x < m_density.width(); ++x) {

            p = jcv_point(x, y);

            // int w0 = orient2d(v1, v2, &p);
            // int w1 = orient2d(v2, v0, &p);
            // int w2 = orient2d(v0, v1, &p);

            // If p is on or inside all edges, render pixel.
            // if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || w0 <= 0 && w1 <= 0 && w2 <= 0){

                    if (isInsideTriangle(p0, p1, s, p))
                    {
                        // idxMap.set(p.x, p.y, (uint32_t)i);
                        // image.setPixelColor(p.x, p.y, c);
                        // int index = find_nearest_point(points, float(p.x) / float(m_density.width()), float(p.y) / float(m_density.height()));
                        idxMap.set(p.x, p.y, (uint32_t)i);
                        // image.setPixelColor(p.x, p.y, c);
                    }


                    // Determine barycentric coordinates
                    // int w0 = orient2d(v1, v2, &p);
                    // int w1 = orient2d(v2, v0, &p);
                    // int w2 = orient2d(v0, v1, &p);

                    // // If p is on or inside all edges, render pixel.
                    // if (w0 >= 0 && w1 >= 0 && w2 >= 0)
                    // {
                    //     // std::cout << "Assign" << std::endl;
                    //     idxMap.set(p.x, p.y, (uint32_t)i);
                    // }
                    // int index = find_nearest_point(points, float(p.x) / float(m_density.width()), float(p.y) / float(m_density.height()));
                    // idxMap.set(p.x, p.y, (uint32_t)index);
                }
            }

            // QPolygonF polygon(convertToQVector({s, p0, p1}, QSize{m_density.width(), m_density.height()}));
            // // painter.setBrush(encodeIndexToQColor(i));
            // painter.setBrush(c);
            // painter.setPen(c);
            // painter.drawPolygon(polygon);

            e = e->next;
        }
        // image.save("./output/voronoi.png");
    }



    // #pragma omp parallel for collapse(2)
    // for (int y = 0; y < m_density.height(); ++y) {
    //     for (int x = 0; x < m_density.width(); ++x) {
    //         if (idxMap.get(x, y) > points.size() || idxMap.get(x, y) < 0)
    //         {
    //             int index = find_nearest_point(points, float(x) / float(m_density.width()), float(y) / float(m_density.height()));
    //             idxMap.set(x, y, (uint32_t)index);
    //         }
    //     }
    // }

    // std::cout << "Generating Index Map" << std::endl;
    // IndexMap idxMap(m_density.width(), m_density.height(), points.size());

    // int bad_count = 0;

    // #pragma omp parallel for collapse(2)
    // for (int y = 0; y < m_density.height(); ++y) {
    //     for (int x = 0; x < m_density.width(); ++x) {
    //         QColor color = image.pixelColor(x, y);
    //         int index = decodeQColorToIndex(color);

    //         if(index > points.size())
    //         {
    //             index = find_nearest_point(points, float(x) / float(m_density.width()), float(y) / float(m_density.height()));
    //             bad_count++;
    //         }

    //         //int index = find_nearest_point(points, float(x) / float(m_density.width()), float(y) / float(m_density.height()));


    //         // Make sure 'set' operation on idxMap is thread-safe or does not cause race conditions
    //         idxMap.set(x, y, (uint32_t)index);
    //     }
    // }

    // std::cout << "Bad Point: " << bad_count << std::endl;

    std::cout << "Calculating Voronoi Cell" << std::endl;
    auto res = accumulateCells(idxMap);
    return res;
}
