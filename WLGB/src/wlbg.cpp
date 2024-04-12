#include "wlbg.h"
#include "QtWidgets/qboxlayout.h"
#include "settings.h"

#include <iostream>
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
}

std::vector<Stipple> WLBG::stippling(MainWindow &w, WLBG *m_wlbg)
{
    // init
    std::vector<Stipple> stipples = init_stipples();

    // stippling iteration
    int num_split = 0;
    int num_merge = 0;

    for (int i = 0; i < settings.max_iteration; i++)
    {
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

        m_wlbg->paint(w, stipples, i);
    }

    return stipples;
}

void WLBG::paint(MainWindow &w, std::vector<Stipple> points, int iteration)
{
    QSize imageSize(1200, 1000); // Set your desired image size
    QString filePath = settings.output_path; // Set your desired file path

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

    w.resize(1200, 1200);


//    QPixmap pixmap = QPixmap::fromImage(image);

//    // Create a label for the image
//    QLabel *imageLabel = new QLabel(&w);
//    imageLabel->setPixmap(pixmap);
//    imageLabel->setAlignment(Qt::AlignCenter);

//    // Create a label for the text
//    QLabel *textLabel = new QLabel(&w);
//    QString labelText = QString("Iteration %1").arg(iteration);
//    textLabel->setText(labelText);
//    textLabel->setAlignment(Qt::AlignCenter);

//    // Create a layout and add the labels
//    QVBoxLayout *layout = new QVBoxLayout();
//    layout->addWidget(textLabel);
//    layout->addWidget(imageLabel);

    // Check if the MainWindow already has a layout
    // If it does, remove it before setting a new one
//    if (w.layout()) {
//        delete w.layout(); // This will delete the old layout and its widgets
//    }


    // Set the container as the central widget of the window
//    w.setLayout(layout);

//    w.show();
}

