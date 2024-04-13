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

std::vector<Stipple> WLBG::stippling(Canvas *m_canvas, WLBG *m_wlbg)
{
    // init
    std::vector<Stipple> stipples = init_stipples();

    // stippling iteration
    int num_split = 0;
    int num_merge = 0;

    for (int i = 0; i < settings.max_iteration; i++)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Iteration: " << i << std::endl;

        // cells
        std::vector<Cell> voronoi_cells = generate_voronoi_cells(stipples);
        std::cout << "Current number of points: " << stipples.size() << std::endl;

        // current hysteresis
        float hysteresis = settings.hysteresis + i * settings.hysteresis_delta;

        stipples.clear();
        for (const auto &cell : voronoi_cells)
        {
            float point_size = current_stipple_size(cell);

            if (cell.total_density < calculate_lower_density_bound(point_size, hysteresis) || cell.area == 0.0f) // merge
                num_merge++;
            else if (cell.total_density < calculate_upper_density_bound(point_size, hysteresis)) // keep
                stipples.push_back({cell.centroid, point_size, Qt::black});
            else // split
            {
                num_split++;
                split_cell(stipples, cell, point_size);
            }
        }

        if (num_split == 0 && num_merge == 0)
            break;
        num_split = 0;
        num_merge = 0;

        m_wlbg->paint(m_canvas, stipples, i);
    }

    return stipples;
}

void WLBG::paint(Canvas *m_canvas, std::vector<Stipple> points, int iteration)
{
    std::cout << iteration << "！！ " << std::endl;
//    QSize imageSize(1200, 1000); // Set your desired image size
    QString filePath = settings.output_path; // Set your desired file path

    QImage image(m_size, QImage::Format_ARGB32);
    image.fill(Qt::white); // Fill the background with white

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing); // Optional: for smoother edges

    // Draw each stipple
    for (const auto& stipple : points)
    {
        QPointF center(stipple.pos.x() * m_size.width(), stipple.pos.y() * m_size.height());
        qreal radius = stipple.size / 2.0; // Assuming size is the diameter

        // Set brush and pen for this stipple
        painter.setBrush(QBrush(stipple.color));
        painter.setPen(Qt::NoPen); // No border

        // Draw the stipple
        painter.drawEllipse(center, radius, radius);
    }

    // Save the image
    image.save(filePath);

//    w.resize(1200, 1200);
    QByteArray arr = QByteArray::fromRawData((const char*) image.bits(), image.sizeInBytes());

    std::vector<RGBA> m_data;
    m_data.clear();
    m_data.reserve(image.width() * image.height());
    for (int i = 0; i < arr.size() / 4.f; i++){
        m_data.push_back(RGBA{(std::uint8_t) arr[4*i], (std::uint8_t) arr[4*i+1], (std::uint8_t) arr[4*i+2], (std::uint8_t) arr[4*i+3]});
    }

    m_canvas->displayImage(); // Update the canvas display
}

