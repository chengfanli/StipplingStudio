#include "mainwindow.h"
#include "settings.h"
#include "wlbg.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <QGroupBox>
#include <QTabWidget>
#include <QScrollArea>
#include <QCheckBox>
#include <iostream>

MainWindow::MainWindow()
{
    setWindowTitle("Stippling");

    // loads in settings from last run or uses default values
//    settings.loadSettingsOrDefaults();

    QHBoxLayout *hLayout = new QHBoxLayout(); // horizontal layout for canvas and controls panel
    QVBoxLayout *vLayout = new QVBoxLayout(); // vertical layout for control panel

    vLayout->setAlignment(Qt::AlignTop);

    hLayout->addLayout(vLayout);
    setLayout(hLayout);

    setupCanvas();
    resize(1200, 1000);

//     makes the canvas into a scroll area
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidget(m_canvas);
    scrollArea->setWidgetResizable(true);
    hLayout->addWidget(scrollArea, 1);

    // groupings by project
    QWidget *group = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setAlignment(Qt::AlignTop);
    group->setLayout(layout);


    QScrollArea *controlsScroll = new QScrollArea();
    QTabWidget *tabs = new QTabWidget();
    controlsScroll->setWidget(tabs);
    controlsScroll->setWidgetResizable(true);

    tabs->addTab(group, "Settings");

    vLayout->addWidget(controlsScroll);

    // brush selection
//    addHeading(layout, "group");

    // clearing canvas
    addPushButton(layout, "Clear canvas", &MainWindow::onClearButtonClick);

    // filter push buttons
    addPushButton(layout, "Load Image", &MainWindow::onUploadButtonClick);
    addPushButton(layout, "Save Image", &MainWindow::onSaveButtonClick);
    addPushButton(layout, "Stippling", &MainWindow::onStippleButtonClick);
}


MainWindow::~MainWindow() {
    // Your cleanup code here, if any
}

/**
 * @brief Sets up Canvas2D
 */
void MainWindow::setupCanvas() {
    m_canvas = new Canvas();
    m_canvas->init();

    if (!settings.image_path.isEmpty()) {
        m_canvas->loadImageFromFile(settings.image_path);
    }
}


// ------ FUNCTIONS FOR ADDING UI COMPONENTS ------

void MainWindow::addHeading(QBoxLayout *layout, QString text) {
    QFont font;
    font.setPointSize(16);
    font.setBold(true);

    QLabel *label = new QLabel(text);
    label->setFont(font);
    layout->addWidget(label);
}

void MainWindow::addLabel(QBoxLayout *layout, QString text) {
    layout->addWidget(new QLabel(text));
}



void MainWindow::addPushButton(QBoxLayout *layout, QString text, auto function) {
    QPushButton *button = new QPushButton(text);
    layout->addWidget(button);
    connect(button, &QPushButton::clicked, this, function);
}




// ------ FUNCTIONS FOR UPDATING SETTINGS ------


void MainWindow::setUIntVal(std::uint8_t &setValue, int newValue) {
    setValue = newValue;
    m_canvas->settingsChanged();
}

void MainWindow::setIntVal(int &setValue, int newValue) {
    setValue = newValue;
    m_canvas->settingsChanged();
}

void MainWindow::setFloatVal(float &setValue, float newValue) {
    setValue = newValue;
    m_canvas->settingsChanged();
}

void MainWindow::setBoolVal(bool &setValue, bool newValue) {
    setValue = newValue;
    m_canvas->settingsChanged();
}


// ------ PUSH BUTTON FUNCTIONS ------

void MainWindow::onClearButtonClick() {
    m_canvas->resize(m_canvas->parentWidget()->size().width(), m_canvas->parentWidget()->size().height());
    m_canvas->clearCanvas();
}

void MainWindow::onStippleButtonClick() {
    // stippling
    WLBG m_wlbg = WLBG();
    std::vector<Stipple> stipples = m_wlbg.stippling(m_canvas, &m_wlbg);
    std::cout << "finish" << std::endl;
//    m_wlbg.paint(m_canvas, stipples, 5);
}

void MainWindow::onUploadButtonClick() {
    // Get new image path selected by user
    QString file = QFileDialog::getOpenFileName(this, tr("Open Image"), QDir::homePath(), tr("Image Files (*.png *.jpg *.jpeg)"));
    if (file.isEmpty()) { return; }
    settings.image_path = file;

    // Display new image
    m_canvas->loadImageFromFile(settings.image_path);

    m_canvas->settingsChanged();
}

void MainWindow::onSaveButtonClick() {
    // Get new image path selected by user
    QString file = QFileDialog::getSaveFileName(this, tr("Save Image"), QDir::currentPath(), tr("Image Files (*.png *.jpg *.jpeg)"));
    if (file.isEmpty()) { return; }

    // Save image
    m_canvas->saveImageToFile(file);
}

