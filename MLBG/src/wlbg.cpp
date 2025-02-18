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
    std::vector<Stipple> stipples = init_stipples(Qt::black, false);

    // stippling iteration
    int num_split = 0;
    int num_merge = 0;
    for (int i = 0; i < settings.max_iteration; i++)
    {
        draw d;
//        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Iteration: " << i << std::endl;

        // cells
        std::vector<Cell> voronoi_cells = generate_voronoi_cells(stipples, d, false);
        std::cout << "Current number of points: " << stipples.size() << std::endl;

        // current hysteresis
        float hysteresis = settings.hysteresis + i * settings.hysteresis_delta;

        stipples.clear();
        for (const auto &cell : voronoi_cells)
        {
            float point_size = current_stipple_size(cell);

            if (cell.total_density < calculate_lower_density_bound(point_size, hysteresis) || cell.area == 0.0f) {// merge
                num_merge++;
                d.drawPoints(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::black);
                d.drawX(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::red);
            }
            else if (cell.total_density < calculate_upper_density_bound(point_size, hysteresis)) {// keep
                stipples.push_back({cell.centroid, point_size, Qt::black});
                d.drawPoints(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::black);
            }
            else // split
            {
                num_split++;
                split_cell(stipples, cell, point_size, Qt::red, false);
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
            m_wlbg->paint(m_canvas, stipples, i);
        }


        // Handle other events, allowing GUI updates
        QCoreApplication::processEvents();
    }

    return stipples;
}

void WLBG::paint(Canvas *m_canvas, std::vector<Stipple> points, int iteration)
{
    std::cout << "Start Painting" << std::endl;
//    QSize imageSize(1200, 1000); // Set your desired image size
    QString filePath = settings.output_path; // Set your desired file path

    QImage image(m_size,  QImage::Format_RGBX8888);
    // image.fill(QColor(255, 192, 203)); // Fill the background with white
    image.fill(QColor(255, 255, 255)); // Fill the background with white

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
        painter.setBrush(QBrush(stipple.color));

        // Draw the stipple
        painter.drawEllipse(center, radius, radius);
    }

//    w.resize(1200, 1200);

    m_canvas->displayImage(image); // Update the canvas display

    // Save the image
    image.save(filePath);
}

QImage WLBG::paintBG(Canvas *m_canvas, std::vector<Stipple> points, int iteration)
{
    std::cout << "Start Painting" << std::endl;
    //    QSize imageSize(1200, 1000); // Set your desired image size
    QString filePath = settings.output_path; // Set your desired file path

    QImage image(m_size,  QImage::Format_RGBX8888);
    image.fill(Qt::black); // Fill the background

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
        painter.setBrush(QBrush(stipple.color));

        // Draw the stipple
        painter.drawEllipse(center, radius, radius);
    }

    //    w.resize(1200, 1200);

    m_canvas->displayImage(image); // Update the canvas display

    // Save the image
//    image.save(filePath);
    return image;
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
