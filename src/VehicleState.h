#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QTimer>
#include <QElapsedTimer>
#include <QHash>
#include <QSet>
#include <QPointF>
#include <QtQml/qqmlregistration.h>
#include <array>
#include "VehicleLink.h"
#include "SystemStatus.h"

class VehicleState : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Vehicle)
    QML_SINGLETON
    Q_PROPERTY(bool externalMode READ externalMode WRITE setExternalMode NOTIFY sourceChanged)
    Q_PROPERTY(bool externalReady READ externalReady NOTIFY sourceChanged)
    Q_PROPERTY(QString sourceMode READ sourceMode WRITE setSourceMode NOTIFY sourceChanged)
    Q_PROPERTY(bool systemMode READ systemMode NOTIFY sourceChanged)
    Q_PROPERTY(bool systemFallback READ systemFallback NOTIFY sourceChanged)
    Q_PROPERTY(bool energyReady READ energyReady NOTIFY sourceChanged)
    Q_PROPERTY(bool batteryKnown READ batteryKnown NOTIFY sourceChanged)
    Q_PROPERTY(bool chargeControlAvailable READ chargeControlAvailable NOTIFY sourceChanged)
    Q_PROPERTY(bool chargePowerKnown READ chargePowerKnown NOTIFY sourceChanged)
    Q_PROPERTY(QString energySourceText READ energySourceText NOTIFY sourceChanged)
    Q_PROPERTY(double driverTemperature READ driverTemperature WRITE setDriverTemperature NOTIFY changed)
    Q_PROPERTY(double passengerTemperature READ passengerTemperature WRITE setPassengerTemperature NOTIFY changed)
    Q_PROPERTY(int fan READ fan WRITE setFan NOTIFY changed)
    Q_PROPERTY(bool acOn READ acOn WRITE setAcOn NOTIFY changed)
    Q_PROPERTY(bool automatic READ automatic WRITE setAutomatic NOTIFY changed)
    Q_PROPERTY(bool defrost READ defrost WRITE setDefrost NOTIFY changed)
    Q_PROPERTY(bool climateSync READ climateSync WRITE setClimateSync NOTIFY changed)
    Q_PROPERTY(QString airDirection READ airDirection WRITE setAirDirection NOTIFY changed)
    Q_PROPERTY(bool seatHeat READ seatHeat WRITE setSeatHeat NOTIFY changed)
    Q_PROPERTY(int heatLevel READ heatLevel WRITE setHeatLevel NOTIFY changed)
    Q_PROPERTY(bool locked READ locked NOTIFY changed)
    Q_PROPERTY(bool hatchOpen READ hatchOpen NOTIFY changed)
    Q_PROPERTY(bool hoodOpen READ hoodOpen NOTIFY changed)
    Q_PROPERTY(QVariantList doors READ doors NOTIFY changed)
    Q_PROPERTY(bool portOpen READ portOpen NOTIFY changed)
    Q_PROPERTY(bool mirrorsFolded READ mirrorsFolded NOTIFY changed)
    Q_PROPERTY(bool lightsOn READ lightsOn WRITE setLightsOn NOTIFY changed)
    Q_PROPERTY(QString lightMode READ lightMode WRITE setLightMode NOTIFY changed)
    Q_PROPERTY(bool highBeam READ highBeam WRITE setHighBeam NOTIFY changed)
    Q_PROPERTY(bool fogLights READ fogLights WRITE setFogLights NOTIFY changed)
    Q_PROPERTY(QVariantList windows READ windows NOTIFY windowProgressChanged)
    Q_PROPERTY(QVariantList windowActual READ windowActual NOTIFY windowProgressChanged)
    Q_PROPERTY(QVariantList windowTargets READ windowTargets NOTIFY windowProgressChanged)
    Q_PROPERTY(QVariantList windowMessages READ windowMessages NOTIFY windowProgressChanged)
    Q_PROPERTY(QVariantList windowBusy READ windowBusy NOTIFY windowProgressChanged)
    Q_PROPERTY(QVariantList doorBusy READ doorBusy NOTIFY motionsChanged)
    Q_PROPERTY(bool hatchBusy READ hatchBusy NOTIFY motionsChanged)
    Q_PROPERTY(bool hoodBusy READ hoodBusy NOTIFY motionsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY motionsChanged)
    Q_PROPERTY(QString operationSummary READ operationSummary NOTIFY motionsChanged)
    Q_PROPERTY(QString globalProgress READ globalProgress NOTIFY motionsChanged)
    Q_PROPERTY(QVariantList notifications READ notifications NOTIFY statusChanged)
    Q_PROPERTY(int warningCount READ warningCount NOTIFY statusChanged)
    Q_PROPERTY(QString statusTitle READ statusTitle NOTIFY statusChanged)
    Q_PROPERTY(bool parked READ parked NOTIFY changed)
    Q_PROPERTY(QString gear READ gear WRITE setGear NOTIFY changed)
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY changed)
    Q_PROPERTY(double tripKm READ tripKm NOTIFY changed)
    Q_PROPERTY(bool navigationActive READ navigationActive NOTIFY changed)
    Q_PROPERTY(QString destination READ destination NOTIFY changed)
    Q_PROPERTY(double routeProgress READ routeProgress NOTIFY changed)
    Q_PROPERTY(double remainingKm READ remainingKm NOTIFY changed)
    Q_PROPERTY(int remainingMinutes READ remainingMinutes NOTIFY changed)
    Q_PROPERTY(QVariantList routePoints READ routePoints NOTIFY changed)
    Q_PROPERTY(QPointF routeMarker READ routeMarker NOTIFY changed)
    Q_PROPERTY(double routeHeading READ routeHeading NOTIFY changed)
    Q_PROPERTY(int turnDistance READ turnDistance NOTIFY changed)
    Q_PROPERTY(QString turnType READ turnType NOTIFY changed)
    Q_PROPERTY(QString turnStreet READ turnStreet NOTIFY changed)
    Q_PROPERTY(QString laneHint READ laneHint NOTIFY changed)
    Q_PROPERTY(double batteryPercent READ batteryPercent NOTIFY changed)
    Q_PROPERTY(int rangeKm READ rangeKm NOTIFY changed)
    Q_PROPERTY(bool plugged READ plugged NOTIFY changed)
    Q_PROPERTY(bool charging READ charging NOTIFY changed)
    Q_PROPERTY(QString chargeStatus READ chargeStatus NOTIFY changed)
    Q_PROPERTY(int chargeTarget READ chargeTarget WRITE setChargeTarget NOTIFY changed)
    Q_PROPERTY(double chargePower READ chargePower NOTIFY changed)
    Q_PROPERTY(int chargeMinutes READ chargeMinutes NOTIFY changed)
    Q_PROPERTY(QString scene READ scene NOTIFY changed)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY changed)
    Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY changed)
    Q_PROPERTY(int track READ track NOTIFY changed)
    Q_PROPERTY(QString trackTitle READ trackTitle NOTIFY changed)
    Q_PROPERTY(QString trackUrl READ trackUrl NOTIFY changed)
    Q_PROPERTY(QVariantList tracks READ tracks NOTIFY changed)
    Q_PROPERTY(QString playMode READ playMode WRITE setPlayMode NOTIFY changed)
    Q_PROPERTY(QString mediaStatus READ mediaStatus NOTIFY changed)
    Q_PROPERTY(QString mediaError READ mediaError NOTIFY changed)
    Q_PROPERTY(double position READ position NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY positionChanged)
    Q_PROPERTY(bool night READ night WRITE setNight NOTIFY changed)
    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY changed)
    Q_PROPERTY(bool autoFocus READ autoFocus WRITE setAutoFocus NOTIFY changed)
    Q_PROPERTY(QString quality READ quality WRITE setQuality NOTIFY changed)
    Q_PROPERTY(int doorMotionDuration READ doorMotionDuration CONSTANT)
    Q_PROPERTY(int hatchMotionDuration READ hatchMotionDuration CONSTANT)
    Q_PROPERTY(int hoodMotionDuration READ hoodMotionDuration CONSTANT)
public:
    explicit VehicleState(QObject *parent=nullptr,SystemStatus *system=nullptr);
    ~VehicleState() override;
    bool externalMode() const;
    bool externalReady() const;
    void setExternalMode(bool);
    QString sourceMode() const{return m_sourceMode;}
    bool systemMode() const{return m_sourceMode=="system";}
    bool systemFallback() const{return systemMode()&&m_systemFallback;}
    bool energyReady() const;
    bool batteryKnown() const;
    bool chargeControlAvailable() const{return !systemMode()||m_systemFallback;}
    bool chargePowerKnown() const{return chargeControlAvailable();}
    QString energySourceText() const;
    void setSourceMode(const QString&);
    SystemStatus *systemStatus() const{return m_system;}
    VehicleLink *connection() const{return m_link;}
    double driverTemperature() const { return m_driver; }
    double passengerTemperature() const { return m_passenger; }
    int fan() const { return m_fan; }
    bool acOn() const { return m_ac; }
    bool automatic() const { return m_auto; }
    bool defrost() const { return m_defrost; }
    bool climateSync() const { return m_sync; }
    QString airDirection() const { return m_airDirection; }
    bool seatHeat() const { return m_heat>0; }
    int heatLevel() const { return m_heat; }
    bool locked() const { return m_locked; }
    bool hatchOpen() const { return m_hatch; }
    bool hoodOpen() const { return m_hood; }
    QVariantList doors() const;
    bool portOpen() const { return m_port; }
    bool mirrorsFolded() const { return m_mirrors; }
    bool lightsOn() const { return m_lightMode=="on" || (m_lightMode=="auto" && m_night); }
    QString lightMode() const { return m_lightMode; }
    bool highBeam() const { return m_highBeam && lightsOn(); }
    bool fogLights() const { return m_fog && lightsOn(); }
    QVariantList windows() const;
    QVariantList windowActual() const;
    QVariantList windowTargets() const;
    QVariantList windowMessages() const;
    QVariantList windowBusy() const;
    QVariantList doorBusy() const;
    bool hatchBusy() const { return m_motions.contains("hatch"); }
    bool hoodBusy() const { return m_motions.contains("hood"); }
    bool busy() const;
    QString operationSummary() const;
    QString globalProgress() const;
    QVariantList notifications() const;
    int warningCount() const;
    QString statusTitle() const;
    bool parked() const { return m_gear=="P"; }
    QString gear() const { return m_gear; }
    int speed() const { return m_speed; }
    double tripKm() const { return m_tripKm; }
    bool navigationActive() const { return m_navigating; }
    QString destination() const { return m_destination; }
    double routeProgress() const { return m_progress; }
    double remainingKm() const;
    int remainingMinutes() const;
    QVariantList routePoints() const { return m_route.points; }
    QPointF routeMarker() const;
    double routeHeading() const;
    int turnDistance() const;
    QString turnType() const;
    QString turnStreet() const;
    QString laneHint() const;
    double batteryPercent() const { return m_battery; }
    int rangeKm() const;
    bool plugged() const { return m_plugged; }
    bool charging() const { return m_charging; }
    QString chargeStatus() const;
    int chargeTarget() const { return m_chargeTarget; }
    double chargePower() const { return !chargePowerKnown()?0:externalMode()?m_remoteChargePower:(m_charging?55.0:0); }
    int chargeMinutes() const;
    QString scene() const { return m_scene; }
    int volume() const { return m_volume; }
    bool playing() const { return m_playing; }
    int track() const { return m_track; }
    QString trackTitle() const;
    QString trackUrl() const;
    QVariantList tracks() const { return m_tracks; }
    QString playMode() const { return m_playMode; }
    QString mediaStatus() const { return m_mediaStatus; }
    QString mediaError() const { return m_mediaError; }
    double position() const { return m_position; }
    double duration() const { return m_duration; }
    bool night() const { return m_night; }
    QString themeMode() const { return m_themeMode; }
    bool autoFocus() const { return m_autoFocus; }
    QString quality() const { return m_quality; }
    int doorMotionDuration() const { return 650; }
    int hatchMotionDuration() const { return 900; }
    int hoodMotionDuration() const { return 800; }
    void setDriverTemperature(double);
    void setPassengerTemperature(double);
    void setFan(int);
    void setAcOn(bool);
    void setAutomatic(bool);
    void setDefrost(bool);
    void setClimateSync(bool);
    void setAirDirection(const QString&);
    void setSeatHeat(bool);
    void setHeatLevel(int);
    void setLightsOn(bool);
    void setLightMode(const QString&);
    void setHighBeam(bool);
    void setFogLights(bool);
    void setGear(const QString&);
    void setSpeed(int);
    void setChargeTarget(int);
    void setVolume(int);
    void setPlaying(bool);
    void setPlayMode(const QString&);
    void setNight(bool);
    void setThemeMode(const QString&);
    void setAutoFocus(bool);
    void setQuality(const QString&);
    void updatePosition(double);
    void updateDuration(double);
    void updateMediaStatus(const QString&,const QString &error=QString());
    bool demoActive() const{return m_demoActive;}
    void mergeMusicTracks(const QVariantList&);
    void updateTrackProperties(const QString &url,const QVariantMap &properties);
    void restoreMusicPosition(const QString &url,qint64 position);
    void removeMusicTrack(int index);
    QStringList playQueue() const;
    QString queueLabel() const{return m_queueLabel;}
    bool hasExplicitQueue() const{return m_explicitQueue;}
    void setPlayQueue(const QStringList &urls,const QString &label);
    void replaceMusicTrack(int index,const QVariantMap &row);
    Q_INVOKABLE void moveQueueTrack(const QString &url,int direction);
    Q_INVOKABLE void removeQueueTrack(const QString &url);
    Q_INVOKABLE void setWindow(int index,int opening);
    Q_INVOKABLE void beginWindowDrag(int index);
    Q_INVOKABLE void previewWindow(int index,int opening);
    Q_INVOKABLE void endWindowDrag(int index);
    Q_INVOKABLE void stopWindow(int index);
    Q_INVOKABLE void closeWindows();
    Q_INVOKABLE void ventilateWindows();
    Q_INVOKABLE void toggleLock();
    Q_INVOKABLE void toggleHatch();
    Q_INVOKABLE void toggleHood();
    Q_INVOKABLE void setDoor(int index,bool open);
    Q_INVOKABLE void closeDoors();
    Q_INVOKABLE void togglePort();
    Q_INVOKABLE void toggleMirrors();
    Q_INVOKABLE bool startNavigation(const QString&);
    Q_INVOKABLE QVariantMap routePreview(const QString&) const;
    Q_INVOKABLE void endNavigation();
    Q_INVOKABLE void startDriving();
    Q_INVOKABLE void parkVehicle();
    Q_INVOKABLE void advanceRoute(double seconds);
    Q_INVOKABLE void advanceSimulation(double seconds);
    Q_INVOKABLE void togglePlug();
    Q_INVOKABLE void startCharging();
    Q_INVOKABLE void stopCharging();
    Q_INVOKABLE void setScene(const QString&);
    Q_INVOKABLE void nextTrack(int direction=1);
    Q_INVOKABLE void selectTrack(int index);
    Q_INVOKABLE void finishTrack();
    Q_INVOKABLE void importTracks(const QVariantList& urls);
    Q_INVOKABLE void retryTrack();
    Q_INVOKABLE void seek(double milliseconds);
    Q_INVOKABLE QVariantMap snapshot() const;
    Q_INVOKABLE void restoreSnapshot(const QVariantMap&);
    Q_INVOKABLE void resetRuntime();
    Q_INVOKABLE void setDemoActive(bool);
    Q_INVOKABLE void reset();
signals:
    void sourceChanged();
    void changed();
    void positionChanged();
    void windowProgressChanged();
    void motionsChanged();
    void statusChanged();
    void visualReset();
    void toast(const QString &message,const QString &kind=QStringLiteral("success"));
    void seekRequested(qint64);
    void mediaReloadRequested();
    void queueChanged();
private:
    bool externalCommand(const QString&,const QJsonObject&,const QString&);
    void applyExternal(const QJsonObject&);
    void externalResult(const QString&,const QJsonObject&,const QString&,const QString&);
    void applySystemPower();
    QVariantMap energySnapshot() const;
    void restoreEnergy(const QVariantMap&);
    bool systemEnergyCommand();
    SystemStatus *m_system=nullptr;
    QString m_sourceMode="internal";
    bool m_systemFallback=false,m_systemHadSoc=false;
    QVariantMap m_energyBeforeSystem;
    VehicleLink *m_link=nullptr;
    QVariantMap m_localBeforeExternal;
    std::array<bool,4> m_remoteDragging{};
    std::array<int,4> m_remoteDragTargets{};
    bool m_remoteTripValid=false;
    QString m_remoteSession;
    double m_remoteForward=0;
    double m_remoteChargePower=0;
    struct Motion { quint64 serial; QString label; };
    struct Route { QString name; double km=12; int minutes=22; QVariantList points; QStringList streets,turns; };
    struct Location { QPointF point; double heading=0,remaining=0,total=1; int leg=0; };
    static Route makeRoute(const QString&);
    Location routeLocation() const;
    bool canDrive();
    int musicIndex(const QString &url) const;
    bool musicAvailable(int index) const;
    void activateMusic(int index,bool recordHistory);
    void resetMusicHistory();
    void beginMotion(const QString&,const QString&,const QString&,int);
    void tickWindows();
    void setWindowInternal(int,int,bool);
    void loadPreferences();
    void savePreferences();
    void schedulePreferences();
    void refreshTheme();
    void cancelMotions();
    QHash<QString,Motion> m_motions;
    quint64 m_motionSerial=0;
    QString m_motionCompletion,m_windowBatch;
    QTimer m_windowTimer,m_saveTimer;
    QElapsedTimer m_windowClock;
    std::array<double,4> m_windowActual{0,0,0,0};
    std::array<int,4> m_windowTargets{0,0,0,0};
    std::array<bool,4> m_windowMoving{false,false,false,false},m_windowDragging{false,false,false,false};
    std::array<QString,4> m_windowMessages;
    int m_windowNotifyTicks=0;
    std::array<bool,4> m_doors{false,false,false,false};
    double m_driver=22,m_passenger=22;
    int m_fan=3,m_heat=0,m_volume=45,m_track=0;
    bool m_ac=true,m_auto=true,m_defrost=false,m_sync=false;
    bool m_locked=true,m_hatch=false,m_hood=false,m_port=false,m_mirrors=false,m_highBeam=false,m_fog=false;
    bool m_navigating=false,m_playing=false,m_night=false,m_autoFocus=true;
    QString m_gear="P",m_scene="comfort",m_themeMode="day",m_quality="standard",m_lightMode="auto",m_airDirection="face";
    int m_speed=0;
    QString m_destination=QStringLiteral("滨江艺术中心");
    Route m_route;
    double m_progress=0,m_battery=82,m_tripKm=0;
    bool m_plugged=false,m_charging=false;
    int m_chargeTarget=90;
    QVariantList m_tracks;
    QStringList m_queueUrls,m_musicHistory;
    QSet<QString> m_shuffleVisited;
    QString m_queueLabel=QStringLiteral("全部歌曲");
    int m_historyCursor=-1;
    bool m_explicitQueue=false;
    QString m_playMode="all",m_mediaStatus="loading",m_mediaError;
    double m_position=0,m_duration=64000;
    double m_savedDriver=22,m_savedPassenger=22;
    int m_savedFan=3;
    bool m_savedAc=true,m_savedAuto=true;
    bool m_savedSync=false,m_savedDefrost=false;
    int m_savedHeat=0,m_savedDefrostFan=3;
    bool m_savedDefrostAuto=true,m_savedDefrostAc=true;
    QString m_savedAirDirection="face";
    int m_defrostFan=3;
    bool m_defrostAuto=true,m_defrostAc=true;
    bool m_preferencesEnabled=true;
    bool m_demoActive=false;
    QString m_settingsPath;
};
