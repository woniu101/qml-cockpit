#pragma once
#include <QStringList>
class QGuiApplication;
class QQuickWindow;
class QMediaPlayer;
class VehicleState;class MediaState;
void runUiSmoke(QGuiApplication&,QQuickWindow&,VehicleState&,MediaState&,QMediaPlayer&,QStringList&,bool native);
