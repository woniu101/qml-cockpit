#pragma once
#include <QtQuick3D/QQuick3DGeometry>
#include <QVector3D>
#include <QtQml/qqmlregistration.h>

class ChargeCableGeometry : public QQuick3DGeometry {
    Q_OBJECT
    QML_ELEMENT
public:
    explicit ChargeCableGeometry(QQuick3DObject *parent=nullptr);
    Q_INVOKABLE QVector3D pointAt(qreal progress) const;
};
