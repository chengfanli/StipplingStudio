#include "mlbg.h"
#include "settings.h"
#include <iostream>
#include <QCoreApplication>
#include <random>
#include <unordered_map>
#include "wlbg.h"

#include <vector>

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
    std::vector<std::vector<Stipple>> Layers_stipples;// the vector of stipples in diff layers
    Layers_stipples.push_back(init_stipples(Qt::black, false));
    Layers_stipples.push_back(init_stipples(Qt::white, true));

    //number of Layer
    int layer_number = Layers_stipples.size();

    // stippling iteration
    int num_split = 0;
    int num_merge = 0;

    // all stipples from diff layers
    std::vector<Stipple> total_stipples;
    for (int i = 0; i < settings.max_iteration; i++)
    {
        std::vector<std::unordered_map<int, int> > Layers_map(Layers_stipples.size());
        draw d;
        std::cout << "Iteration: " << i << std::endl;

        // current hysteresis
        float hysteresis = settings.hysteresis + i * settings.hysteresis_delta;

        std::vector<std::vector<Cell>> Layers_cells;//sum of cells in diff layers
        total_stipples.clear();
        // cells
        for(int i = 0; i < Layers_stipples.size(); i++){
            auto &stipples = Layers_stipples[i];
            total_stipples.insert(total_stipples.end(), stipples.begin(), stipples.end());
            std::vector<Stipple> newStipples;
            std::vector<Cell> voronoi_cells = generate_voronoi_cells(stipples, newStipples, d, stipples[0].inverse);
            Layers_map[i] = computeMap(stipples, newStipples);
            Layers_cells.push_back(voronoi_cells);
        }
        std::vector<Stipple> new_total_stipples;
        std::vector<Cell> merge_cells = generate_voronoi_cells(total_stipples, new_total_stipples, d, false);// todo: confirm this step has nothing to do w/ density
        std::unordered_map<int, int> merge_map = computeMap(total_stipples, new_total_stipples);

        for(int i = 0; i < layer_number; i++) {
            for(int j = 0; j < Layers_stipples[i].size(); j++) {
                //within-class or cross-class
                //change the position of stipple
                float probability = density_sum - Layers_cells[i][j].average_density;
                //get a random number from 0 to 1
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<> dis(0.0, 1.0);
                float rnumber = dis(gen);
                if(rnumber < probability) { //within-class
                    if(Layers_map[i].find(j) == Layers_map[i].end()) continue;
                    Layers_stipples[i][j].pos = Layers_cells[i][Layers_map[i][j]].centroid;
                }else{ //cross-class
                    if(merge_map.find(i * Layers_stipples[i].size() + j) == merge_map.end()) continue;
                    Layers_stipples[i][j].pos = merge_cells[merge_map[i * Layers_stipples[i].size() + j]].centroid;
                }

            }
            //as the position of stipples change, need to generate new cells, check whether split or delete based on new cells
            std::vector<Cell> voronoi_cells = generate_voronoi_cells(Layers_stipples[i], d, Layers_stipples[i][0].inverse);
            //store color, as its whether black or white. This step may need to be changed when it comes to color imgae
            QColor color = Layers_stipples[i][0].color;
            bool inverse = Layers_stipples[i][0].inverse;
            Layers_stipples[i].clear();
            for (const auto &cell : voronoi_cells)
            {
                float point_size = current_stipple_size(cell);

                if (cell.total_density < calculate_lower_density_bound(point_size, hysteresis) || cell.area == 0.0f) {// merge
                    num_merge++;
                    d.drawPoints(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::black);
                    d.drawX(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::red);
                }
                else if (cell.total_density < calculate_upper_density_bound(point_size, hysteresis)) {// keep
                    Layers_stipples[i].push_back({cell.centroid, point_size, color, inverse});
                    d.drawPoints(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::black);
                }
                else // split
                {
                    num_split++;
                    split_cell(Layers_stipples[i], cell, point_size, color, inverse);

                    d.drawPoints(cell.centroid.x() * m_size.width(), cell.centroid.y() * m_size.height(), Qt::black);
                    auto last = Layers_stipples[i].back();
                    auto secondLast = Layers_stipples[i][Layers_stipples[i].size() - 2];
                    d.drawPoints(last.pos.x() * m_size.width(), last.pos.y() * m_size.height(), Qt::green);
                    d.drawPoints(secondLast.pos.x() * m_size.width(), secondLast.pos.y() * m_size.height(), Qt::green);
                }
            }
        }
        //put stipples from diff layers into totle stipple. used for display on canvas
        total_stipples.clear();
        for(auto &stipples: Layers_stipples){
            total_stipples.insert(total_stipples.end(), stipples.begin(), stipples.end());
        }
        // total_stipples.insert(total_stipples.end(), Layers_stipples[1].begin(), Layers_stipples[1].end());
        if (num_split == 0 && num_merge == 0)
            break;
        num_split = 0;
        num_merge = 0;

        if (isVoronoi == true) {
            d.endPaint(i, m_canvas);
        }
        else
        {
            m_mlbg->paint(m_canvas, total_stipples, i);
        }


        // Handle other events, allowing GUI updates
        QCoreApplication::processEvents();
    }

    return total_stipples;/*total_stipples?*/
}




std::unordered_map<int, int> MLBG::computeMap(std::vector<Stipple> points, std::vector<Stipple> newPoints){
    std::unordered_map<int, int> map;
    // assert(points.size() == newPoints.size() && "points don't have same size!");
    for(int i = 0; i < points.size(); i++) {
        points[i].id = i;
        newPoints[i].id = i;
    }
    std::sort(points.begin(), points.end(), [](const Stipple& a, const Stipple& b) {
        if (a.pos.x() == b.pos.x()) {
            return a.pos.y() < b.pos.y();
        }
        return a.pos.x() < b.pos.x();
    });

    std::sort(points.begin(), points.end(), [](const Stipple& a, const Stipple& b) {
        if (a.pos.x() == b.pos.x()) {
            return a.pos.y() < b.pos.y();  // 当 x 相等时，比较 y
        }
        return a.pos.x() < b.pos.x();
    });
    map.clear();
    for(int i = 0; i < (int)points.size(); i++) {
        map.insert(std::make_pair(points[i].id, newPoints[i].id));
    }
    return map;
}
