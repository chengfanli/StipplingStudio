#include "mlbg.h"
#include "settings.h"
#include <iostream>
#include <QCoreApplication>
#include <random>
#include <unordered_map>
#include <chrono>
#include <thread>
#include "wlbg.h"

#include <vector>
#include <algorithm>

MLBG::MLBG() {
    m_image = QImage(settings.image_path);
    m_density = m_image.scaledToWidth(
                           settings.supersampling_factor * m_image.width(),
                           Qt::SmoothTransformation
                           ).convertToFormat(QImage::Format_Grayscale8);
    m_size = m_image.size();
}

std::vector<Stipple> MLBG::stippling(Canvas *m_canvas, MLBG *m_mlbg, bool isVoronoi)
{
    // init
    std::vector<std::vector<Stipple>> layers_stipples;// the vector of stipples in diff layers
    layers_stipples.push_back(init_stipples(Qt::black, false));
    layers_stipples.push_back(init_stipples(Qt::white, true));
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

            std::vector<Cell> voronoi_cells = generate_voronoi_cells(stipples, indices_map[i], d, stipples[0].inverse);
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
            std::vector<Cell> voronoi_cells = generate_voronoi_cells(layers_stipples[i], indices_map[i], d, layers_stipples[i][0].inverse);
            std::vector<Stipple> stipple_pre_order = layers_stipples[i];// stipples in previous order
            layers_stipples[i].clear();
            for (unsigned long long cellId = 0; cellId < voronoi_cells.size(); cellId++)
            {
                const auto &cell = voronoi_cells[cellId];
                Stipple stipple = stipple_pre_order[indices_map[i][cellId]];
                stipple.layerId = i;

                float point_size = current_stipple_size(cell);

                if (cell.total_density < calculate_lower_density_bound(point_size, hysteresis) || cell.area == 0.0f) {// merge
                    num_merge++;
                    d.drawPoints(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::black);
                    d.drawX(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::red);
                }
                else if (cell.total_density < calculate_upper_density_bound(point_size, hysteresis)) {// keep
                    layers_stipples[i].push_back({cell.centroid, point_size, stipple.color, stipple.inverse, stipple.average_density, i});
                    d.drawPoints(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::black);
                }
                else // split
                {
                    num_split++;
                    split_cell(layers_stipples[i], cell, point_size, stipple);

                    d.drawPoints(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::black);
                    auto last = layers_stipples[i].back();
                    auto secondLast = layers_stipples[i][layers_stipples[i].size() - 2];
                    d.drawPoints(last.pos.x() * m_size.width(), last.pos.y() * m_size.height(), Qt::green);
                    d.drawPoints(secondLast.pos.x() * m_size.width(), secondLast.pos.y() * m_size.height(), Qt::green);
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

std::vector<Stipple> MLBG::filling(std::vector<Stipple> foregroundStipples, Canvas *m_canvas, MLBG *m_mlbg)
{
    std::cout << "================= Begin filling ==================" << std::endl;

    std::vector<Stipple> tempStipples;
    std::vector<Stipple> backgroundStipples;

    for (int i = 0; i < foregroundStipples.size(); i++) {
        tempStipples.push_back(foregroundStipples[i]);
        tempStipples[i].color = Qt::white;
        // backgroundStipples.push_back(tempStipples[i]);
        // backgroundStipples[i].size =  20.0f;
    }

    backgroundStipples.push_back(tempStipples[0]);

    QImage newImage = m_mlbg->paintBG(m_canvas, tempStipples, 0);
    tempStipples.clear();
    QImage newDensity = newImage.scaledToWidth(
                                      settings.supersampling_factor * newImage.width(),
                                      Qt::SmoothTransformation
                                      ).convertToFormat(QImage::Format_Grayscale8);

    QImage originImage = QImage(settings.image_path);
    QImage originDensity = originImage.scaledToWidth(
                           settings.supersampling_factor * originImage.width(),
                           Qt::SmoothTransformation
                           ).convertToFormat(QImage::Format_Grayscale8);
//    m_inv_density =



    printf("here\n");

    // stippling iteration
    int num_split = 0;
    int num_merge = 0;

//    std::this_thread::sleep_for(std::chrono::seconds(3));


    for (int i = 0; i < 18; i++)
    {
        draw d;
        std::cout << "Iteration: " << i << std::endl;

        // cells
        std::vector<Cell> voronoi_cells = generate_voronoi_cells_withDiffBGImage(backgroundStipples, d, newDensity);
        std::vector<Cell> cells_originImage = generate_voronoi_cells_withDiffBGImage(backgroundStipples, d, originDensity);
        std::cout << "Current number of points: " << backgroundStipples.size() << std::endl;
//        std::vector<int> merge_indices_map

        // current hysteresis
        float hysteresis = settings.hysteresis + i * settings.hysteresis_delta;

        backgroundStipples.clear();

        for (unsigned long long cellId = 0; cellId < voronoi_cells.size(); cellId++)
        {
            const auto &cell = voronoi_cells[cellId];
            float point_size = 4.0f;//current_stipple_size(cell);

            QColor stippleColor;
            if (cells_originImage[cellId].average_density >= cells_originImage[cellId].average_density_inv) {
                stippleColor = Qt::black;
            } else {
                stippleColor = Qt::white;
            }

            if (false){//cell.total_density < calculate_lower_density_bound(point_size, hysteresis) || cell.area == 0.0f) {// merge
                num_merge++;
                d.drawPoints(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), stippleColor);
                d.drawX(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::red);
            }
            else if (false){//(cell.total_density < calculate_upper_density_bound(point_size, hysteresis)) {// keep
                backgroundStipples.push_back({cell.centroid, point_size, stippleColor});
                d.drawPoints(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), stippleColor);
            }
            else // split
            {
                num_split++;
                split_cell(backgroundStipples, cell, point_size, stippleColor, false);
                d.drawPoints(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), stippleColor);
                auto last = backgroundStipples.back();
                auto secondLast = backgroundStipples[backgroundStipples.size() - 2];
                d.drawPoints(last.pos.x() * m_size.width(), last.pos.y() * m_size.height(), Qt::green);
                d.drawPoints(secondLast.pos.x() * m_size.width(), secondLast.pos.y() * m_size.height(), Qt::green);
            }


        }

        if (num_split == 0 && num_merge == 0)
            break;
        num_split = 0;
        num_merge = 0;

        m_mlbg->paint(m_canvas, backgroundStipples, 20);

        // Handle other events, allowing GUI updates
        QCoreApplication::processEvents();

    }

    for (int i = 0; i < foregroundStipples.size(); i++) {
        backgroundStipples.push_back(foregroundStipples[i]);
    }

    m_mlbg->paint(m_canvas, backgroundStipples, 20);

    // Handle other events, allowing GUI updates
    QCoreApplication::processEvents();

    return foregroundStipples;
}

std::vector<Cell> MLBG::generate_voronoi_cells_withDiffBGImage(std::vector<Stipple> points, draw &d, QImage newDensity)
{
    std::cout << 271 << std::endl;
    jcv_diagram diagram;
    auto count = points.size();
    jcv_clipper* clipper = 0;
    jcv_rect rect;

    std::cout << 277 << std::endl;

    jcv_point dimensions;
    dimensions.x = (jcv_real)newDensity.width();
    dimensions.y = (jcv_real)newDensity.height();

    std::cout << 283 << std::endl;

    d.init(dimensions.x, dimensions.y);

    std::cout << 287 << std::endl;

    jcv_point m;
    m.x = 0.0;
    m.y = 0.0;
    rect.min = m;
    rect.max = dimensions;

    std::cout << 295 << std::endl;

    memset(&diagram, 0, sizeof(jcv_diagram));
    std::cout << 298 << std::endl;
    jcv_point* temp = construct_jcv(points, &diagram.min, &dimensions, &dimensions);
    std::cout << 300 << std::endl;
    jcv_diagram_generate(count, temp, &rect, clipper, &diagram);
    std::cout << 302 << std::endl;

    std::cout << "Generating Voronoi Diagram" << std::endl;
    const jcv_site* sites = jcv_diagram_get_sites( &diagram );

    IndexMap idxMap(m_density.width(), m_density.height(), points.size());

    std::cout << "Painting Voronoi Diagram" << std::endl;

    for( int i = 0; i < diagram.numsites; ++i )
    {
        const jcv_site* site = &sites[i];
        jcv_point s = site->p;

        const jcv_graphedge* e = site->edges;
        while( e )
        {
            jcv_point p0 = e->pos[0];
            jcv_point p1 = e->pos[1];
            d.drawEdge(p0, p1, QColor(255, 192, 203));

            // Compute triangle bounding box
            jcv_point* v0 = &s;
            jcv_point* v1 = &p0;
            jcv_point* v2 = &p1;

            // Clip against screen bounds
            auto minX = max2(min3((int)v0->x, (int)v1->x, (int)v2->x), 0);
            auto minY = max2(min3((int)v0->y, (int)v1->y, (int)v2->y), 0);
            auto maxX = min2(max3((int)v0->x, (int)v1->x, (int)v2->x), newDensity.width() - 1);
            auto maxY = min2(max3((int)v0->y, (int)v1->y, (int)v2->y), newDensity.height() - 1);

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

    std::cout << "Calculating Voronoi Cell" << std::endl;
    auto res = accumulateCells_withDiffImage(idxMap, newDensity);
    return res;
}

std::vector<Cell> MLBG::accumulateCells_withDiffImage(const IndexMap& map, QImage density)
{
    // compute voronoi cell moments
    std::vector<Cell> cells = std::vector<Cell>(map.count());
    std::vector<Moments> moments = std::vector<Moments>(map.count());

    #pragma omp parallel for collapse(2)
    for (int x = 0; x < map.width; ++x) {
        for (int y = 0; y < map.height; ++y) {
            uint32_t index = map.get(x, y);

            QRgb densityPixel = density.pixel(x, y);
            float density = std::max(1.0f - qGray(densityPixel) / 255.0f * qAlpha(densityPixel) / 255.0f,
                                     std::numeric_limits<float>::epsilon());
//            if(inverse) {
//                density = 1.f - density;
//            }
            #pragma omp critical
            {
                Cell& cell = cells[index];
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
        cell.average_density_inv = 1.f - cell.average_density;

        // centroid
        cell.centroid[0] = (m10 / m00);
        cell.centroid[1] = (m01 / m00);

        // orientation
        float x = m20 / m00 - cell.centroid.x() * cell.centroid.x();
        float y = 2.0f * (m11 / m00 - cell.centroid.x() * cell.centroid.y());
        float z = m02 / m00 - cell.centroid.y() * cell.centroid.y();
        cell.orientation = std::atan2(y, x - z) / 2.0f;

        cell.centroid[0] = ((cell.centroid.x() + 0.5f) / density.width());
        cell.centroid[1] = ((cell.centroid.y() + 0.5f) / density.height());
    }
    return cells;
}


std::vector<Cell> MLBG::generate_voronoi_cells(std::vector<Stipple> points, std::vector<int> &indices, draw &d, bool inverse)
{
    jcv_diagram diagram;
    auto count = points.size();
    jcv_clipper* clipper = 0;
    jcv_rect rect;

    jcv_point dimensions;
    dimensions.x = (jcv_real)m_density.width();
    dimensions.y = (jcv_real)m_density.height();

    d.init(dimensions.x, dimensions.y);

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

    IndexMap idxMap(m_density.width(), m_density.height(), points.size());

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
            d.drawEdge(p0, p1, QColor(255, 192, 203));

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
    if((int)points.size() != (int)diagram.numsites) {
        std::cout<<"p"<<points.size()<<" "<<"num"<<diagram.numsites<<std::endl;
    }
    // assert((int)points.size() == (int)diagram.numsites);
    // std::cout << "Calculating Voronoi Cell" << std::endl;
    auto res = accumulateCells(idxMap, inverse);
    return res;
}


std::vector<int> MLBG::computeIdxMap(std::vector<int> indices, int size) { //switch map from "cell->stipple" to "stipple->cell"
    std::vector<int> ret(size); // size: the size of old points. might be bigger than cells size(indices.size())
    for(int i = 0; i < (int)indices.size(); i++) {
        ret[indices[i]] = i;
    }
    return ret;
}

void MLBG::split_cell(std::vector<Stipple>& stipples, Cell cell, float point_size, Stipple stipple)
{
    const float area = std::max(1.0f, cell.area);
    const float circleRadius = std::sqrt(area / M_PI);
    Eigen::Vector2f splitVector = Eigen::Vector2f(0.5f * circleRadius, 0.0f);

    const float a = cell.orientation;
    Eigen::Vector2f splitVectorRotated = Eigen::Vector2f(
        splitVector.x() * std::cos(a) - splitVector.y() * std::sin(a),
        splitVector.y() * std::cos(a) + splitVector.x() * std::sin(a));

    splitVectorRotated[0] = (splitVectorRotated[0] / m_density.width());
    splitVectorRotated[1] = (splitVectorRotated[1] / m_density.height());

    Eigen::Vector2f splitSeed1 = cell.centroid - splitVectorRotated;
    Eigen::Vector2f splitSeed2 = cell.centroid + splitVectorRotated;

    // check boundaries
    splitSeed1[0] = (std::max(0.0f, std::min(splitSeed1.x(), 1.0f)));
    splitSeed1[1] = (std::max(0.0f, std::min(splitSeed1.y(), 1.0f)));

    splitSeed2[0] = (std::max(0.0f, std::min(splitSeed2.x(), 1.0f)));
    splitSeed2[1] = (std::max(0.0f, std::min(splitSeed2.y(), 1.0f)));

    stipples.push_back({jitter(splitSeed1), point_size, stipple.color, stipple.inverse, stipple.average_density, stipple.layerId});
    stipples.push_back({jitter(splitSeed2), point_size, stipple.color, stipple.inverse, stipple.average_density, stipple.layerId});
}
