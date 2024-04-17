#include "draw.h"
#include <iostream>

draw::draw() {}

void draw::init(int width, int height) {
    image = QImage(width, height, QImage::Format_ARGB32);
    image.fill(Qt::white);
    bool success = painter.begin(&image);
    if (!success) {
        std::cerr<<"Cant make an image of voronoi"<<std::endl;
    }
}

void draw::drawEdge(jcv_point p0,  jcv_point p1, QColor color) {
    painter.setPen(QPen(color, 2));
    QPoint start(p0.x, p0.y);
    QPoint end(p1.x, p1.y);
    painter.drawLine(start, end);

}

void draw::drawPoints(float posx, float posy, QColor color) {
    painter.setPen(QPen(color, 3));
    painter.drawPoint(posx, posy);
}

void draw::drawX(float posx, float posy, QColor color) {
    float elipson = 2.f;
    painter.setPen(QPen(color, 1));
    painter.drawLine(posx - elipson, posy - elipson, posx + elipson, posy + elipson);
    painter.drawLine(posx - elipson, posy + elipson, posx + elipson, posy - elipson);
}

void draw::endPaint(int i, Canvas *m_canvas) {
    painter.end();
    QString filename = QString("voronoi-images/veronoi%1.png").arg(i);
    image.save(filename);
    m_canvas->displayImage(image);
}
