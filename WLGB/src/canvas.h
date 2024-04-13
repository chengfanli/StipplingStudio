#ifndef CANVAS_H
#define CANVAS_H

#include <QLabel>
#include <QMouseEvent>
#include <array>
#include "rgba.h"

class Canvas : public QLabel {
    Q_OBJECT
public:
    int m_width = 0;
    int m_height = 0;

    bool m_isDown = false;

    void init();
    void clearCanvas();
    bool loadImageFromFile(const QString &file);
    bool saveImageToFile(const QString &file);
    Q_INVOKABLE void displayImage(QImage image);
    void resize(int w, int h);

    // This will be called when the settings have changed
    void settingsChanged();

//    void filterImage();

    std::vector<RGBA> &getCanvasData() {return m_data;}


private:
    std::vector<RGBA> m_data;
};

#endif // CANVAS_H
