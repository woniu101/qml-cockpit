#include <QGuiApplication>
#include <QSvgRenderer>
#include <QPainter>
#include <QImage>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>

int main(int argc,char **argv){
    qputenv("QT_QPA_PLATFORM","offscreen");qputenv("QT_FORCE_STDERR_LOGGING","1");QGuiApplication app(argc,argv);const auto args=app.arguments();if(args.size()!=2)return 2;
    // Offscreen uses the basic font database rather than Windows font discovery.
    for(const auto &name:{"msyh.ttc","msyhbd.ttc","segoeui.ttf"}){
        if(QFontDatabase::addApplicationFont("C:/Windows/Fonts/"+QString(name))<0){qCritical()<<"Missing render font"<<name;return 6;}
    }
    app.setFont(QFont("Microsoft YaHei UI",12));
    const QDir dir(args[1]);const auto names=dir.entryList({"*.svg"},QDir::Files,QDir::Name);if(names.isEmpty())return 2;
    QImage contact(1640,((names.size()+1)/2)*500+40,QImage::Format_ARGB32_Premultiplied);contact.fill(QColor("#e8ede7"));QPainter sheet(&contact);sheet.setFont(QFont("Microsoft YaHei UI",12));
    int index=0;
    for(const auto &name:names){
        const QString stem=QFileInfo(name).completeBaseName();QSvgRenderer svg(dir.filePath(name));if(!svg.isValid())return 3;
        for(const auto scale:{1.0,1.25,0.7}){
            QImage image(QSize(qRound(1600*scale),qRound(900*scale)),QImage::Format_ARGB32_Premultiplied);image.fill(Qt::transparent);QPainter p(&image);p.setRenderHint(QPainter::Antialiasing);svg.render(&p);p.end();
            const auto suffix=scale==1.0?"":scale==1.25?"@125":"@min";if(!image.save(dir.filePath(stem+suffix+".png")))return 4;
            if(scale==1.0){const int x=20+(index%2)*810,y=20+(index/2)*500;sheet.drawImage(QRect(x,y,800,450),image);sheet.setPen(QColor("#23332f"));sheet.drawText(x+8,y+478,stem);}
        }
        ++index;qInfo().noquote()<<stem;
    }
    sheet.end();return contact.save(dir.filePath("../overview.png"))?0:5;
}
