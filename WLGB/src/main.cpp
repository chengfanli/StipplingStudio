#include "mainwindow.h"
#include "wlbg.h"
#include <cstdlib>
#include <ctime>

#include <QApplication>
#include <QSurfaceFormat>
#include <QScreen>

#include "settings.h"

#include <QCommandLineParser>
#include <QtCore>
#include <iostream>

int main(int argc, char *argv[])
{
    srand(static_cast<unsigned>(time(0)));

    // Create a Qt application
    QApplication a(argc, argv);
    QCoreApplication::setApplicationName("WLBG");
    QCoreApplication::setOrganizationName("CS 2240");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    // Set OpenGL version to 4.1 and context to Core
    QSurfaceFormat fmt;
    fmt.setVersion(4, 1);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(fmt);

    // Parse input
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("config",  "Path of the config (.ini) file.");
    parser.process(a);
    const QStringList args = parser.positionalArguments();
    if (args.size() < 1) {
        std::cerr << "Not enough arguments. Please provide a path to a config file (.ini) as a command-line argument." << std::endl;
        a.exit(1);
        return 1;
    }

    // settings
    QSettings input( args[0], QSettings::IniFormat);
    // IO
    settings.image_path = input.value("IO/image_path").toString().toStdString();

    // stippling
    WLBG m_wlbg = WLBG();
    m_wlbg.stipple();
    std::cout << "finish" << std::endl;

    // Create a GUI window
    MainWindow w;
    w.resize(600, 500);
    int desktopArea = QGuiApplication::primaryScreen()->size().width() *
                      QGuiApplication::primaryScreen()->size().height();
    int widgetArea = w.width() * w.height();
    if (((float)widgetArea / (float)desktopArea) < 0.75f)
        w.show();
    else
        w.showMaximized();

    return a.exec();
}
