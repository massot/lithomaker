#include "lithophane.h"

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <sstream>

#include <QFile>
#include <QtEndian>


Lithophane::Lithophane() {}

Lithophane::~Lithophane() {}

void Lithophane::reset()
{
    m_VerticesData.clear();
}

void Lithophane::configure(
    const QImage& image,
    float width,
    float totalThickness, float minThickness,
    float frameBorder, float frameSlopeFactor,
    bool permanentStabilizers,
    float stabilizerHeightFactor, float stabilizerThreshold,
    uint32_t noOfHangers
)
{
    this->image = image;
    this->width = width;
    this->totalThickness = totalThickness;
    this->minThickness = minThickness;
    this->frameBorder = frameBorder;
    this->noOfHangers = noOfHangers;
    this->frameSlopeFactor = frameSlopeFactor;
    this->permanentStabilizers = permanentStabilizers;
    this->stabilizerHeightFactor = stabilizerHeightFactor;
    this->stabilizerThreshold = stabilizerThreshold;

    widthFactor = (width - (frameBorder * 2.0f)) / image.width();
    depthFactor = -1.0f * ((totalThickness - minThickness) / 255.0f);
    minThicknessInv = -1.0f * minThickness;
    totalHeight = ((frameBorder * 2.0f) + (image.height() * widthFactor));
}

void Lithophane::generate()
{
    renderImage();
    addFrame();
    if(noOfHangers > 0) addHangers();
    if(stabilizerThreshold > 0 and width > stabilizerThreshold) addStabilizers();
}

void Lithophane::renderImage()
{
    emit progress(0);

    uint32_t numQuads = (image.width() - 1) * (image.height() - 1);
    uint32_t numVertices = 4 * numQuads;

    m_VerticesData.reserve(numVertices);

    for (int y = 0; y < (image.height() - 1); ++y)
    {
        // Close left side
        addQuad(
            {0, (float) y    , minThicknessInv          },
            {0, (float) y    , getPixel(image, 0, y    )},
            {0, (float) y + 1, getPixel(image, 0, y + 1)},
            {0, (float) y + 1, minThicknessInv          },
            true
        );

        for (int x = 0; x < (image.width() - 1); ++x)
        {
            if (y == 0)
            {
                // Close bottom
                addQuad(
                    {(float) x + 1, 0, getPixel(image, x + 1, 0)},
                    {(float) x    , 0, getPixel(image, x, 0)},
                    {(float) x    , 0, minThicknessInv},
                    {(float) x + 1, 0, minThicknessInv},
                    true
                );

                // Close top
                addQuad(
                    {(float) x    , (float) image.height() - 1, minThicknessInv},
                    {(float) x    , (float) image.height() - 1, getPixel(image, x    , image.height() - 1)},
                    {(float) x + 1, (float) image.height() - 1, getPixel(image, x + 1, image.height() - 1)},
                    {(float) x + 1, (float) image.height() - 1, minThicknessInv},
                    true
                );
            }
            // The lithophane heightmap
            addQuad(
                {(float) x    , (float) y    , getPixel(image, x    , y    )},
                {(float) x + 1, (float) y    , getPixel(image, x + 1, y    )},
                {(float) x + 1, (float) y + 1, getPixel(image, x + 1, y + 1)},
                {(float) x    , (float) y + 1, getPixel(image, x    , y + 1)},
                true
            );
        }
        // Close right side
        addQuad(
            {(float) image.width() - 1, (float) y + 1, getPixel(image, image.width() - 1, y + 1)},
            {(float) image.width() - 1, (float) y    , getPixel(image, image.width() - 1, y    )},
            {(float) image.width() - 1, (float) y    , minThicknessInv                          },
            {(float) image.width() - 1, (float) y + 1, minThicknessInv                          },
            true
        );

        emit this->progress((int)((float)y / (float)(image.height() - 1) * 100.0f));
    }
    emit this->progress(100);

    // Backside
    addQuad(
        {0                        , (float) image.height() - 1, minThicknessInv},
        {(float) image.width() - 1, (float) image.height() - 1, minThicknessInv},
        {0                        , 0                         , minThicknessInv},
        {(float) image.width() - 1, 0                         , minThicknessInv},
        true
    );
}

std::tuple<bool, QString> Lithophane::saveToStl(const QString &path, const QString& format, const bool overrideFile)
{
    emit this->progress(0);

    bool isBinaryOut = (format == "binary");
    std::stringstream buffer;

    std::ios_base::openmode open_mode = std::ios::trunc;
    if(isBinaryOut) open_mode = std::ios::binary;
    std::ofstream out(path.toStdString(), open_mode);

    auto writeTriangle = [isBinaryOut, &out, &buffer](const QVector3D& normal, const QList<QVector3D> &points)
    {
        static float normalx, normaly, normalz, x, y, z;
        constexpr uint16_t attrByteCount = 0;

        normalx = normal.x();
        normaly = normal.y();
        normalz = normal.z();

        if(isBinaryOut)
        {
            out.write((char *)&normalx, sizeof(float));
            out.write((char *)&normaly, sizeof(float));
            out.write((char *)&normalz, sizeof(float));
        }
        else {
            buffer << "facet normal " << normalx << " " << normaly << " " << normalz << std::endl;
            buffer << "outer loop" << std::endl;
        }

        for(const QVector3D& p : points)
        {
            x = p.x();
            y = p.y();
            z = p.z();
            if(isBinaryOut)
            {
                out.write((char *)&x, sizeof(float));
                out.write((char *)&y, sizeof(float));
                out.write((char *)&z, sizeof(float));
            }
            else
            {
                buffer << "vertex " << p.x() << " " << p.y() << " " << p.z() << std::endl;                
            }
        }
        
        if(isBinaryOut)
        {
            out.write((char *)&attrByteCount, sizeof(uint16_t));
        }
        else
        {
            buffer << "endloop" << std::endl;
            buffer << "endfacet" << std::endl;
        }
    };

    if (m_VerticesData.isEmpty())
    {
        return {false, tr("There is currently no rendered lithophane in the STL buffer. You need to render one before you can export it.")};
    }
    if (QFile::exists(path) && !overrideFile)
    {
        return {false, tr("The output STL file already exists. Do you want to overwrite it?")};
    }

    printf("Exporting to file: '%s'... \n", path.toStdString().c_str());

    QVector3D normal;

    if (out.good())
    {
        if(isBinaryOut)
        {
            char title[80];
            memset(title, 0, 80);
            strcpy(title, "lithophane");
            out.write((char *)&title, 80);

            uint32_t polCount = m_VerticesData.length() / 3;
            if(QSysInfo::ByteOrder == QSysInfo::BigEndian)
            {
                polCount = qToLittleEndian(polCount);
            }

            out.write((char *)&polCount, sizeof(uint32_t));
        }
        else buffer << "solid lithophane" << std::endl;
        
        for (int a = 0; a < m_VerticesData.length(); a += 3)
        {
            normal = QVector3D::normal(
                m_VerticesData.at(a), m_VerticesData.at(a + 1), m_VerticesData.at(a + 2)
            );
            writeTriangle(
                normal,
                {m_VerticesData.at(a),
                 m_VerticesData.at(a + 1),
                 m_VerticesData.at(a + 2)}
            );

            emit this->progress((int)((float)a / (float)(m_VerticesData.length()) * 100.0f));
        }

        if(!isBinaryOut)
        {
            buffer << "endsolid" << std::endl;
            // Write buffer to disc all at once
            printf("writing file to disc...\n");
            out << buffer.str();
        }
        out.close();
        
        emit this->progress(100);
        printf("Success!\n");
        return {true, tr("The binary STL was successfully exported. You can now import it in your preferred 3D printing slicer.")};
    }
    else
    {
        printf("Failed!\n");
        return {false, tr("File could not be opened for writing. Please check export filename and try again.")};
    }
}

void Lithophane::generate()
{
    setXDisplacement(-width / 2.0f);
    renderImage();
    // addFrame();
    // if(noOfHangers > 0) addHangers();
    // if(stabilizerThreshold > 0 and width > stabilizerThreshold) addStabilizers();
}

void Lithophane::renderImage()
{
    emit progress(0);

    uint32_t numQuads = (image.width() - 1) * (image.height() - 1);
    uint32_t numVertices = 4 * numQuads;

    m_VerticesData.clear();
    m_VerticesData.reserve(numVertices);

    for (int y = 0; y < image.height() - 1; ++y)
    {
        // Close left side
        addQuad(
            {0, (float) y    , minThicknessInv          },
            {0, (float) y    , getPixel(image, 0, y    )},
            {0, (float) y + 1, getPixel(image, 0, y + 1)},
            {0, (float) y + 1, minThicknessInv          },
            true
        );

        for (int x = 0; x < image.width() - 1; ++x)
        {
            if (y == 0)
            {
                // Close bottom
                addQuad(
                    {(float) x + 1, 0, getPixel(image, x + 1, 0)},
                    {(float) x    , 0, getPixel(image, x, 0)},
                    {(float) x    , 0, minThicknessInv},
                    {(float) x + 1, 0, minThicknessInv},
                    true
                );

                // Close top
                addQuad(
                    {(float) x    , (float) image.height() - 1, minThicknessInv},
                    {(float) x    , (float) image.height() - 1, getPixel(image, x    , image.height() - 1)},
                    {(float) x + 1, (float) image.height() - 1, getPixel(image, x + 1, image.height() - 1)},
                    {(float) x + 1, (float) image.height() - 1, minThicknessInv},
                    true
                );
            }
            // The lithophane heightmap
            addQuad(
                {(float) x    , (float) y    , getPixel(image, x    , y    )},
                {(float) x + 1, (float) y    , getPixel(image, x + 1, y    )},
                {(float) x + 1, (float) y + 1, getPixel(image, x + 1, y + 1)},
                {(float) x    , (float) y + 1, getPixel(image, x    , y + 1)},
                true
            );
        }
        // Close right side
        addQuad(
            {(float) image.width() - 1, (float) y + 1, getPixel(image, image.width() - 1, y + 1)},
            {(float) image.width() - 1, (float) y    , getPixel(image, image.width() - 1, y    )},
            {(float) image.width() - 1, (float) y    , minThicknessInv                          },
            {(float) image.width() - 1, (float) y + 1, minThicknessInv                          },
            true
        );

        emit this->progress((int)((float)y / (float)(image.height() - 1) * 100.0f));
    }
    emit this->progress(100);

    // Backside
    addQuad(
        {0                        , 0                         , minThicknessInv},
        {0                        , (float) image.height() - 1, minThicknessInv},
        {(float) image.width() - 1, (float) image.height() - 1, minThicknessInv},
        {(float) image.width() - 1, 0                         , minThicknessInv},
        true
    );
}

void Lithophane::addFrame()
{
    float w = width;
    float h = totalHeight;
    float depth = (totalThickness - minThickness);
    float frameSlope = (depth * frameSlopeFactor);

    addTriangle({w, h, minThicknessInv}, {0.0f, h, minThicknessInv}, {0.0f, h, depth});
    addTriangle({w, h, minThicknessInv}, {0.0f, h, depth}, {w, h, depth});
    addTriangle({w - frameBorder - frameSlope, frameBorder + frameSlope, 0.0f}, {frameBorder + frameSlope, h - frameBorder - frameSlope, 0.0f}, {w - frameBorder - frameSlope, h - frameBorder - frameSlope, 0.0f});
    addTriangle({w - frameBorder - frameSlope, frameBorder + frameSlope, 0.0f}, {frameBorder + frameSlope, frameBorder + frameSlope, 0.0f}, {frameBorder + frameSlope, h - frameBorder - frameSlope, 0.0f});
    addTriangle({0.0f, 0.0f, depth}, {0.0f, h, depth}, {0.0f, h, minThicknessInv});
    addTriangle({0.0f, 0.0f, depth}, {0.0f, h, minThicknessInv}, {0.0f, 0.0f, minThicknessInv});
    addTriangle({0.0f, 0.0f, minThicknessInv}, {w, 0.0f, minThicknessInv}, {w, 0.0f, depth});
    addTriangle({0.0f, 0.0f, minThicknessInv}, {w, 0.0f, depth}, {0.0f, 0.0f, depth});
    addTriangle({w, 0.0f, minThicknessInv}, {w, h, minThicknessInv}, {w, h, depth});
    addTriangle({w, 0.0f, minThicknessInv}, {w, h, depth}, {w, 0.0f, depth});
    addTriangle({0.0f, 0.0f, minThicknessInv}, {0.0f, h, minThicknessInv}, {w, h, minThicknessInv});
    addTriangle({0.0f, 0.0f, minThicknessInv}, {w, h, minThicknessInv}, {w, 0.0f, minThicknessInv});
    addTriangle({frameBorder, frameBorder, depth}, {frameBorder, h - frameBorder, depth}, {0.0f, h, depth});
    addTriangle({frameBorder, frameBorder, depth}, {0.0f, h, depth}, {0.0f, 0.0f, depth});
    addTriangle({w - frameBorder, h - frameBorder, depth}, {w - frameBorder, frameBorder, depth}, {w, 0.0f, depth});
    addTriangle({w - frameBorder, h - frameBorder, depth}, {w, 0.0f, depth}, {w, h, depth});
    addTriangle({frameBorder, h - frameBorder, depth}, {w - frameBorder, h - frameBorder, depth}, {w, h, depth});
    addTriangle({frameBorder, h - frameBorder, depth}, {w, h, depth}, {0.0f, h, depth});
    addTriangle({w - frameBorder, frameBorder, depth}, {frameBorder, frameBorder, depth}, {0.0f, 0.0f, depth});
    addTriangle({w - frameBorder, frameBorder, depth}, {0.0f, 0.0f, depth}, {w, 0.0f, depth});
    addTriangle({frameBorder + frameSlope, frameBorder + frameSlope, 0.0f}, {frameBorder + frameSlope, h - frameBorder - frameSlope, 0.0f}, {frameBorder, h - frameBorder, depth});
    addTriangle({frameBorder + frameSlope, frameBorder + frameSlope, 0.0f}, {frameBorder, h - frameBorder, depth}, {frameBorder, frameBorder, depth});
    addTriangle({w - frameBorder - frameSlope, h - frameBorder - frameSlope, 0.0f}, {w - frameBorder - frameSlope, frameBorder + frameSlope, 0.0f}, {w - frameBorder, frameBorder, depth});
    addTriangle({w - frameBorder - frameSlope, h - frameBorder - frameSlope, 0.0f}, {w - frameBorder, frameBorder, depth}, {w - frameBorder, h - frameBorder, depth});
    addTriangle({frameBorder + frameSlope, h - frameBorder - frameSlope, 0.0f}, {w - frameBorder - frameSlope, h - frameBorder - frameSlope, 0.0f}, {w - frameBorder, h - frameBorder, depth});
    addTriangle({frameBorder + frameSlope, h - frameBorder - frameSlope, 0.0f}, {w - frameBorder, h - frameBorder, depth}, {frameBorder, h - frameBorder, depth});
    addTriangle({w - frameBorder - frameSlope, frameBorder + frameSlope, 0.0f}, {frameBorder + frameSlope, frameBorder + frameSlope, 0.0f}, {frameBorder, frameBorder, depth});
    addTriangle({w - frameBorder - frameSlope, frameBorder + frameSlope, 0.0f}, {frameBorder, frameBorder, depth}, {w - frameBorder, frameBorder, depth}); 
}

void Lithophane::addHangers()
{
    if(noOfHangers < 1) return;

    float w = width;
    float h = totalHeight;

    float xDelta = (w / noOfHangers) / 2.0f;
    float x = xDelta - 4.5f; // 4.5 is half the width of a hanger

    constexpr float hangerWidth = 2.0f;

    for (uint8_t a = 0; a < noOfHangers; a++)
    {
        addTriangle({x + 3, h, minThicknessInv}, {x, h, minThicknessInv}, {x + 3, h + 3, minThicknessInv}); 
        addTriangle({x + 3, h + 3, minThicknessInv}, {x + 6, h + 3, minThicknessInv}, {x + 9, h, minThicknessInv});
        addTriangle({x + 9, h, minThicknessInv}, {x + 6, h, minThicknessInv}, {x + 5, h + 1, minThicknessInv});
        addTriangle({x + 4, h + 1, minThicknessInv}, {x + 3, h, minThicknessInv}, {x + 3, h + 3, minThicknessInv}); 
        addTriangle({x + 3, h + 3, minThicknessInv}, {x + 9, h, minThicknessInv}, {x + 5, h + 1, minThicknessInv}); 
        addTriangle({x + 3, h + 3, minThicknessInv}, {x + 5, h + 1, minThicknessInv}, {x + 4, h + 1, minThicknessInv}); 
        addTriangle({x + 3, h + 3, hangerWidth - minThickness}, {x, h, hangerWidth - minThickness}, {x + 3, h, hangerWidth - minThickness}); 
        addTriangle({x + 3, h + 3, hangerWidth - minThickness}, {x + 3, h, hangerWidth - minThickness}, {x + 4, h + 1, hangerWidth - minThickness});         addTriangle({x + 9, h, hangerWidth - minThickness}, {x + 6, h + 3, hangerWidth - minThickness}, {x + 3, h + 3, hangerWidth - minThickness}); 
        addTriangle({x + 5, h + 1, hangerWidth - minThickness}, {x + 6, h, hangerWidth - minThickness}, {x + 9, h, hangerWidth - minThickness}); 
        addTriangle({x + 3, h + 3, hangerWidth - minThickness}, {x + 4, h + 1, hangerWidth - minThickness}, {x + 5, h + 1, hangerWidth - minThickness}); 
        addTriangle({x + 5, h + 1, hangerWidth - minThickness}, {x + 9, h, hangerWidth - minThickness}, {x + 3, h + 3, hangerWidth - minThickness}); 
        addTriangle({x + 5, h + 1, minThicknessInv}, {x + 6, h, minThicknessInv}, {x + 6, h, hangerWidth - minThickness}); 
        addTriangle({x + 5, h + 1, minThicknessInv}, {x + 6, h, hangerWidth - minThickness}, {x + 5, h + 1, hangerWidth - minThickness}); 
        addTriangle({x + 9, h, minThicknessInv}, {x + 6, h + 3, minThicknessInv}, {x + 6, h + 3, hangerWidth - minThickness}); 
        addTriangle({x + 9, h, minThicknessInv}, {x + 6, h + 3, hangerWidth - minThickness}, {x + 9, h, hangerWidth - minThickness}); 
        addTriangle({x + 3, h + 3, minThicknessInv}, {x, h, minThicknessInv}, {x, h, hangerWidth - minThickness}); 
        addTriangle({x + 3, h + 3, minThicknessInv}, {x, h, hangerWidth - minThickness}, {x + 3, h + 3, hangerWidth - minThickness}); 
        addTriangle({x, h, minThicknessInv}, {x + 3, h, minThicknessInv}, {x + 3, h, hangerWidth - minThickness}); 
        addTriangle({x, h, minThicknessInv}, {x + 3, h, hangerWidth - minThickness}, {x, h, hangerWidth - minThickness}); 
        addTriangle({x + 4, h + 1, minThicknessInv}, {x + 5, h + 1, minThicknessInv}, {x + 5, h + 1, hangerWidth - minThickness}); 
        addTriangle({x + 4, h + 1, minThicknessInv}, {x + 5, h + 1, hangerWidth - minThickness}, {x + 4, h + 1, hangerWidth - minThickness}); 
        addTriangle({x + 6, h, minThicknessInv}, {x + 9, h, minThicknessInv}, {x + 9, h, hangerWidth - minThickness}); 
        addTriangle({x + 6, h, minThicknessInv}, {x + 9, h, hangerWidth - minThickness}, {x + 6, h, hangerWidth - minThickness}); 
        addTriangle({x + 6, h + 3, minThicknessInv}, {x + 3, h + 3, minThicknessInv}, {x + 3, h + 3, hangerWidth - minThickness}); 
        addTriangle({x + 6, h + 3, minThicknessInv}, {x + 3, h + 3, hangerWidth - minThickness}, {x + 6, h + 3, hangerWidth - minThickness}); 
        addTriangle({x + 3, h, minThicknessInv}, {x + 4, h + 1, minThicknessInv}, {x + 4, h + 1, hangerWidth - minThickness}); 
        addTriangle({x + 3, h, minThicknessInv}, {x + 4, h + 1, hangerWidth - minThickness}, {x + 3, h, hangerWidth - minThickness}); 

        // Move over to the next hanger placement
        x += xDelta * 2;
    }
}

void Lithophane::addStabilizers()
{
    constexpr float stabSeparation = 0.8f;
    constexpr float stabWidth = 0.8f;
    constexpr float brimHeight = 0.2f;
    constexpr float unionDist = 15.0f;
    constexpr float unionHeight = 1.2f;
    
    const float brimWidth = std::max(10.0f, 0.1f * width);
    const float w = std::max(12.0f, 0.25f * width);
    const float h = totalHeight * stabilizerHeightFactor;
    const int noOfUnions = (int)(h / unionDist) + 1;
    
    float zp = totalThickness - minThickness;
    float zn = minThicknessInv;

    // Left stabilizer
    float x = 0.0f - stabSeparation;
    addTriangle({x, h, zp}, {x, 0.0f,  w}, {x, 0.0f, zp});
    addTriangle({x, h, zn}, {x, 0.0f, zn}, {x, 0.0f, -w});
    addQuad(
        {x, h   , zn},
        {x, h   , zp},
        {x, 0.0f, zp},
        {x, 0.0f, zn}
    );
    x -= stabWidth;
    addTriangle({x, h, zp}, {x, 0.0f, zp}, {x, 0.0f,  w});
    addTriangle({x, h, zn}, {x, 0.0f, -w}, {x, 0.0f, zn});
    addQuad(
        {x, h   , zp},
        {x, h   , zn},
        {x, 0.0f, zn},
        {x, 0.0f, zp}
    );
    addQuad( // bottom
        {x + stabWidth, 0.0f,  w},
        {x            , 0.0f,  w},
        {x            , 0.0f, -w},
        {x + stabWidth, 0.0f, -w}
    );
    addQuad( // top
        {x + stabWidth, h, zn},
        {x            , h, zn},
        {x            , h, zp},
        {x + stabWidth, h, zp}
    );
    addQuad( // front
        {x + stabWidth, h   , zp},
        {x            , h   , zp},
        {x            , 0.0f, w },
        {x + stabWidth, 0.0f, w}
    );
    addQuad( // back
        {x            , h   , zn},
        {x + stabWidth, h   , zn},
        {x + stabWidth, 0.0f, -w},
        {x            , 0.0f, -w}
    );

    // // Right Stabilizer
    x = (float) width + stabSeparation;
    addTriangle({x, h, zp}, {x, 0.0f, zp}, {x, 0.0f,  w});
    addTriangle({x, h, zn}, {x, 0.0f, -w}, {x, 0.0f, zn});
    addQuad(
        {x, h   , zp},
        {x, h   , zn},
        {x, 0.0f, zn},
        {x, 0.0f, zp}
    );
    x += stabWidth;
    addTriangle({x, h, zp}, {x, 0.0f,  w}, {x, 0.0f, zp});
    addTriangle({x, h, zn}, {x, 0.0f, zn}, {x, 0.0f, -w});
    addQuad(
        {x, h   , zn},
        {x, h   , zp},
        {x, 0.0f, zp},
        {x, 0.0f, zn}
    );
    addQuad( // bottom
        {x            , 0.0f,  w},
        {x - stabWidth, 0.0f,  w},
        {x - stabWidth, 0.0f, -w},
        {x            , 0.0f, -w}
    );
    addQuad( // top
        {x            , h, zn},
        {x - stabWidth, h, zn},
        {x - stabWidth, h, zp},
        {x            , h, zp}
    );
    addQuad( // front
        {x            , h   , zp},
        {x - stabWidth, h   , zp},
        {x - stabWidth, 0.0f, w },
        {x            , 0.0f, w }
    );
    addQuad( // back
        {x - stabWidth, h   , zn},
        {x            , h   , zn},
        {x            , 0.0f, -w},
        {x - stabWidth, 0.0f, -w}
    );

    // Unions
    float y = h;
    const float zp2 = zp * 0.3;
    const float zn2 = zn * 0.3f;
    const QVector3D unionSize = {stabSeparation, unionHeight, zp2 - zn2};
    for(int n = 0; n < noOfUnions; n++)
    {
        float yb = y - unionHeight;

        // Left
        x = 0.0f;
        addCube(
            {x - stabSeparation, yb, zp2},
            unionSize
        );

        // Right
        x = width;
        addCube(
            {x, yb, zp2},
            unionSize
        );

        y -= unionDist;
    }

    // Brim right/front
    const QVector3D brimSize = {brimWidth * 2.0f, brimHeight, w - zp - stabSeparation + brimWidth};
    x = width;
    addCube(
        {x + stabSeparation - brimWidth, 0.0f, w + brimWidth},
        brimSize
    );

    // Brim right/back
    addCube(
        {x + stabSeparation - brimWidth, 0.0f, zn - stabSeparation},
        brimSize
    );

    // Brim right/middle
    addCube(
        {x, 0.0f, zp + stabSeparation},
        {stabSeparation, brimHeight, (zp - zn) + stabSeparation * 2.0f}
    );

    // Brim left/front
    x = 0;
    addCube(
        {x - stabSeparation - brimWidth, 0.0f, w + brimWidth},
        brimSize
    );

    // Brim left/back
    addCube(
        {x - stabSeparation - brimWidth, 0.0f, zn - stabSeparation},
        brimSize
    );

    // Brim left/middle
    addCube(
        {x - stabSeparation, 0.0f, zp + stabSeparation},
        {stabSeparation, brimHeight, (zp - zn) + stabSeparation * 2.0f}
    );

    // MIDDLE - front
    const float mw = brimWidth * 0.33f;
    addCube(
        {0.0f, 0.0f, zp + stabSeparation + mw},
        {width, brimHeight, mw}
    );
    // MIDDLE - back
    addCube(
        {0.0f, 0.0f, zn - stabSeparation},
        {width, brimHeight, mw}
    );
    // MIDDLE - unions
    constexpr float unionsLateralGaps = 2.0f;
    constexpr uint8_t bottomBrimUnions = 5;
    assert(bottomBrimUnions > 2);
    const float bus = ((float) width - (2.0f * unionsLateralGaps) - (zp2 - zn2)) / (float) (bottomBrimUnions - 1);
    float bux = unionsLateralGaps;
    for(uint8_t i = 0; i < bottomBrimUnions; i++)
    {
        addCube(
            {bux, 0.0f, zp + stabSeparation},
            {zp2 - zn2, brimHeight, stabSeparation}
        );
        addCube(
            {bux, 0.0f, zn},
            {zp2 - zn2, brimHeight, stabSeparation}
        );
        bux += bus;
    }
}
