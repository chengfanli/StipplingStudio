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

#include <QImage>
#include <QDir>

void saveProcessedImages(const QList<QImage>& processedImages) {
    // Create the output directory if it doesn't exist
    QDir().mkpath("./out_frame");

    // Save processed images
    for (int i = 0; i < processedImages.size(); ++i) {
        QString fileName = QString("./out_frame/frame_%1.jpg").arg(i, 4, 10, QChar('0'));
        if (!processedImages[i].save(fileName)) {
            qDebug() << "Failed to save image:" << fileName;
        }
    }
}

QImage WLBG::process(QImage input, Canvas *m_canvas, WLBG *m_wlbg, bool isVoronoi)
{
    // init

    m_image = input;
    m_density = m_image.scaledToWidth(
                           settings.supersampling_factor * m_image.width(),
                           Qt::SmoothTransformation
                           ).convertToFormat(QImage::Format_Grayscale8);
    m_size = m_image.size();

    auto stipples = stippling(m_canvas, m_wlbg, false);

    std::cout << "Start Painting" << std::endl;
    //    QSize imageSize(1200, 1000); // Set your desired image size
    // QString filePath = settings.output_path; // Set your desired file path

    QImage image(m_size,  QImage::Format_RGBX8888);
    image.fill(Qt::white); // Fill the background with white

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing); // Optional: for smoother edges
    painter.setPen(Qt::NoPen); // No border

    // Draw each stipple
    // #pragma omp parallel for
    for (const auto& stipple : stipples)
    {
        QPointF center(stipple.pos.x() * m_size.width(), stipple.pos.y() * m_size.height());
        qreal radius = stipple.size / 2.0; // Assuming size is the diameter
        painter.setBrush(QBrush(Qt::black));

        // Draw the stipple
        painter.drawEllipse(center, radius, radius);
    }

    return image;

}

void WLBG::stippling_video(Canvas *m_canvas, WLBG *m_wlbg, bool isVoronoi)
{
    QList<QImage> processedImages;

    // Define the directory path
    QDir dir("./frame");

    // Filter JPEG files
    QStringList filters;
    filters << "*.jpg" << "*.jpeg";
    dir.setNameFilters(filters);

    // Iterate over the files in the directory
    QStringList fileList = dir.entryList();
    int i = 53;
    foreach (QString file, fileList) {
        // Load the image
        QImage image(dir.filePath(file));
        if (image.isNull()) {
            qDebug() << "Failed to load image:" << file;
            continue;
        }

        // Process the image (replace with your custom processing function)
        QImage processedImage = process(image, m_canvas, m_wlbg, false);

        // Add the processed image to the list
        // processedImages.append(processedImage);

        QString fileName = QString("./out_frame/frame_%1.jpg").arg(i, 4, 10, QChar('0'));
        processedImage.save(fileName);
        i++;
    }

    // std::cout << "Save" << std::endl;
    // saveProcessedImages(processedImages);
}
