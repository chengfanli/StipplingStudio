#ifndef DRAW_H
#define DRAW_H
#include <QImage>
#include <QPainter>
#include <QColor>
#include "src/jc_voronoi.h"

class draw
{
public:
    draw();
    void init(int width, int height);
    void drawEdge(jcv_point p0,  jcv_point p1, QColor color);
    // void drawPoints(std::vector<Stipple> &stipples, QColor color);
    void drawPoints(float posx, float posy, QColor color);
    void drawX(float posx, float posy, QColor color);
    void endPaint(int i);

private:
    QImage image;
    QPainter painter;
};

#endif // DRAW_H

/*
struct Stipple
{
    Eigen::Vector2f pos;
    float size;
    QColor color;
};

*/
