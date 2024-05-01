#pragma once

#include <QMainWindow>
#include <QSlider>
#include <QSpinBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QBoxLayout>

#include "canvas.h"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    Canvas *m_canvas;
    MainWindow();
    ~MainWindow();

private:
    void setupCanvas2D();


    void addHeading(QBoxLayout *layout, QString text);
    void addLabel(QBoxLayout *layout, QString text);
    void onClearButtonClick();
    void onUploadButtonClick();
    void onSaveButtonClick();
    void onMLBGButtonClick();
    void onColorButtonClick();
    void setUIntVal(std::uint8_t &setValue, int newValue);
    void setIntVal(int &setValue, int newValue);
    void setFloatVal(float &setValue, float newValue);
    void setBoolVal(bool &setValue, bool newValue);
    void setupCanvas();
    void addPushButton(QBoxLayout *layout, QString text, auto function);

    void onStippleButtonClick();
    void onDrawVoronoiButtonClick();
    void onFillButtonClick();
};
