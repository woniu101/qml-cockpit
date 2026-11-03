#include "VehicleState.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QTime>
#include <algorithm>
#include <cmath>

void VehicleState::loadPreferences() {
    m_preferencesEnabled=!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS");
    m_settingsPath=qEnvironmentVariable("CAR_TERMINAL_SETTINGS_FILE");
    if(m_settingsPath.isEmpty())m_settingsPath=QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)+"/preferences.ini";
    if(!m_preferencesEnabled)return;
    QSettings prefs(m_settingsPath,QSettings::IniFormat);
    auto temp=[&](const char *name){const double value=prefs.value(name,22).toDouble();return std::isfinite(value)?std::clamp(std::round(value*2)/2,16.0,30.0):22.0;};
    m_driver=temp("driver");m_passenger=temp("passenger");m_volume=std::clamp(prefs.value("volume",45).toInt(),0,100);
    m_fan=std::clamp(prefs.value("fan",3).toInt(),0,7);m_ac=prefs.value("ac",true).toBool();m_auto=prefs.value("automatic",true).toBool();
    m_sync=prefs.value("sync",false).toBool();if(m_sync)m_passenger=m_driver;
    m_heat=std::clamp(prefs.value("heat",0).toInt(),0,3);m_chargeTarget=std::clamp(prefs.value("chargeTarget",90).toInt(),50,100);
    const auto air=prefs.value("airDirection","face").toString();if(QStringList{"face","feet","mixed"}.contains(air))m_airDirection=air;
    const auto theme=prefs.value("theme","day").toString();if(QStringList{"day","night","auto"}.contains(theme))m_themeMode=theme;
    const auto light=prefs.value("lights","auto").toString();if(QStringList{"off","on","auto"}.contains(light))m_lightMode=light;
    m_autoFocus=prefs.value("autoFocus",true).toBool();m_quality=prefs.value("quality","standard").toString()=="high"?"high":"standard";
    const auto mode=prefs.value("playMode","all").toString();if(QStringList{"all","one","shuffle"}.contains(mode))m_playMode=mode;
    refreshTheme();
}
void VehicleState::setDemoActive(bool value) {
    if(value==m_demoActive)return;
    if(value){savePreferences();m_saveTimer.stop();m_demoActive=true;}
    else {m_demoActive=false;schedulePreferences();}
}
void VehicleState::schedulePreferences() {if(m_preferencesEnabled&&!m_demoActive)m_saveTimer.start();}
void VehicleState::savePreferences() {
    if(!m_preferencesEnabled||m_demoActive)return;
    QDir().mkpath(QFileInfo(m_settingsPath).absolutePath());QSettings prefs(m_settingsPath,QSettings::IniFormat);
    const QVariantMap values{{"driver",m_driver},{"passenger",m_passenger},{"volume",m_volume},{"fan",m_fan},{"ac",m_ac},{"automatic",m_auto},{"sync",m_sync},{"heat",m_heat},{"theme",m_themeMode},{"lights",m_lightMode},{"autoFocus",m_autoFocus},{"quality",m_quality},{"chargeTarget",m_chargeTarget},{"playMode",m_playMode}};
    for(auto it=values.cbegin();it!=values.cend();++it)prefs.setValue(it.key(),it.value());
    if(externalMode()&&!m_localBeforeExternal.isEmpty())prefs.setValue("chargeTarget",m_localBeforeExternal["chargeTarget"]);
    if(systemMode()&&!m_energyBeforeSystem.isEmpty())prefs.setValue("chargeTarget",m_energyBeforeSystem["chargeTarget"]);
    // Transient defrost and scene overrides must not become startup defaults.
    const bool normal=m_scene=="comfort";
    const bool defrost=normal?m_defrost:m_savedDefrost;
    prefs.setValue("driver",normal?m_driver:m_savedDriver);prefs.setValue("passenger",normal?m_passenger:m_savedPassenger);
    prefs.setValue("fan",defrost?(normal?m_defrostFan:m_savedDefrostFan):(normal?m_fan:m_savedFan));
    prefs.setValue("ac",defrost?(normal?m_defrostAc:m_savedDefrostAc):(normal?m_ac:m_savedAc));
    prefs.setValue("automatic",defrost?(normal?m_defrostAuto:m_savedDefrostAuto):(normal?m_auto:m_savedAuto));
    prefs.setValue("sync",normal?m_sync:m_savedSync);prefs.setValue("heat",normal?m_heat:m_savedHeat);
    prefs.setValue("airDirection",normal?m_airDirection:m_savedAirDirection);prefs.sync();
}
void VehicleState::refreshTheme() {
    const int hour=QTime::currentTime().hour();const bool night=m_themeMode=="night"||(m_themeMode=="auto"&&(hour>=18||hour<6));
    if(night!=m_night){m_night=night;emit changed();}
}
void VehicleState::setThemeMode(const QString &value) {if(!QStringList{"day","night","auto"}.contains(value)||value==m_themeMode)return;m_themeMode=value;refreshTheme();emit changed();schedulePreferences();}
void VehicleState::setNight(bool value) {setThemeMode(value?"night":"day");}
void VehicleState::setAutoFocus(bool value) {if(value==m_autoFocus)return;m_autoFocus=value;emit changed();schedulePreferences();}
void VehicleState::setQuality(const QString &value) {if(!QStringList{"standard","high"}.contains(value)||value==m_quality)return;m_quality=value;emit changed();schedulePreferences();}
QVariantMap VehicleState::snapshot() const {
    return {{"driver",m_driver},{"passenger",m_passenger},{"fan",m_fan},{"ac",m_ac},{"auto",m_auto},{"sync",m_sync},{"defrost",m_defrost},{"heat",m_heat},{"air",m_airDirection},
        {"locked",m_locked},{"doors",doors()},{"hatch",m_hatch},{"hood",m_hood},{"port",m_port},{"mirrors",m_mirrors},{"windows",windowActual()},
        {"gear",m_gear},{"speed",m_speed},{"tripKm",m_tripKm},{"navigation",m_navigating},{"destination",m_destination},{"progress",m_progress},{"battery",m_battery},
        {"plugged",m_plugged},{"charging",m_charging},{"chargeTarget",m_chargeTarget},{"scene",m_scene},{"theme",m_themeMode},{"lightMode",m_lightMode},{"highBeam",m_highBeam},{"fog",m_fog},
        {"volume",m_volume},{"track",m_track},{"playing",m_playing},{"position",m_position},{"playMode",m_playMode},{"autoFocus",m_autoFocus},{"quality",m_quality},
        {"queue",m_queueUrls},{"queueExplicit",m_explicitQueue},{"queueLabel",m_queueLabel},{"musicHistory",m_musicHistory},{"historyCursor",m_historyCursor},{"shuffleVisited",m_shuffleVisited.values()},
        {"savedDriver",m_savedDriver},{"savedPassenger",m_savedPassenger},{"savedFan",m_savedFan},{"savedAc",m_savedAc},{"savedAuto",m_savedAuto},
        {"savedSync",m_savedSync},{"savedDefrost",m_savedDefrost},{"savedHeat",m_savedHeat},{"savedAir",m_savedAirDirection},{"savedDefrostFan",m_savedDefrostFan},{"savedDefrostAuto",m_savedDefrostAuto},{"savedDefrostAc",m_savedDefrostAc},
        {"defrostFan",m_defrostFan},{"defrostAuto",m_defrostAuto},{"defrostAc",m_defrostAc}};
}
void VehicleState::restoreSnapshot(const QVariantMap &s) {
    if(systemMode()){emit toast(QStringLiteral("电脑映射模式不恢复模拟电量快照"),"warning");return;}
    if(externalMode()){emit toast(QStringLiteral("外部联调时不恢复本地车辆快照"),"warning");return;}
    if(s.isEmpty())return;cancelMotions();emit visualReset();
    m_driver=s["driver"].toDouble();m_passenger=s["passenger"].toDouble();m_fan=s["fan"].toInt();m_ac=s["ac"].toBool();m_auto=s["auto"].toBool();m_sync=s["sync"].toBool();m_defrost=s["defrost"].toBool();m_heat=s["heat"].toInt();
    m_locked=s["locked"].toBool();const auto doors=s["doors"].toList(),windows=s["windows"].toList();
    for(int i=0;i<4;++i){m_doors[i]=doors.value(i).toBool();m_windowActual[i]=std::clamp(windows.value(i).toDouble(),0.0,100.0);m_windowTargets[i]=qRound(m_windowActual[i]);m_windowMessages[i]=QStringLiteral("当前开度 %1%").arg(m_windowTargets[i]);}
    m_hatch=s["hatch"].toBool();m_hood=s["hood"].toBool();m_port=s["port"].toBool();m_mirrors=s["mirrors"].toBool();
    m_gear=s["gear"].toString();m_speed=s["speed"].toInt();m_navigating=s["navigation"].toBool();m_destination=s["destination"].toString();m_route=makeRoute(m_destination);m_progress=s["progress"].toDouble();
    m_tripKm=s["tripKm"].toDouble();m_battery=s["battery"].toDouble();m_plugged=s["plugged"].toBool();m_charging=s["charging"].toBool();m_chargeTarget=s["chargeTarget"].toInt();m_scene=s["scene"].toString();
    m_themeMode=s["theme"].toString();m_lightMode=s["lightMode"].toString();m_highBeam=s["highBeam"].toBool();m_fog=s["fog"].toBool();
    m_volume=s["volume"].toInt();m_track=std::clamp(s["track"].toInt(),0,int(m_tracks.size())-1);m_playing=s["playing"].toBool();m_position=s["position"].toDouble();m_playMode=s["playMode"].toString();m_autoFocus=s["autoFocus"].toBool();m_quality=s["quality"].toString();
    if(s.contains("queue")){m_queueUrls=s["queue"].toStringList();m_explicitQueue=s["queueExplicit"].toBool();m_queueLabel=s["queueLabel"].toString();m_musicHistory=s["musicHistory"].toStringList();m_historyCursor=s["historyCursor"].toInt();const auto visited=s["shuffleVisited"].toStringList();m_shuffleVisited=QSet<QString>(visited.begin(),visited.end());emit queueChanged();}
    m_savedDriver=s["savedDriver"].toDouble();m_savedPassenger=s["savedPassenger"].toDouble();m_savedFan=s["savedFan"].toInt();m_savedAc=s["savedAc"].toBool();m_savedAuto=s["savedAuto"].toBool();
    m_defrostFan=s["defrostFan"].toInt();m_defrostAuto=s["defrostAuto"].toBool();m_defrostAc=s["defrostAc"].toBool();
    m_airDirection=s["air"].toString();m_savedSync=s["savedSync"].toBool();m_savedDefrost=s["savedDefrost"].toBool();m_savedHeat=s["savedHeat"].toInt();m_savedAirDirection=s["savedAir"].toString();
    m_savedDefrostFan=s["savedDefrostFan"].toInt();m_savedDefrostAuto=s["savedDefrostAuto"].toBool();m_savedDefrostAc=s["savedDefrostAc"].toBool();
    refreshTheme();emit changed();emit windowProgressChanged();emit motionsChanged();emit positionChanged();emit seekRequested(qint64(m_position));schedulePreferences();
}
void VehicleState::resetRuntime() {
    if(systemMode()){emit toast(QStringLiteral("请切回内置演示后重置车辆"),"warning");return;}
    if(externalMode()){emit toast(QStringLiteral("请在模拟器中重置车辆"),"warning");return;}
    cancelMotions();emit visualReset();m_doors.fill(false);m_windowActual.fill(0);m_windowTargets.fill(0);
    for(auto &message:m_windowMessages)message=QStringLiteral("拖动滑条，或选择常用开度");
    m_locked=true;m_hatch=m_hood=m_port=m_mirrors=m_plugged=m_charging=m_navigating=m_playing=m_highBeam=m_fog=false;
    m_gear="P";m_speed=0;m_progress=m_position=m_tripKm=0;m_battery=82;m_scene="comfort";m_defrost=false;
    m_destination=QStringLiteral("滨江艺术中心");m_route=makeRoute(m_destination);m_track=0;
    m_explicitQueue=false;m_queueUrls.clear();m_queueLabel=QStringLiteral("全部歌曲");resetMusicHistory();emit queueChanged();
    emit changed();emit motionsChanged();emit windowProgressChanged();emit positionChanged();emit seekRequested(0);
}
void VehicleState::reset() {
    if(systemMode()){emit toast(QStringLiteral("请切回内置演示后恢复初始状态"),"warning");return;}
    if(externalMode()){emit toast(QStringLiteral("请切换内置演示后恢复初始状态"),"warning");return;}
    m_driver=m_passenger=22;m_fan=3;m_volume=45;m_ac=m_auto=true;m_sync=false;m_heat=0;m_themeMode="day";m_lightMode="auto";m_autoFocus=true;m_quality="standard";m_chargeTarget=90;m_playMode="all";m_airDirection="face";
    resetRuntime();refreshTheme();schedulePreferences();emit toast(QStringLiteral("已恢复演示初始状态"));
}
