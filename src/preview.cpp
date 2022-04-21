#include "preview.h"

#include <iostream>

#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QForwardRenderer>
#include <QQuaternion>
#include <QtCore>
#include <Qt3DCore/QNode>
#include <Qt3DRender/QRenderSettings>
#include <Qt3DCore/QComponent>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QMaterial>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QDiffuseSpecularMaterial>
#include <Qt3DExtras/QPerVertexColorMaterial>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DRender/QMesh>
#include <Qt3DRender/QSceneLoader>
#include <Qt3DRender/QPointLight>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DRender/QMaterial>
#include <Qt3DRender/QGeometry> 
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>

#include <QCoreApplication>
#include <QEventLoop>
#include <QTime>

const QColor FrontColor = QColor(QRgb(0xa7d6ff));
const QColor BackColor = QColor(QRgb(0xffc7a7));
const QColor LightColor = QColor(QRgb(0xffffff));
const QColor XColor = QColor(QRgb(0xff0000));
const QColor YColor = QColor(QRgb(0x00ff00));
const QColor ZColor = QColor(QRgb(0x0000ff));

Preview::Preview(QWidget *parent)
   : QWidget(parent)
{
    auto view = new Qt3DExtras::Qt3DWindow();

    // create a container for Qt3DWindow
    container = createWindowContainer(view,this);
    view->defaultFrameGraph()->setClearColor(BackColor);

    Qt3DRender::QRenderSettings *renderSettings = view->renderSettings();        
    renderSettings->setRenderPolicy(Qt3DRender::QRenderSettings::Always);

    // Camera
    camera = view->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(0, 20.0f, 90.0f));
    camera->setUpVector(QVector3D(0, 1, 0));
    camera->setViewCenter(QVector3D(0, 20.0f, 0));

    // Root entity
    rootEntity = new Qt3DCore::QEntity();

    // Light
    Qt3DCore::QEntity *lightEntity = new Qt3DCore::QEntity(rootEntity);
    Qt3DRender::QPointLight *light = new Qt3DRender::QPointLight(lightEntity);
    light->setColor(LightColor);
    light->setIntensity(0.5f);
    lightEntity->addComponent(light);
    Qt3DCore::QTransform *lightTransform = new Qt3DCore::QTransform(lightEntity);
    lightTransform->setTranslation(camera->position());
    lightEntity->addComponent(lightTransform);

    auto *camController = new Qt3DExtras::QOrbitCameraController(rootEntity);
    camController->setCamera(camera);

    // Axes
    createAxe({-100.0f, 0.0f, 0.0f}, {100.0f, 0.0f, 0.0f}, XColor);
    createAxe({0.0f, -100.0f, 0.0f}, {0.0f, 100.0f, 0.0f}, YColor);
    createAxe({0.0f, 0.0f, -100.0f}, {0.0f, 0.0f, 100.0f}, ZColor);

    // Set root object of the scene
    view->setRootEntity(rootEntity);
}

void Preview::createAxe(const QVector3D& p1, const QVector3D& p2, const QColor& color)
{
    QByteArray vertexBufferData;
    vertexBufferData.resize((2 * sizeof(QVector3D)));
    float *rawVertexArray = reinterpret_cast<float *>(vertexBufferData.data());
    rawVertexArray[0] = p1.x();
    rawVertexArray[1] = p1.y();
    rawVertexArray[2] = p1.z();
    rawVertexArray[3] = p2.x();
    rawVertexArray[4] = p2.y();
    rawVertexArray[5] = p2.z();

    Qt3DCore::QEntity *axesEntity = new Qt3DCore::QEntity(rootEntity);

    Qt3DRender::QBuffer *vertexBuffer = new Qt3DRender::QBuffer(axesEntity);
    vertexBuffer->setData(vertexBufferData);

    const uint32_t stride = 3 * sizeof(float);
    Qt3DRender::QAttribute *positionAttribute = new Qt3DRender::QAttribute(axesEntity);
    positionAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    positionAttribute->setBuffer(vertexBuffer);
    positionAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
    positionAttribute->setVertexSize(3);
    positionAttribute->setByteOffset(0);
    positionAttribute->setByteStride(stride);
    positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());

    Qt3DRender::QGeometry *axesGeometry = new Qt3DRender::QGeometry;
    axesGeometry->addAttribute(positionAttribute);
    
    Qt3DRender::QGeometryRenderer *axesRenderer = new Qt3DRender::QGeometryRenderer;
    axesRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);
    axesRenderer->setGeometry(axesGeometry);
    axesRenderer->setVertexCount(2);

    Qt3DExtras::QDiffuseSpecularMaterial *material = new Qt3DExtras::QDiffuseSpecularMaterial;
    material->setAmbient(color);

    axesEntity->addComponent(axesRenderer);
    axesEntity->addComponent(material);
}

void Preview::loadStl(const QString& path)
{
    if(lithophaneEntity != nullptr) lithophaneEntity->deleteLater();
    // Lithophane Entity
    lithophaneEntity = new Qt3DCore::QEntity(rootEntity);
 
    // Lithophane Mesh
    auto* sceneLoader = new Qt3DRender::QSceneLoader(lithophaneEntity);
    lithophaneEntity->addComponent(sceneLoader);

    const QColor color = FrontColor; 
    connect(sceneLoader, &Qt3DRender::QSceneLoader::statusChanged, [sceneLoader, color](Qt3DRender::QSceneLoader::Status status) {
        if(status == Qt3DRender::QSceneLoader::Ready)
        {
            for(const QString& name : sceneLoader->entityNames())
            {
                Qt3DCore::QEntity* entity = sceneLoader->entity(name);

                for(auto *material : entity->componentsOfType<Qt3DRender::QMaterial>())
                    entity->removeComponent(material);

                Qt3DExtras::QDiffuseSpecularMaterial *lithophaneMaterial = new Qt3DExtras::QDiffuseSpecularMaterial;
                lithophaneMaterial->setAmbient(FrontColor);
                entity->addComponent(lithophaneMaterial);
            }

        }
    });

    sceneLoader->setSource(QUrl::fromLocalFile(path));  // fileUrl is input
}

void Preview::loadData(const QList<QVector3D>& vertices)
{
    const uint32_t noOfVertices = vertices.count();
    QByteArray vertexBufferData;
    vertexBufferData.resize(vertices.size() * sizeof(QVector3D) * 2);
    float *rawVertexArray = reinterpret_cast<float *>(vertexBufferData.data());

    uint32_t i = 0;
    for(uint32_t idx = 0; idx < noOfVertices; idx += 3)
    {
        const QVector3D &v1 = vertices.at(idx);
        const QVector3D &v2 = vertices.at(idx + 1);
        const QVector3D &v3 = vertices.at(idx + 2);
        const QVector3D &normal = QVector3D::normal(v1, v2, v3);

        rawVertexArray[i++] = v1.x();
        rawVertexArray[i++] = v1.y();
        rawVertexArray[i++] = v1.z();
        rawVertexArray[i++] = normal.x();
        rawVertexArray[i++] = normal.y();
        rawVertexArray[i++] = normal.z();

        rawVertexArray[i++] = v2.x();
        rawVertexArray[i++] = v2.y();
        rawVertexArray[i++] = v2.z();
        rawVertexArray[i++] = normal.x();
        rawVertexArray[i++] = normal.y();
        rawVertexArray[i++] = normal.z();

        rawVertexArray[i++] = v3.x();
        rawVertexArray[i++] = v3.y();
        rawVertexArray[i++] = v3.z();
        rawVertexArray[i++] = normal.x();
        rawVertexArray[i++] = normal.y();
        rawVertexArray[i++] = normal.z();
    }

    // Lithophane Entity
    if(lithophaneEntity != nullptr) lithophaneEntity->deleteLater();
    lithophaneEntity = new Qt3DCore::QEntity(rootEntity);

    Qt3DRender::QBuffer *vertexBuffer = new Qt3DRender::QBuffer(lithophaneEntity);
    vertexBuffer->setData(vertexBufferData);

    const uint32_t stride = (3 + 3) * sizeof(float);
    Qt3DRender::QAttribute *positionAttribute = new Qt3DRender::QAttribute(lithophaneEntity);
    positionAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    positionAttribute->setBuffer(vertexBuffer);
    positionAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
    positionAttribute->setVertexSize(3);
    positionAttribute->setByteOffset(0);
    positionAttribute->setByteStride(stride);
    positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());

    Qt3DRender::QAttribute *normalAttribute = new Qt3DRender::QAttribute(lithophaneEntity);
    normalAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    normalAttribute->setBuffer(vertexBuffer);
    normalAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
    normalAttribute->setVertexSize(3);
    normalAttribute->setByteOffset(3 * sizeof(float));
    normalAttribute->setByteStride(stride);
    normalAttribute->setName(Qt3DRender::QAttribute::defaultNormalAttributeName());

    Qt3DRender::QGeometry *geometry = new Qt3DRender::QGeometry(lithophaneEntity);
    geometry->addAttribute(positionAttribute);
    geometry->addAttribute(normalAttribute);

    Qt3DRender::QGeometryRenderer *renderer = new Qt3DRender::QGeometryRenderer(lithophaneEntity);
    renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
    renderer->setGeometry(geometry);
    renderer->setVertexCount(noOfVertices);
    renderer->setPrimitiveRestartEnabled(true);
    renderer->setRestartIndexValue(0);

    Qt3DExtras::QDiffuseSpecularMaterial *lithophaneMaterial = new Qt3DExtras::QDiffuseSpecularMaterial;
    lithophaneMaterial->setAmbient(FrontColor);

    // make entity
    lithophaneEntity->addComponent(renderer);
    lithophaneEntity->addComponent(lithophaneMaterial);
    
}