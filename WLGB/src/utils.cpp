#include "wlbg.h"
#include "settings.h"
#include <random>
#include <Eigen/Core>

using namespace Eigen;

float WLBG::calculate_lower_density_bound(float point_size, float hysteresis)
{
    float pointArea = M_PI * (point_size / 2.0f) * (point_size / 2.0f);
    return (1.0f - hysteresis / 2.0f) * pointArea * settings.supersampling_factor * settings.supersampling_factor;
}

float WLBG::calculate_upper_density_bound(float point_size, float hysteresis)
{
    float pointArea = M_PI * (point_size / 2.0f) * (point_size / 2.0f);
    return (1.0f + hysteresis / 2.0f) * pointArea * settings.supersampling_factor * settings.supersampling_factor;
}

Vector2f WLBG::jitter(Vector2f s)
{
    std::uniform_real_distribution<float> jitter_dis(-0.001f, 0.001f);
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return s += Vector2f(jitter_dis(gen), jitter_dis(gen));
}

int WLBG::min2(int a, int b)
{
    return (a < b) ? a : b;
}

int WLBG::max2(int a, int b)
{
    return (a > b) ? a : b;
}

int WLBG::min3(int a, int b, int c)
{
    return min2(a, min2(b, c));
}
int WLBG::max3(int a, int b, int c)
{
    return max2(a, max2(b, c));
}

int orientation(jcv_point p, jcv_point q, jcv_point r) {
    return (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
}

// Function to check if point p lies on the line segment 'pr'
bool onSegment(jcv_point p, jcv_point q, jcv_point r) {
    if (q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) &&
        q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y))
        return true;
    return false;
}

bool WLBG::isInsideTriangle(jcv_point a, jcv_point b, jcv_point c, jcv_point p) {
    // Check the orientation of the ordered triples
    int o1 = orientation(a, b, p);
    int o2 = orientation(b, c, p);
    int o3 = orientation(c, a, p);

    // General case
    if (o1 == 0 && onSegment(a, p, b)) return true; // Check if p is on segment ab
    if (o2 == 0 && onSegment(b, p, c)) return true; // Check if p is on segment bc
    if (o3 == 0 && onSegment(c, p, a)) return true; // Check if p is on segment ca

    // Check for general position of p inside the triangle
    if ((o1 > 0 && o2 > 0 && o3 > 0) || (o1 < 0 && o2 < 0 && o3 < 0))
        return true;

    return false; // If none of the above cases, then p is not inside abc
}
