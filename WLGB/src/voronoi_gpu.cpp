#include "wlbg.h"
#include "settings.h"

#include <iostream>
#include <set>
#include <vector>
#include <random>
#include <omp.h>

#include "resources/shaders/Voronoi.frag.h"
#include "resources/shaders/Voronoi.vert.h"

#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVersionFunctionsFactory>

QVector<QVector2D> sites(const std::vector<Stipple> &stipples)
{
    QVector<QVector2D> sites(stipples.size());
    std::transform(stipples.begin(), stipples.end(), sites.begin(),
                   [](const auto &s) { return QVector2D(s.pos[0], s.pos[1]); });
    return sites;
}

// std::vector<Cell> WLBG::generate_voronoi_cells_gpu(std::vector<Stipple> points)
// {
//     auto indexMap = calculate(sites(points));
//     std::cout << "finish calculate" << std::endl;
//     return accumulateCells(indexMap);
// }

namespace CellEncoder {
QVector3D encode(const uint32_t index) {
    uint32_t r = (index >> 16) & 0xff;
    uint32_t g = (index >> 8) & 0xff;
    uint32_t b = (index >> 0) & 0xff;
    return QVector3D(r / 255.0f, g / 255.0f, b / 255.0f);
}

uint32_t decode(const uint32_t& r, const uint32_t& g, const uint32_t& b) {
    return 0x00000000 | (r << 16) | (g << 8) | b;
}
}  // namespace CellEncoder

IndexMap::IndexMap(int32_t w, int32_t h, int32_t count)
    : width(w), height(h), m_numEncoded(count) {
    m_data = QVector<uint32_t>(w * h);
}

void IndexMap::set(const int32_t x, const int32_t y, const uint32_t value) {
    m_data[y * width + x] = value;
}

uint32_t IndexMap::get(const int32_t x, const int32_t y) const {
    return m_data[y * width + x];
}

int32_t IndexMap::count() const { return m_numEncoded; }

uint calcNumConeSlices(const float radius, const float maxError)
{
    const float alpha = 2.0f * std::acos((radius - maxError) / radius);
    return static_cast<uint>(2 * M_PI / alpha + 0.5f);
}

std::vector<Cell> WLBG::accumulateCells(const IndexMap& map)
{
    // compute voronoi cell moments
    std::vector<Cell> cells = std::vector<Cell>(map.count());
    std::vector<Moments> moments = std::vector<Moments>(map.count());

    #pragma omp parallel for collapse(2)
    for (int x = 0; x < map.width; ++x) {
        for (int y = 0; y < map.height; ++y) {
            uint32_t index = map.get(x, y);

            QRgb densityPixel = m_density.pixel(x, y);
            float density = std::max(1.0f - qGray(densityPixel) / 255.0f,
                                     std::numeric_limits<float>::epsilon());

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

QVector<QVector3D> createConeDrawingData(const QSize& size)
{
    const float radius = std::sqrt(2.0f);
    const float maxError =
        1.0f / (size.width() > size.height() ? size.width() : size.height());
    const uint numConeSlices = calcNumConeSlices(radius, maxError);

    const float angleIncr = 2.0f * M_PI / numConeSlices;
    const float height = 1.99f;

    QVector<QVector3D> conePoints;
    conePoints.push_back(QVector3D(0.0f, 0.0f, height));

    const float aspect = static_cast<float>(size.width()) / size.height();

    for (uint i = 0; i < numConeSlices; ++i) {
        conePoints.push_back(QVector3D(radius * std::cos(i * angleIncr),
                                       aspect * radius * std::sin(i * angleIncr),
                                       height - radius));
    }

    conePoints.push_back(QVector3D(radius, 0.0f, height - radius));

    return conePoints;
}

void WLBG::init_voronoi_context()
{
    m_context = new QOpenGLContext();
    QSurfaceFormat format;
    format.setMajorVersion(3);
    format.setMinorVersion(3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    m_context->setFormat(format);
    m_context->create();

    m_surface = new QOffscreenSurface(Q_NULLPTR, m_context);
    m_surface->setFormat(m_context->format());
    m_surface->create();

    m_context->makeCurrent(m_surface);

    m_vao = new QOpenGLVertexArrayObject(m_context);
    m_vao->create();

    m_shaderProgram = new QOpenGLShaderProgram(m_context);
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                             voronoiVertex.c_str());
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                             voronoiFragment.c_str());
    m_shaderProgram->link();

    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
    m_fbo = new QOpenGLFramebufferObject(m_density.width(),
                                         m_density.height(), fboFormat);
    QVector<QVector3D> cones = createConeDrawingData(m_density.size());

    m_vao->bind();

    QOpenGLBuffer coneVBO = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    coneVBO.create();
    coneVBO.setUsagePattern(QOpenGLBuffer::StaticDraw);
    coneVBO.bind();
    coneVBO.allocate(cones.constData(), cones.size() * sizeof(QVector3D));
    coneVBO.release();

    m_shaderProgram->bind();

    coneVBO.bind();
    m_shaderProgram->enableAttributeArray(0);
    m_shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3);
    coneVBO.release();

    m_shaderProgram->release();

    m_vao->release();
}

IndexMap WLBG::calculate(const QVector<QVector2D>& points) {
    assert(!points.empty());

    m_context->makeCurrent(m_surface);

    auto gl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(m_context);

    m_vao->bind();

    m_shaderProgram->bind();

    QOpenGLBuffer vboPositions = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vboPositions.create();
    vboPositions.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vboPositions.bind();
    vboPositions.allocate(points.constData(), points.size() * sizeof(QVector2D));
    m_shaderProgram->enableAttributeArray(1);
    m_shaderProgram->setAttributeBuffer(1, GL_FLOAT, 0, 2);
    gl->glVertexAttribDivisor(1, 1);
    vboPositions.release();

    QVector<QVector3D> colors(points.size());
    uint32_t n = 0;
    std::generate(colors.begin(), colors.end(),
                  [&n]() mutable { return CellEncoder::encode(n++); });

    QOpenGLBuffer vboColors = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vboColors.create();
    vboColors.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vboColors.bind();
    vboColors.allocate(colors.data(), colors.size() * sizeof(QVector3D));
    m_shaderProgram->enableAttributeArray(2);
    m_shaderProgram->setAttributeBuffer(2, GL_FLOAT, 0, 3);
    gl->glVertexAttribDivisor(2, 1);
    vboColors.release();

    m_fbo->bind();

    gl->glViewport(0, 0, m_density.width(), m_density.height());

    gl->glDisable(GL_MULTISAMPLE);
    gl->glDisable(GL_DITHER);

    gl->glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);

    gl->glEnable(GL_DEPTH_TEST);

    gl->glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl->glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, m_coneVertices, points.size());

    m_shaderProgram->release();

    m_vao->release();

    QImage voronoiDiagram = m_fbo->toImage();
    //  voronoiDiagram.save("voronoiDiagram.png");

    m_fbo->release();
    m_context->doneCurrent();

    IndexMap idxMap(m_fbo->width(), m_fbo->height(), points.size());

    for (int y = 0; y < m_fbo->height(); ++y) {
        for (int x = 0; x < m_fbo->width(); ++x) {
            QRgb voroPixel = voronoiDiagram.pixel(x, y);

            int r = qRed(voroPixel);
            int g = qGreen(voroPixel);
            int b = qBlue(voroPixel);

            uint32_t index = CellEncoder::decode(r, g, b);
            assert(index <= points.size());

            idxMap.set(x, y, index);
        }
    }
    return idxMap;
}


