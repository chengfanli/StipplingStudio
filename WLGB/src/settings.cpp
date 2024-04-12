#include "settings.h"
#include <QSettings>

Settings settings;

void Settings::saveSettings() {
    QSettings s("stippling", "stippling");

    s.setValue("image_path", image_path);
}

