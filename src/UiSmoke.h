#pragma once
#include <QStringList>
class QGuiApplication;
class QQuickWindow;
class QMediaPlayer;
class VehicleState;
void runUiSmoke(QGuiApplication&,QQuickWindow&,VehicleState&,QMediaPlayer&,QStringList&,bool native);
