#include "wlbg.h"
#include "settings.h"

#include <iostream>
#include <set>
#include <vector>
#include <random>

using namespace Eigen;

std::vector<Stipple> WLBG::init_stipples()
{
    int init_num = settings.init_stipple_num;
    float init_size = settings.init_stipple_size;

    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_real_distribution<float> dis(0.01f, 0.99f);
    std::vector<Stipple> stipples(init_num);
    std::generate(stipples.begin(), stipples.end(), [&]() {
        return Stipple{Vector2f(dis(gen), dis(gen)), init_size,
                       Qt::black};
    });
    return stipples;
}

float WLBG::current_stipple_size(Cell cell)
{
    if (settings.adaptive_stipple_size)
    {
        auto size = settings.init_stipple_size;
        return std::clamp(size - 2.0f, 0.1f, size) * (1.0f - std::sqrt(cell.total_density / cell.area)) +
               (size + 2.0f) * std::sqrt(cell.total_density / cell.area);
    }
    else
        return settings.init_stipple_size;

    return settings.init_stipple_size;
}
