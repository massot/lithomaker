#ifndef __PREVIEW_H__
#define __PREVIEW_H__

#include <QObject>
#include <QWidget>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>


class Preview : public QWidget
{
    Q_OBJECT

private:
    QWidget *container;
    // Root and lithophane entities
    Qt3DCore::QEntity *rootEntity = nullptr, *lithophaneEntity = nullptr;
    Qt3DRender::QCamera *camera = nullptr;

    // Private methods
    void createAxe(const QVector3D& p1, const QVector3D& p2, const QColor& color);

public:
    Preview(QWidget *parent = nullptr);

    void loadStl(const QString& path);
    void loadData(const QList<QVector3D>& vertices);
    void setCameraPosition(const QVector3D& position)
    {
        if(camera) {
            camera->setPosition(position);
            camera->setViewCenter(QVector3D(0, position.y(), 0));
        }
    }

protected:
    void resizeEvent ( QResizeEvent * event )
    {
        resizeView(this->size());
    }

public slots:
    void resizeView(QSize size)
    {
        container->resize(size);
    }

};

#endif // __PREVIEW_H__