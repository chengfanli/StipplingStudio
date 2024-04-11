#include "wlbg.h"
#include "settings.h"

#include <iostream>
#include <set>
#include <QPainter>
#include <vector>

using namespace Eigen;

WLBG::WLBG()
{
    m_image = QImage(QString::fromStdString(settings.image_path));
    m_density = m_image.scaledToWidth(
            settings.supersampling_factor * m_image.width(),
            Qt::SmoothTransformation
        ).convertToFormat(QImage::Format_Grayscale8);
}

std::vector<Stipple> WLBG::stippling()
{
    // init
    std::vector<Stipple> stipples = init_stipples();
    // if (settings.use_gpu) init_voronoi_context();

    // stippling iteration
    int num_split = 0;
    int num_merge = 0;

    for (int i = 0; i < settings.max_iteration; i++)
    {
        std::cout << "Iteration: " << i + 1 << std::endl;

        // cells
        std::vector<Cell> voronoi_cells = settings.use_gpu ? generate_voronoi_cells_gpu(stipples) : generate_voronoi_cells(stipples);
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
    }

    return stipples;
}

void WLBG::paint(std::vector<Stipple> points)
{
    std::cout << "Start Painting" << std::endl;

    QSize imageSize(1200, 1000); // Set your desired image size
    QString filePath = QString::fromStdString(settings.output_path); // Set your desired file path

    QImage image(imageSize, QImage::Format_ARGB32);
    image.fill(Qt::white); // Fill the background with white

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing); // Optional: for smoother edges

    // Draw each stipple
    for (const auto& stipple : points)
    {
        QPointF center(stipple.pos.x() * imageSize.width(), stipple.pos.y() * imageSize.height());
        qreal radius = stipple.size / 2.0; // Assuming size is the diameter

        // Set brush and pen for this stipple
        painter.setBrush(QBrush(stipple.color));
        painter.setPen(Qt::NoPen); // No border

        // Draw the stipple
        painter.drawEllipse(center, radius, radius);
    }

    // Save the image
    image.save(filePath);

    std::cout << "Finish Painting" << std::endl;
}

