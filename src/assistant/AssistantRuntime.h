#pragma once

#include "AssistantAppearance.h"
#include "AssistantMedia.h"
#include "AssistantNlu.h"
#include "AssistantSpeech.h"
#include "AssistantVehicle.h"
#include "AssistantVoice.h"
#include "AssistantWake.h"
#include "AssistantWakeFeedback.h"

class QQmlContext;

class AssistantRuntime final {
public:
    AssistantRuntime(QQmlContext &, VehicleState &, MediaState &, MediaSession &, VideoLibrary &, RadioService &, QAudioOutput &);

private:
    AssistantVehicle m_vehicle;
    AssistantMedia m_media;
    AssistantSpeech m_speech;
    AssistantVoice m_voice;
    AssistantNlu m_nlu;
    AssistantAppearance m_appearance;
    AssistantWake m_wake;
    AssistantWakeFeedback m_wakeFeedback{m_voice};
    QTimer m_wakeSync;
    QTimer m_followUp;
    QTimer m_wakeCapture;
};
