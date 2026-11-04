#pragma once
#include <QStringList>
#include "BluetoothAudio.h"
class QGuiApplication;class QQuickWindow;class QMediaPlayer;class MediaState;class AudioDevices;class BluetoothStatus;
BluetoothAudio::Factory deviceReceiverFixture();
void runDeviceSmoke(QGuiApplication&,QQuickWindow&,MediaState&,QMediaPlayer&,AudioDevices&,BluetoothStatus&,BluetoothAudio&,QStringList&);
