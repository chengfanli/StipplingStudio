#include "wlbg.h"
#include "settings.h"
#include <iostream>
#include <set>
#include <vector>
#include <util/nanoflann.hpp>
#include <vector>
#include <random>
#include <Eigen/Core>

using namespace Eigen;

// Adaptor for nanoflann, allows nanoflann to understand your data structure
template <typename T>
struct PointCloud {
    std::vector<Stipple>  pts;

    // Must return the number of data points
    inline size_t kdtree_get_point_count() const { return pts.size(); }

    // Returns the distance between the vector "p1[0:size-1]" and the data point with index "idx_p2" in the class:
    inline T kdtree_distance(const T *p1, const size_t idx_p2, size_t size) const {
        const T d0 = p1[0] - pts[idx_p2].pos[0];
        const T d1 = p1[1] - pts[idx_p2].pos[1];
        return d0 * d0 + d1 * d1;
    }

    // Returns the dim'th component of the idx'th point in the class:
    // Since this is in 2D, `dim` should be 0 for x, and 1 for y.
    inline T kdtree_get_pt(const size_t idx, int dim) const {
        if (dim == 0) return pts[idx].pos[0];
        else return pts[idx].pos[1];
    }

    // Optional bounding-box computation: return false to default to a standard bbox computation loop.
    // Return true if the BBOX was already computed by the class and returned in "bb" so it can be avoided to redo it again.
    // Look at bb.size() to find out the expected dimensionality (e.g. 2 or 3 for point clouds)
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /* bb */) const { return false; }
};

// Alias for convenience
using my_kd_tree_t = nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<float, PointCloud<float>>,
    PointCloud<float>,
    2 /* dim */
    >;

int WLBG::find_nearest_point(const std::vector<Stipple>& points, float pos_x, float pos_y)
{
    // Constructing the point cloud
    PointCloud<float> cloud;
    cloud.pts = points;

    // Constructing the kd-tree index:
    my_kd_tree_t index(2 /*dim*/, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10 /* max leaf */));
    index.buildIndex();

    // Query point:
    float query_pt[2] = { pos_x, pos_y };

    // Perform the search:
    size_t ret_index;
    float out_dist_sqr;
    nanoflann::KNNResultSet<float> resultSet(1);
    resultSet.init(&ret_index, &out_dist_sqr);
    index.findNeighbors(resultSet, &query_pt[0], nanoflann::SearchParameters(10));

    // `ret_index` now contains the index of the nearest point in the `points` vector.
    return static_cast<int>(ret_index);
}

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
