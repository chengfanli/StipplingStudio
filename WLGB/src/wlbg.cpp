#include "wlbg.h"
#include "QtWidgets/qboxlayout.h"
#include "settings.h"
#include "mainwindow.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <set>
#include <QPainter>
#include <vector>
#include <QLabel>
#include <QCoreApplication>
#include <QThread>

#include "src/draw.h"

using namespace Eigen;

WLBG::WLBG()
{
    m_image = QImage(settings.image_path);
    m_density = m_image.scaledToWidth(
            settings.supersampling_factor * m_image.width(),
            Qt::SmoothTransformation
        ).convertToFormat(QImage::Format_Grayscale8);
    m_size = m_image.size();
}

std::vector<Stipple> WLBG::stippling(Canvas *m_canvas, WLBG *m_wlbg, bool isVoronoi)
{
    // init
    std::vector<Stipple> stipples = init_stipples();

    // stippling iteration
    int num_split = 0;
    int num_merge = 0;

    for (int i = 0; i < settings.max_iteration; i++)
    {
        draw d;
//        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Iteration: " << i << std::endl;

        // cells
        std::vector<CellEdge> cellEdges;
        std::vector<int> idx;
        std::vector<Cell> voronoi_cells = generate_voronoi_cells(stipples, d, cellEdges, idx);
        std::cout << "Current number of points: " << stipples.size() << std::endl;

        if(settings.point_animation && i == 6) {
            paint_point_voronoi(m_canvas, stipples, i, cellEdges);
        }
        // current hysteresis
        float hysteresis = settings.hysteresis + i * settings.hysteresis_delta;
<<<<<<< HEAD
        std::vector<Stipple> oldStipple = stipples;
=======

        float old_size;

>>>>>>> bcd2f5a72bdf16a829c96f6ff155124cb4537603
        stipples.clear();
        int cellid = 0;
        for (const auto &cell : voronoi_cells)
        {
            cellid = 0;
            float point_size = current_stipple_size(cell);

            if (cell.total_density < calculate_lower_density_bound(point_size, hysteresis) || cell.area == 0.0f) {// merge
                num_merge++;
                // d.drawPoints(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::black);
                // d.drawX(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::red);
                oldStipple[idx[cellid]].is_delete = true;
            }
            else if (cell.total_density < calculate_upper_density_bound(point_size, hysteresis)) {// keep
                stipples.push_back({cell.centroid, point_size, Qt::black, false, cell.centroid, 0.0f});
                d.drawPoints(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::black);
            }
            else // split
            {
                num_split++;
                split_cell(stipples, cell, point_size);
                d.drawPoints(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::black);
                auto last = stipples.back();
                auto secondLast = stipples[stipples.size() - 2];
                d.drawPoints(last.pos.x() * m_size.width(), last.pos.y() * m_size.height(), Qt::green);
                d.drawPoints(secondLast.pos.x() * m_size.width(), secondLast.pos.y() * m_size.height(), Qt::green);


            }
        }

        if (num_split == 0 && num_merge == 0)
            break;
        num_split = 0;
        num_merge = 0;

        if (isVoronoi == true) {
            d.endPaint(i, m_canvas);
        }
        else
        {
            if (settings.point_animation)
            {
                for (int f = 0; f < settings.animation_frame; f++)
                {
                    for (auto& s : stipples)
                    {
                        if (s.moving)
                        {
                            float oldRadius = std::sqrt(s.size / M_PI);
                            float newRadius = std::sqrt(s.aim_size / M_PI);
                            float radiusSpeed = (newRadius - oldRadius) / settings.animation_frame;

                            // std::cout << "R: " << oldRadius << " " << newRadius << " " << radiusSpeed << std::endl;

                            s.pos += (s.aim_pos - s.pos).normalized() * s.speed;
                            s.size = (oldRadius + radiusSpeed * f) * M_PI;
                            if ((s.aim_pos - s.pos).norm() <= s.speed)
                            {
                                s.moving = false;
                                s.pos = s.aim_pos;
                            }
                        }
                    }
                    m_wlbg->paint(m_canvas, stipples, i);
                    QCoreApplication::processEvents();
                }
            }
            else
                m_wlbg->paint(m_canvas, stipples, i);
        }


        // Handle other events, allowing GUI updates
        QCoreApplication::processEvents();
    }

    return stipples;
}

void WLBG::paint(Canvas *m_canvas, std::vector<Stipple> points, int iteration)
{
    // std::cout << "Start Painting" << std::endl;
//    QSize imageSize(1200, 1000); // Set your desired image size
    QString filePath = settings.output_path; // Set your desired file path

    QImage image(m_size,  QImage::Format_RGBX8888);
    image.fill(Qt::white); // Fill the background with white

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing); // Optional: for smoother edges
    painter.setPen(Qt::NoPen); // No border

    // Draw each stipple
    // #pragma omp parallel for
    for (const auto& stipple : points)
    {
        QPointF center(stipple.pos.x() * m_size.width(), stipple.pos.y() * m_size.height());
        qreal radius = stipple.size / 2.0; // Assuming size is the diameter

        // Set brush and pen for this stipple
        if (iteration == settings.max_iteration - 1)
            painter.setBrush(QBrush(Qt::black));
        else
            if(stipple.is_delete)
                painter.setBrush(QBrush(Qt::green));
            else painter.setBrush(QBrush(stipple.color));

        // Draw the stipple
        painter.drawEllipse(center, radius, radius);
        if(stipple.is_delete) {
            std::cout<<"is deleted!"<<std::endl;
            painter.setPen(QPen(Qt::green, 10));
            float size = 100;
            float centerX = stipple.pos.x() * m_size.width();
            float centerY = stipple.pos.y() * m_size.height();
            painter.drawLine(centerX - size, centerY - size, centerX + size, centerY + size);
            painter.drawLine(centerX + size, centerY - size, centerX - size, centerY + size);
        }
    }

//    w.resize(1200, 1200);

    m_canvas->displayImage(image); // Update the canvas display
//    emit m_canvas->imageUpdated(m_canvas, image);

    // Now schedule the displayImage call on the main thread
//    QMetaObject::invokeMethod(m_canvas, "displayImage", Qt::QueuedConnection,
//                              Q_ARG(QImage, image));


    // Save the image
    image.save(filePath);
}

void WLBG::paint_point_voronoi(Canvas *m_canvas, std::vector<Stipple> points, int iteration, std::vector<CellEdge> cellEdges)
{
    // std::cout << "Start Painting voronoi" << std::endl;

    assert(points.size() == cellEdges.size());
    for(int i = 0; i < points.size(); i++) {
        points[i].edges = cellEdges[i];
    }

    std::sort(points.begin(), points.end(), [](const Stipple& a, const Stipple& b) {
        if (a.pos.y() == b.pos.y())
            return a.pos.x() < b.pos.x(); // 若y坐标相同，则按x坐标从左到右排序
        return a.pos.y() < b.pos.y(); // 主要按y坐标从上到下排序
    });

    //    QSize imageSize(1200, 1000); // Set your desired image size
    QString filePath = settings.output_path; // Set your desired file path

    QImage image(m_size,  QImage::Format_RGBX8888);
    image.fill(Qt::white); // Fill the background with white

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing); // Optional: for smoother edges
    painter.setPen(Qt::NoPen); // No border

    // Draw each stipple
    // #pragma omp parallel for
    for (const auto& stipple : points)
    {
        QPointF center(stipple.pos.x() * m_size.width(), stipple.pos.y() * m_size.height());
        qreal radius = stipple.size / 2.0; // Assuming size is the diameter

        // Set brush and pen for this stipple
        if (iteration == settings.max_iteration - 1)
            painter.setBrush(QBrush(Qt::black));
        else
            painter.setBrush(QBrush(stipple.color));

        // Draw the stipple
        painter.drawEllipse(center, radius, radius);
    }

    //    w.resize(1200, 1200);

    m_canvas->displayImage(image); // Update the canvas display
    //    emit m_canvas->imageUpdated(m_canvas, image);

    // Now schedule the displayImage call on the main thread
    //    QMetaObject::invokeMethod(m_canvas, "displayImage", Qt::QueuedConnection,
    //                              Q_ARG(QImage, image));

    QCoreApplication::processEvents();
    QColor lightPink(255, 182, 193);

    for(int i = 0; i < points.size(); i++) {
        for(auto& edge: points[i].edges.edges) {
            QPoint point1(edge.first.x(), edge.first.y());
            QPoint point2(edge.second.x(), edge.second.y());
            painter.setPen(QPen(lightPink, 2));
            painter.drawLine(point1, point2);
        }
        m_canvas->displayImage(image); // Update the canvas display
        QCoreApplication::processEvents();
        QThread::msleep(200);

    }
    // Save the image
    image.save(filePath);
}


// Index Map
IndexMap::IndexMap(int32_t w, int32_t h, int32_t count)
    : width(w), height(h), m_numEncoded(count) {
    m_data = QVector<uint32_t>(w * h);
}

void IndexMap::set(const int32_t x, const int32_t y, const uint32_t value) {
    m_data[y * width + x] = value;
}

uint32_t IndexMap::get(const int32_t x, const int32_t y) const {
    return m_data[y * width + x];
}

int32_t IndexMap::count() const { return m_numEncoded; }
