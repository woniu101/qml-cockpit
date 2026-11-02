#pragma once
#include <QQuickImageProvider>
#include <QFile>
#include <QPainter>
#include <QSvgRenderer>

class IconProvider : public QQuickImageProvider {
public:
    IconProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}
    QImage requestImage(const QString &id,QSize *size,const QSize &requested) override {
        const QStringList parts=id.split('/');
        const QString name=parts.value(0);
        if(name.contains("..") || name.contains('\\'))return {};
        QColor color(QStringLiteral("#")+parts.value(1,QStringLiteral("23332f")));
        if(!color.isValid())color=QColor("#23332f");
        QFile file(QStringLiteral(":/ui/icons/%1.svg").arg(name));
        if(!file.open(QIODevice::ReadOnly))return {};
        QByteArray svg=file.readAll();svg.replace("currentColor",color.name().toUtf8());
        QSize target=requested.isValid()?requested:QSize(64,64);
        if(size)*size=QSize(24,24);
        QImage result(target,QImage::Format_ARGB32_Premultiplied);result.fill(Qt::transparent);
        QSvgRenderer renderer(svg);QPainter painter(&result);renderer.render(&painter);
        return result;
    }
};
