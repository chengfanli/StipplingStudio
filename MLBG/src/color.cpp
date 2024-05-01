#include "mlbg.h"
#include "wlbg.h"
#include "settings.h"
#include <iostream>
#include <random>
#include <QCoreApplication>

void MLBG::init_config() {
    // std::cout << "Image path: " << settings.pre_image_path.toStdString() << std::endl;
    // std::cout<<"Select Color Palette"<<std::endl;
    // system("cd Pylette && python run.py && cd ../");

    // std::cout << "Image path: " << settings.pre_image_path.toStdString() << std::endl;
    // std::cout<<"PreProcessing image"<<std::endl;
    // system("cd unmixer && python run.py && cd ../");

    m_image = QImage(settings.pre_image_path);
    std::cout << "Image path: " << settings.pre_image_path.toStdString() << std::endl;
    m_density = m_image.scaledToWidth(
                           settings.supersampling_factor * m_image.width(),
                           Qt::SmoothTransformation
                           ).convertToFormat(QImage::Format_Grayscale8);
    m_size = m_image.size();

    color_size = 0;
    for (QString &colorString : settings.palette) {
        colorString = colorString.trimmed();
        colors.push_back(QColor(colorString));
        color_size++;
    }

    for(int i = 0; i < color_size; i++) {
        QImage tmp_image = QImage(QString("./images/%1.png").arg(i));
        m_densities.push_back(tmp_image.scaledToWidth(
                                           settings.supersampling_factor * m_image.width(),
                                           Qt::SmoothTransformation
                                           ).convertToFormat(QImage::Format_Grayscale8));
    }
    std::vector<float> sum;
    for(int x = 0; x < m_image.width(); x++)
        for(int y = 0; y < m_image.height(); y++) {
            float density_sum = 0.f;
            for(int i = 0; i < color_size; i++) {
                QRgb densityPixel = m_densities[i].pixel(x, y);
                float density_i = std::max(1.0f - qGray(densityPixel) / 255.0f * qAlpha(densityPixel) / 255.0f,
                                           std::numeric_limits<float>::epsilon());
                density_sum += density_i;
            }
            sum_density_pixel[std::make_pair(x,y)] = std::clamp(density_sum, 0.0f, 1.0f);
        }

}
std::vector<Stipple> MLBG::init_stipples_color(QColor color, int id)
{
    int init_num = settings.init_stipple_num;
    float init_size = settings.init_stipple_size;

    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_real_distribution<float> dis(0.01f, 0.99f);
    std::vector<Stipple> stipples(init_num);
    std::generate(stipples.begin(), stipples.end(), [&]() {
        return Stipple{Eigen::Vector2f(dis(gen), dis(gen)), init_size,
                       color, false, 0.f, id};
    });

    return stipples;
}

std::vector<Stipple> MLBG::stippling_color(Canvas *m_canvas, MLBG *m_mlbg, bool isVoronoi)
{
    // init
    init_config();
    std::vector<std::vector<Stipple>> layers_stipples;// the vector of stipples in diff layers
    for(int i = 0; i < color_size; i++) {
        layers_stipples.push_back(init_stipples_color(colors[i], i));
    }
    std::vector<std::vector<Cell>> layers_cells;//the vector of cells in diff layers

    //number of Layer
    int layer_number = layers_stipples.size();

    // stippling iteration
    int num_split = 0;
    int num_merge = 0;

    // all stipples from diff layers
    std::vector<Stipple> total_stipples;

    //to map between stipple and cell
    std::vector<std::vector<int> > indices_map(layers_stipples.size()); //every layer
    std::vector<int> merge_indices_map;//all layers

    for (int it = 0; it < settings.max_iteration; it++)
    {
        draw d;
        std::cout << "Iteration: " << it << std::endl;

        // current hysteresis
        float hysteresis = settings.hysteresis + it * settings.hysteresis_delta;

        //clear vector data for last loop
        total_stipples.clear();
        layers_cells.clear();

        // cells
        for(int i = 0; i < layers_stipples.size(); i++){
            auto &stipples = layers_stipples[i];
            total_stipples.insert(total_stipples.end(), stipples.begin(), stipples.end());

            std::vector<Cell> voronoi_cells = generate_voronoi_cells_color(stipples, indices_map[i], i);
            indices_map[i] = computeIdxMap(indices_map[i], stipples.size());
            layers_cells.push_back(voronoi_cells);
        }

        std::vector<Cell> merge_cells = generate_voronoi_cells(total_stipples, merge_indices_map, d, false);
        merge_indices_map = computeIdxMap(merge_indices_map, total_stipples.size());

        int pre_stipples_num = 0;
        for(int i = 0; i < layer_number; i++) {
            for(int j = 0; j < layers_stipples[i].size(); j++) {
                //within-class or cross-class
                //change the position of stipple
                float density_sum = layers_cells[i][j].density_sum;
                float probability = density_sum - layers_cells[i][j].average_density;
                //get a random number from 0 to 1
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<> dis(0.0, 1.0);
                float rnumber = dis(gen);
                if(rnumber < probability) { //within-class
                    layers_stipples[i][j].pos = layers_cells[i][indices_map[i][j]].centroid;
                }else{ //cross-class
                    int idx = pre_stipples_num + j;
                    layers_stipples[i][j].pos = merge_cells[merge_indices_map[idx]].centroid;
                }
                layers_stipples[i][j].average_density = layers_cells[i][indices_map[i][j]].average_density;
            }

            pre_stipples_num += layers_stipples[i].size(); // this is for merge_cells[merge_indices_map[idx]].centroid;
            //as the position of stipples change, need to generate new cells, check whether split or delete based on new cells
            std::vector<Cell> voronoi_cells = generate_voronoi_cells_color(layers_stipples[i], indices_map[i], i);
            std::vector<Stipple> stipple_pre_order = layers_stipples[i];// stipples in previous order
            layers_stipples[i].clear();
            for (unsigned long long cellId = 0; cellId < voronoi_cells.size(); cellId++)
            {
                const auto &cell = voronoi_cells[cellId];
                Stipple stipple = stipple_pre_order[indices_map[i][cellId]];
                stipple.layerId = i;

                float point_size = current_stipple_size(cell);

                if (voronoi_cells.size() > 2 && (cell.total_density < calculate_lower_density_bound(point_size, hysteresis) || cell.area == 0.0f)) {// merge
                    num_merge++;
                    // std::cout<<"merge"<<std::endl;
                }
                else if (cell.total_density < calculate_upper_density_bound(point_size, hysteresis)) {// keep
                    layers_stipples[i].push_back({cell.centroid, point_size, stipple.color, stipple.inverse, stipple.average_density, i});
                    // std::cout<<"keep"<<std::endl;

                }
                else // split
                {
                    num_split++;
                    split_cell(layers_stipples[i], cell, point_size, stipple);
                    // std::cout<<"split"<<std::endl;

                }
            }
        }
        //put stipples from diff layers into totle stipple. used for display on canvas
        total_stipples.clear();
        for(auto &stipples: layers_stipples){
            total_stipples.insert(total_stipples.end(), stipples.begin(), stipples.end());
        }
        //sort based on density
        std::sort(total_stipples.begin(), total_stipples.end(),
                  [](const Stipple& a, const Stipple& b) {
                      return a.average_density > b.average_density;
                  });
        if (num_split == 0 && num_merge == 0)
            break;
        num_split = 0;
        num_merge = 0;

        if (isVoronoi == true) {
            d.endPaint(it, m_canvas);
        }
        else
        {
            m_mlbg->paint(m_canvas, total_stipples, it);
        }


        // Handle other events, allowing GUI updates
        QCoreApplication::processEvents();
    }

    return total_stipples;
}



std::vector<Cell> MLBG::generate_voronoi_cells_color(std::vector<Stipple> points, std::vector<int> &indices, int id)
{
    jcv_diagram diagram;
    auto count = points.size();
    jcv_clipper* clipper = 0;
    jcv_rect rect;

    jcv_point dimensions;
    dimensions.x = (jcv_real)m_densities[id].width();
    dimensions.y = (jcv_real)m_densities[id].height();

    jcv_point m;
    m.x = 0.0;
    m.y = 0.0;
    rect.min = m;
    rect.max = dimensions;

    memset(&diagram, 0, sizeof(jcv_diagram));
    jcv_point* temp = construct_jcv(points, &diagram.min, &dimensions, &dimensions);
    jcv_diagram_generate(count, temp, &rect, clipper, &diagram);

    // std::cout << "Generating Voronoi Diagram" << std::endl;
    const jcv_site* sites = jcv_diagram_get_sites( &diagram );

    IndexMap idxMap(m_densities[id].width(), m_densities[id].height(), points.size());

    indices.clear();
    // std::cout << "Painting Voronoi Diagram" << std::endl;

    for( int i = 0; i < diagram.numsites; ++i )
    {
        const jcv_site* site = &sites[i];
        jcv_point s = site->p;
        indices.push_back(site->index);

        const jcv_graphedge* e = site->edges;
        while( e )
        {
            jcv_point p0 = e->pos[0];
            jcv_point p1 = e->pos[1];

            // Compute triangle bounding box
            jcv_point* v0 = &s;
            jcv_point* v1 = &p0;
            jcv_point* v2 = &p1;

            // Clip against screen bounds
            auto minX = max2(min3((int)v0->x, (int)v1->x, (int)v2->x), 0);
            auto minY = max2(min3((int)v0->y, (int)v1->y, (int)v2->y), 0);
            auto maxX = min2(max3((int)v0->x, (int)v1->x, (int)v2->x), m_density.width() - 1);
            auto maxY = min2(max3((int)v0->y, (int)v1->y, (int)v2->y), m_density.height() - 1);

            // Rasterize
            jcv_point p;
            for (int y = minY; y <= maxY; y++)
            {
                for (int x = minX; x <= maxX; x++)
                {
                    //                    p = jcv_point(x, y);
                    p.x = x;
                    p.y = y;

                    if (isInsideTriangle(p0, p1, s, p))
                        idxMap.set(p.x, p.y, (uint32_t)i);
                }
            }
            e = e->next;
        }
    }

    jcv_diagram_free( &diagram );
    // if((int)points.size() != (int)diagram.numsites) {
    //     std::cout<<"p"<<points.size()<<" "<<"num"<<diagram.numsites<<std::endl;
    // }
    // assert((int)points.size() == (int)diagram.numsites);
    // std::cout << "Calculating Voronoi Cell" << std::endl;
    auto res = accumulateCells_color(idxMap, id);
    return res;
}

std::vector<Cell> MLBG::accumulateCells_color(const IndexMap& map, int id)
{
    // compute voronoi cell moments
    std::vector<Cell> cells = std::vector<Cell>(map.count());
    // std::cout<<"map size"<<map.count()<<std::endl;
    std::vector<Moments> moments = std::vector<Moments>(map.count());

#pragma omp parallel for collapse(2)
    for (int x = 0; x < map.width; ++x) {
        for (int y = 0; y < map.height; ++y) {
            uint32_t index = map.get(x, y);

            QRgb densityPixel = m_densities[id].pixel(x, y);
            float density = std::max(1.0f - qGray(densityPixel) / 255.0f * qAlpha(densityPixel) / 255.0f,
                                    std::numeric_limits<float>::epsilon());


#pragma omp critical
            {
                Cell& cell = cells[index];
                cell.density_sum +=sum_density_pixel[std::make_pair(x,y)];
                cell.area++;
                cell.total_density += density;

                Moments& m = moments[index];
                m.moment00 += density;
                m.moment10 += x * density;
                m.moment01 += y * density;
                m.moment11 += x * y * density;
                m.moment20 += x * x * density;
                m.moment02 += y * y * density;
            }
        }
    }

    // compute cell quantities
#pragma omp parallel for
    for (size_t i = 0; i < cells.size(); ++i) {
        Cell& cell = cells[i];
        if (cell.total_density <= 0.0f) continue;

        auto [m00, m10, m01, m11, m20, m02] = moments[i];

        //density
        cell.average_density = cell.total_density / cell.area;
        std::clamp(cell.average_density, 0.f, 1.f);

        cell.density_sum = cell.density_sum / cell.area;
        if(cell.density_sum > 1.0)
            std::cout<<"density_sum:" << cell.density_sum<<std::endl;
        // centroid
        cell.centroid[0] = (m10 / m00);
        cell.centroid[1] = (m01 / m00);

        // orientation
        float x = m20 / m00 - cell.centroid.x() * cell.centroid.x();
        float y = 2.0f * (m11 / m00 - cell.centroid.x() * cell.centroid.y());
        float z = m02 / m00 - cell.centroid.y() * cell.centroid.y();
        cell.orientation = std::atan2(y, x - z) / 2.0f;

        cell.centroid[0] = ((cell.centroid.x() + 0.5f) / m_density.width());
        cell.centroid[1] = ((cell.centroid.y() + 0.5f) / m_density.height());
    }
    return cells;
}


