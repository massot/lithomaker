#ifndef __LITHOPHANE_H__
#define __LITHOPHANE_H__

#include <tuple>

#include <cmath>

#include <QObject>
#include <QImage>
#include <QVector3D>
#include <QList>


class Lithophane : public QObject
{
    Q_OBJECT

public:
    Lithophane();
    ~Lithophane();

    void reset();
    void configure(
        const QImage& image,
        float width,
        float totalThickness, float minThickness,
        float frameBorder, float frameSlopeFactor,
        bool permanentStabilizers = false,
        float stabilizerHeightFactor = 0.15f, float stabilizerThreshold = 0,
        uint32_t noOfHangers = 0
    );
    void generate();
    const QList<QVector3D>& getVertices() const { return m_VerticesData; }
    std::tuple<bool, QString> saveToStl(const QString& path, const QString& format, const bool overrideFile);

    float getHeight() { return totalHeight; }

    // SPHERE
    void uv_sphere(uint32_t n_slices, uint32_t n_stacks, float radius=10.0f, bool inner = false)
    {
        setXDisplacement(0.0f);

        QList<QVector3D> vertices;
        // top vertex
        QVector3D v0 = {0, 1.0f, 0};
        vertices.append(v0);

        float inc = 1.0f;
        const int ll = (n_stacks / 2) - 100;
        const int lh = (n_stacks / 2) + 100;
        // generate vertices per stack / slice
        for (uint32_t i = 0; i < n_stacks - 1; i++)
        {
            float phi = M_PI * float(i + 1) / float(n_stacks);

            for (uint32_t j = 0; j < n_slices; j++)
            {
                float theta = 2.0f * M_PI * float(j) / float(n_slices);
                float x = std::sin(phi) * std::cos(theta);
                float y = std::cos(phi);
                float z = std::sin(phi) * std::sin(theta);

                if(!inner && i >= ll && i <= lh && j >= ll && j <= lh) inc = 1.1f;
                else inc = 1.0f;
                
                vertices.append(QVector3D{x, y, z} * radius * inc);
            }
        }

        // bottom vertex
        QVector3D v1 = {0, -1.0f, 0};
        vertices.append(v1 * radius);

        // top / bottom triangles
        for (uint32_t i = 0; i < n_slices; ++i)
        {
            uint32_t i0 = i + 1;
            uint32_t i1 = (i + 1) % n_slices + 1;
            if(!inner)
                addTriangle(v0, vertices.at(i1), vertices.at(i0));
            else
                addTriangle(v0, vertices.at(i0), vertices.at(i1));

            i0 = i + n_slices * (n_stacks - 2) + 1;
            i1 = (i + 1) % n_slices + n_slices * (n_stacks - 2) + 1;
            if(!inner)
                addTriangle(v1, vertices.at(i0), vertices.at(i1));
            else
                addTriangle(v1, vertices.at(i1), vertices.at(i0));
        }

        // add quads per stack / slice
        for (uint32_t j = 0; j < n_stacks - 2; j++)
        {
            uint32_t j0 = j * n_slices + 1;
            uint32_t j1 = (j + 1) * n_slices + 1;
            for (uint32_t i = 0; i < n_slices; i++)
            {
                uint32_t i0 = j0 + i;
                uint32_t i1 = j0 + (i + 1) % n_slices;
                uint32_t i2 = j1 + (i + 1) % n_slices;
                uint32_t i3 = j1 + i;
                if(!inner)
                    addQuad(vertices.at(i0), vertices.at(i1), vertices.at(i2), vertices.at(i3));
                else
                    addQuad(vertices.at(i0), vertices.at(i3), vertices.at(i2), vertices.at(i1));
            }
        }
    }
    // ------

signals:
    void progress(int value); // 0 - 100%

private:
    void renderImage();
    void addFrame();
    void addHangers();
    void addStabilizers();
    
    float getPixel(const QImage &image, const int &x, const int &y)
    {
        return image.pixelColor(x, image.height() - 1 - y).red() * depthFactor * -1;
    }

    QVector3D getVertex(float x, float y, float z, const bool scale = false)
    {
        float add = 0.0;
        if (scale)
        {
            x = (x * widthFactor);
            y = y * widthFactor;
            add = frameBorder;
        }
        return QVector3D(x + add + xDisplacement, y + add, z);
    }

    void setXDisplacement(float v)
    {
        xDisplacement = v;
    }

    void addTriangle(const QVector3D& p1, const QVector3D& p2, const QVector3D& p3, bool scale = false)
    {
        m_VerticesData.append(getVertex(p1.x(), p1.y(), p1.z(), scale));
        m_VerticesData.append(getVertex(p2.x(), p2.y(), p2.z(), scale));
        m_VerticesData.append(getVertex(p3.x(), p3.y(), p3.z(), scale));
    }

    void addQuad(const QVector3D& p1, const QVector3D& p2, const QVector3D& p3, const QVector3D& p4, bool scale = false)
    {
        addTriangle(p1, p2, p3, scale);
        addTriangle(p3, p4, p1, scale);
    }

    void addCube(const QVector3D& position, const QVector3D& size, bool scale = false)
    {
        /* bottom              top
           p2  ----  p3     |  p4  ----  p3              
              |    |        |     | 1 |
           p1 *----* p4     |  p1 *----  p2
        */
        QVector3D bp1 = position, bp2, bp3, bp4; // Bottom
        bp2 = {bp1.x()           , bp1.y(), bp1.z() - size.z()};
        bp3 = {bp1.x() + size.x(), bp1.y(), bp1.z() - size.z()};
        bp4 = {bp1.x() + size.x(), bp1.y(), bp1.z()           };

        QVector3D tp1, tp2, tp3, tp4; // Top
        tp1 = {bp1.x(), bp1.y() + size.y(), bp1.z()};
        tp2 = {bp4.x(), bp4.y() + size.y(), bp4.z()};
        tp3 = {bp3.x(), bp3.y() + size.y(), bp3.z()};
        tp4 = {bp2.x(), bp2.y() + size.y(), bp2.z()};

        addQuad(bp1, bp2, bp3, bp4, scale); // bottom
        addQuad(tp1, tp2, tp3, tp4, scale); // top
        addQuad(tp2, tp1, bp1, bp4, scale); // front
        addQuad(tp4, tp3, bp3, bp2, scale); // back
        addQuad(tp1, tp4, bp2, bp1, scale); // left
        addQuad(tp3, tp2, bp4, bp3, scale); // right
    }
    
    QImage image;
    QList<QVector3D> m_VerticesData;

    float width;
    float totalThickness, minThickness, minThicknessInv;
    float totalHeight;

    float depthFactor = -1.0;
    float widthFactor = -1.0;
    float frameBorder = -1.0;
    uint32_t noOfHangers = 0;
    float frameSlopeFactor = 0.75;
    bool permanentStabilizers = false;
    float stabilizerHeightFactor = 0.15;
    float stabilizerThreshold = 0;

    float xDisplacement = 0.0f;
};

#endif