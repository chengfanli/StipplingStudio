#include "canvas.h"
#include <QPainter>
#include <QMessageBox>
#include <QFileDialog>
#include <cmath>
#include <iostream>
#include "settings.h"

/**
 * @brief Initializes new 1200x1200 canvas
 */
void Canvas::init() {
    setMouseTracking(true);
    m_width = 1200;
    m_height = 1200;
    clearCanvas();
}

/**
 * @brief Canvas::clearCanvas sets all canvas pixels to blank white
 */
void Canvas::clearCanvas() {
    m_data.assign(m_width * m_height, RGBA{255, 255, 255, 255});
    settings.image_path = "";
    QImage emptyImage; // Default constructor creates an empty image
    displayImage(emptyImage);
}


/**
 * @brief Stores the image specified from the input file in this class's
 * `std::vector<RGBA> m_image`.
 * Also saves the image width and height to canvas width and height respectively.
 * @param file: file path to an image
 * @return True if successfully loads image, False otherwise.
 */
bool Canvas::loadImageFromFile(const QString &file) {
    QImage myImage;
    if (!myImage.load(file)) {
        std::cout<<"Failed to load in image"<<std::endl;
        return false;
    }
    myImage = myImage.convertToFormat(QImage::Format_RGBX8888);
    m_width = myImage.width();
    m_height = myImage.height();
    QByteArray arr = QByteArray::fromRawData((const char*) myImage.bits(), myImage.sizeInBytes());

    m_data.clear();
    m_data.reserve(m_width * m_height);
    for (int i = 0; i < arr.size() / 4.f; i++){
        m_data.push_back(RGBA{(std::uint8_t) arr[4*i], (std::uint8_t) arr[4*i+1], (std::uint8_t) arr[4*i+2], (std::uint8_t) arr[4*i+3]});
    }
    QImage emptyImage;
    displayImage(emptyImage);
    return true;
}

/**
 * @brief Saves the current canvas image to the specified file path.
 * @param file: file path to save image to
 * @return True if successfully saves image, False otherwise.
 */
bool Canvas::saveImageToFile(const QString &file) {
    QImage myImage = QImage(m_width, m_height, QImage::Format_RGBX8888);
    for (int i = 0; i < m_data.size(); i++){
        myImage.setPixelColor(i % m_width, i / m_width, QColor(m_data[i].r, m_data[i].g, m_data[i].b, m_data[i].a));
    }
    if (!myImage.save(file)) {
        std::cout<<"Failed to save image"<<std::endl;
        return false;
    }
    return true;
}


/**
 * @brief Get Canvas2D's image data and display this to the GUI
 */
void Canvas::displayImage(const QImage &image) {
    QImage now;
    if (image.isNull())
    {
        QByteArray* img = new QByteArray(reinterpret_cast<const char*>(m_data.data()), 4*m_data.size());
        now = QImage((const uchar*)img->data(), m_width, m_height, QImage::Format_RGBX8888);
        //    m_data.assign(m_width * m_height, RGBA{255, 255, 255, 255});
        setPixmap(QPixmap::fromImage(now));
        setFixedSize(m_width, m_height);
        update();
    }
    else
    {
//        now = image;
        //    m_data.assign(m_width * m_height, RGBA{255, 255, 255, 255});
        setPixmap(QPixmap::fromImage(image));
        setFixedSize(image.width(), image.height());
        update();
    }

}

/**
 * @brief Canvas2D::resize resizes canvas to new width and height
 * @param w
 * @param h
 */
void Canvas::resize(int w, int h) {
    m_width = w;
    m_height = h;
    m_data.resize(w * h);
    QImage emptyImage;
    displayImage(emptyImage);
}


std::uint8_t rgbaToGray(const RGBA &pixel) {
    double luma = 0.299 * pixel.r + 0.587 * pixel.g + 0.114 * pixel.b;
    return (uint8_t)luma;
}




/**
 * @brief Called when any of the parameters in the UI are modified.
 */
void Canvas::settingsChanged() {
    // this saves your UI settings locally to load next time you run the program
    settings.saveSettings();
}


