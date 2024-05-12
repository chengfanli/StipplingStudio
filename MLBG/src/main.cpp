#include "mainwindow.h"
#include "wlbg.h"
#include <cstdlib>
#include <ctime>

#include <QApplication>
#include <QSurfaceFormat>
#include <QScreen>
#include <QLabel>
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
//    QSurfaceFormat fmt;
//    fmt.setVersion(4, 1);
//    fmt.setProfile(QSurfaceFormat::CoreProfile);
//    QSurfaceFormat::setDefaultFormat(fmt);

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
    settings.image_path = QString::fromStdString(input.value("IO/image_path").toString().toStdString());
    settings.output_path = QString::fromStdString(input.value("IO/output_path").toString().toStdString());
    // stipple init
    settings.init_stipple_num = input.value("INIT/init_stipple_num").toInt();
    settings.init_stipple_size = input.value("INIT/init_stipple_size").toFloat();
    settings.supersampling_factor = input.value("INIT/supersampling_factor").toInt();
    // wlbg para
    settings.max_iteration = input.value("WLBG/max_iteration").toInt();
    settings.hysteresis = input.value("WLBG/hysteresis").toFloat();
    settings.hysteresis_delta = input.value("WLBG/hysteresis_delta").toFloat();
    settings.adaptive_stipple_size = input.value("WLBG/adaptive_stipple_size").toBool();
    //Color
    //settings.palette = input.value("COLOR/palette").toStringList();
    settings.pre_image_path = settings.image_path;
    settings.input_path = args[0];

    // Create a GUI window
    MainWindow w;

    w.show();

    return a.exec();
}
