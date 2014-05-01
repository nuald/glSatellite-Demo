#pragma once

#include "GlobeRenderer.h"
#include "MessageQueue.h"
#include "helper/gestureDetector.h"
#include "helper/tapCamera.h"
#include "NDKHelper.h"

class Engine {
    GlobeRenderer renderer_;

    ndk_helper::GLContext *gl_context_;

    bool initialized_resources_;
    bool has_focus_;
    bool no_error_;
    float zoom_distance_;

    // Gesture detectors
    ndk_helper::DoubletapDetector doubletap_detector_;
    ndk_helper::PinchDetector pinch_detector_;
    ndk_helper::DragDetector drag_detector_;
    helper::TapDetector tap_detector_;

    ndk_helper::PerfMonitor monitor_;
    helper::TapCamera tap_camera_;

    android_app *app_;

    ASensorManager *sensor_manager_;
    const ASensor *accelerometer_sensor_;
    ASensorEventQueue *sensor_event_queue_;

    void UpdateFPS(float fFPS);
    void ShowUI();
    void ShowError(const char *error);
    void ShowAds();
    void UseTle(char *path);
    void ShowBeam(int num);
    void TransformPosition(ndk_helper::Vec2 &vec);

public:
    static void HandleCmd(android_app *app, int32_t cmd);
    static int32_t HandleInput(android_app *app, AInputEvent *event);
    void HandleMessage(Message msg);

    Engine();
    ~Engine();

    void TermDisplay() {
        gl_context_->Suspend();
    }
    bool IsReady() {
        return has_focus_ && no_error_;
    }

    void LoadResources();
    void UnloadResources();
    void SetState(android_app *state);
    void InitDisplay();
    void DrawFrame();
    void InitWindow();
    void InitSensors();
    void ProcessSensors(int32_t id);
    void SuspendSensors();
    void ResumeSensors();
    void TrimMemory();
    void UpdateZoom(ndk_helper::Vec2 v1, ndk_helper::Vec2 v2);
    bool IsZoomEnabled(ndk_helper::Vec2 v1, ndk_helper::Vec2 v2);
};
