#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <unordered_set>

struct Settings {
    std::string image_path; // image file path for main object
};

// The global Settings object, will be initialized by MainWindow
extern Settings settings;

#endif // SETTINGS_H

