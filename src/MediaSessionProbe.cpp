#include <QCoreApplication>
#include "AvrcpTransportProbe.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#ifdef Q_OS_WIN
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>
#include <chrono>
#endif
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);QJsonArray sessions;QString error;
#ifdef Q_OS_WIN
    using namespace winrt::Windows::Media::Control;using namespace std::chrono_literals;
    try{
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        auto request=GlobalSystemMediaTransportControlsSessionManager::RequestAsync();
        if(request.wait_for(4s)!=winrt::Windows::Foundation::AsyncStatus::Completed){request.Cancel();error="Media session query timed out";}
        else{
            const auto manager=request.GetResults();for(const auto &session:manager.GetSessions()){
                QJsonObject row{{"sourceApp",QString::fromWCharArray(session.SourceAppUserModelId().c_str())}};
                auto metadata=session.TryGetMediaPropertiesAsync();
                if(metadata.wait_for(3s)==winrt::Windows::Foundation::AsyncStatus::Completed){const auto value=metadata.GetResults();row["title"]=QString::fromWCharArray(value.Title().c_str());row["artist"]=QString::fromWCharArray(value.Artist().c_str());row["hasThumbnail"]=bool(value.Thumbnail());}
                else{metadata.Cancel();row["metadataError"]="timeout";}
                const auto timeline=session.GetTimelineProperties();row["positionMs"]=double(timeline.Position().count()/10000);row["durationMs"]=double((timeline.EndTime()-timeline.StartTime()).count()/10000);
                const auto playback=session.GetPlaybackInfo();row["playbackStatus"]=int(playback.PlaybackStatus());row["canPlay"]=playback.Controls().IsPlayEnabled();row["canPause"]=playback.Controls().IsPauseEnabled();row["canSkipNext"]=playback.Controls().IsNextEnabled();
                // SourceAppUserModelId identifies an app, not the paired Bluetooth device.
                row["phoneIdentityVerified"]=false;sessions.append(row);
            }
        }
    }catch(const winrt::hresult_error &e){error=QString("Windows 0x%1").arg(quint32(e.code().value),8,16,QChar('0'));}
#else
    error="Windows only";
#endif
    QDir().mkpath("artifacts/music");QFile file("artifacts/music/phone-sessions.json");if(!file.open(QIODevice::WriteOnly))return 2;
    file.write(QJsonDocument(QJsonObject{{"capturedAt",QDateTime::currentDateTime().toString(Qt::ISODate)},{"readOnly",true},{"sessions",sessions},{"error",error},{"automaticPhoneBinding",false},{"localAvrcpTransport",probeLocalAvrcpTransport()},{"reason","No Bluetooth-device identity is exposed by GSMTC; never bind GetCurrentSession() to a phone by assumption"}}).toJson());return error.isEmpty()?0:1;
}
