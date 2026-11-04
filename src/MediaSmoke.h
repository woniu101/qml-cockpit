#pragma once
#include <QStringList>
class QGuiApplication;
class QQuickWindow;
class VehicleState;
class MediaState;
class MediaSession;
class RadioService;
class VideoLibrary;
void runMediaSmoke(QGuiApplication &, QQuickWindow &, VehicleState &, MediaState &, MediaSession &,
                   RadioService &, VideoLibrary &, QStringList &);
