#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <unordered_set>

struct Settings {
    // IO
    std::string image_path; // image file path
    std::string output_path; // output image file path

    // stipple init
    int init_stipple_num; // init number of stipples
    float init_stipple_size; // omot size of stipples
    int supersampling_factor; // factor of supersampling the original image

    // wlbg para
    int max_iteration; // max iteration num
    float hysteresis; // hysteresis for split and merge
    float hysteresis_delta; // delta of hysteresis
    bool adaptive_stipple_size; // open adaptive size or not
};

// The global Settings object, will be initialized by MainWindow
extern Settings settings;

#endif // SETTINGS_H

