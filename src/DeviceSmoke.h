#pragma once
#include <QStringList>
#include "BluetoothAudio.h"
class QGuiApplication;class QQuickWindow;class QMediaPlayer;class VehicleState;class AudioDevices;class BluetoothStatus;
BluetoothAudio::Factory deviceReceiverFixture();
void runDeviceSmoke(QGuiApplication&,QQuickWindow&,VehicleState&,QMediaPlayer&,AudioDevices&,BluetoothStatus&,BluetoothAudio&,QStringList&);
