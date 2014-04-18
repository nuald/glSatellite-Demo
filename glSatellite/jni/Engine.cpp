#include "Engine.h"
#include "DebugUtils.h"
#include "Satellite.h"
#include "FileReaderFactory.h"
#include "MessageQueue.h"

using namespace ndk_helper;

Engine::Engine() :
            initialized_resources_(false),
            has_focus_(false),
            no_error_(true),
            zoom_distance_(0.f),
            app_(nullptr),
            sensor_manager_(nullptr),
            accelerometer_sensor_(nullptr),
            sensor_event_queue_(nullptr) {
    gl_context_ = ndk_helper::GLContext::GetInstance();
}

Engine::~Engine() {}

void Engine::LoadResources() {
    renderer_.Init();
    renderer_.Bind(&tap_camera_);
    auto reader = FileReaderFactory::Get(APP, "iridium.txt");
    renderer_.InitSatelliteMgr(*reader);
}

void Engine::UnloadResources() {
    renderer_.Unload();
}

/**
 * Initialize an EGL context for the current display.
 */
void Engine::InitDisplay() {
    if (!initialized_resources_) {
        gl_context_->Init(app_->window);
        LoadResources();
        initialized_resources_ = true;
    } else {
        // initialize OpenGL ES and EGL
        if (EGL_SUCCESS != gl_context_->Resume(app_->window)) {
            UnloadResources();
            LoadResources();
        }
    }

    // Initialize GL state.
    glEnable (GL_CULL_FACE);
    glCullFace (GL_BACK);
    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_LEQUAL);

    //Note that screen size might have been changed
    glViewport(0, 0, gl_context_->GetScreenWidth(),
        gl_context_->GetScreenHeight());
    renderer_.UpdateViewport();

    tap_camera_.SetFlip(1.f, -1.f, -1.f);
    tap_camera_.SetPinchTransformFactor(2.f, 2.f, 8.f);
}

/**
 * Just the current frame in the display.
 */
void Engine::DrawFrame() {
    float fFPS;
    if (monitor_.Update(fFPS)) {
        UpdateFPS(fFPS);
    }
    renderer_.Update(monitor_.GetCurrentTime());

    renderer_.Render();

    // Swap
    if (EGL_SUCCESS != gl_context_->Swap()) {
        UnloadResources();
        LoadResources();
    }
}

void Engine::UpdateZoom(Vec2 v1, Vec2 v2) {
    zoom_distance_ = (v1 - v2).Length();
}

bool Engine::IsZoomEnabled(Vec2 v1, Vec2 v2) {
    float zoom_distance = (v1 - v2).Length();
    float diff = zoom_distance - zoom_distance_;
    zoom_distance_ = zoom_distance;
    if (!renderer_.IsZoomInEnabled()) {
        return diff < 0;
    }
    if (!renderer_.IsZoomOutEnabled()) {
        return diff > 0;
    }
    return true;
}

/**
 * Process the next input event.
 */
int32_t Engine::HandleInput(android_app *app, AInputEvent *event) {
    auto engine = (Engine*)app->userData;
    if (engine->no_error_
            && AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {

        auto doubleTapState = engine->doubletap_detector_.Detect(event);
        auto dragState = engine->drag_detector_.Detect(event);
        auto pinchState = engine->pinch_detector_.Detect(event);
        auto tapState = engine->tap_detector_.Detect(event);

        //Double tap detector has a priority over other detectors
        if (doubleTapState == GESTURE_STATE_ACTION) {
            // Detect double tap
            engine->tap_camera_.Reset(true);
        } else if (tapState == GESTURE_STATE_ACTION) {
            // Single tag is detected
            Vec2 v;
            engine->tap_detector_.GetPointer(v);
            float x, y;
            v.Value(x, y);
            y = engine->gl_context_->GetScreenHeight() - y;
            engine->renderer_.RequestRead(Vec2(x, y));
        } else {
            //Handle drag state
            if (dragState & GESTURE_STATE_START) {
                //Otherwise, start dragging
                Vec2 v;
                engine->drag_detector_.GetPointer(v);
                engine->TransformPosition(v);
                engine->tap_camera_.BeginDrag(v);
            } else if (dragState & GESTURE_STATE_MOVE) {
                Vec2 v;
                engine->drag_detector_.GetPointer(v);
                engine->TransformPosition(v);
                engine->tap_camera_.Drag(v);
            } else if (dragState & GESTURE_STATE_END) {
                engine->tap_camera_.EndDrag();
            }

            //Handle pinch state
            if (pinchState & GESTURE_STATE_START) {
                //Start new pinch
                Vec2 v1;
                Vec2 v2;
                engine->pinch_detector_.GetPointers(v1, v2);
                engine->UpdateZoom(v1, v2);
                engine->TransformPosition(v1);
                engine->TransformPosition(v2);
                engine->tap_camera_.BeginPinch(v1, v2);
            } else if (pinchState & GESTURE_STATE_MOVE) {
                //Multi touch
                //Start new pinch
                Vec2 v1;
                Vec2 v2;
                engine->pinch_detector_.GetPointers(v1, v2);
                if (engine->IsZoomEnabled(v1, v2)) {
                    engine->TransformPosition(v1);
                    engine->TransformPosition(v2);
                    engine->tap_camera_.Pinch(v1, v2);
                }
            }
        }
        return 1;
    }
    return 0;
}

void Engine::InitWindow() {
    ShowUI();

    try {
        InitDisplay();
        DrawFrame();
    } catch (const RuntimeError &err) {
        no_error_ = false;
        ShowError(err.what());
    }
}

/**
 * Process the next main command.
 */
void Engine::HandleCmd(android_app *app, int32_t cmd) {
    auto engine = (Engine*)app->userData;
    switch (cmd) {
    case APP_CMD_SAVE_STATE:
        break;
    case APP_CMD_INIT_WINDOW:
        // The window is being shown, get it ready.
        if (app->window) {
            engine->InitWindow();
        }
        break;
    case APP_CMD_TERM_WINDOW:
        // The window is being hidden or closed, clean it up.
        engine->TermDisplay();
        engine->has_focus_ = false;
        break;
    case APP_CMD_STOP:
        break;
    case APP_CMD_GAINED_FOCUS:
        engine->ResumeSensors();
        //Start animation
        engine->has_focus_ = true;
        break;
    case APP_CMD_LOST_FOCUS:
        engine->SuspendSensors();
        // Also stop animating.
        engine->has_focus_ = false;
        if (engine->no_error_) {
            engine->DrawFrame();
        }
        break;
    case APP_CMD_LOW_MEMORY:
        //Free up GL resources
        engine->TrimMemory();
        break;
    }
}

//-------------------------------------------------------------------------
//Sensor handlers
//-------------------------------------------------------------------------
void Engine::InitSensors() {
    sensor_manager_ = ASensorManager_getInstance();
    accelerometer_sensor_ = ASensorManager_getDefaultSensor(sensor_manager_,
        ASENSOR_TYPE_ACCELEROMETER);
    sensor_event_queue_ = ASensorManager_createEventQueue(sensor_manager_,
        app_->looper, LOOPER_ID_USER, nullptr, nullptr);
}

void Engine::ProcessSensors(int32_t id) {
    // If a sensor has data, process it now.
    if (id == LOOPER_ID_USER) {
        if (accelerometer_sensor_) {
            ASensorEvent event;
            while (ASensorEventQueue_getEvents(sensor_event_queue_, &event, 1)
                    > 0) {
                // Empty loop
            }
        }
    }
}

void Engine::ResumeSensors() {
    // When our app gains focus, we start monitoring the accelerometer.
    if (accelerometer_sensor_) {
        ASensorEventQueue_enableSensor(sensor_event_queue_,
            accelerometer_sensor_);
        // We'd like to get 60 events per second (in us).
        ASensorEventQueue_setEventRate(sensor_event_queue_,
            accelerometer_sensor_, (1000L / 60) * 1000);
    }
}

void Engine::SuspendSensors() {
    // When our app loses focus, we stop monitoring the accelerometer.
    // This is to avoid consuming battery while not being used.
    if (accelerometer_sensor_) {
        ASensorEventQueue_disableSensor(sensor_event_queue_,
            accelerometer_sensor_);
    }
}

//-------------------------------------------------------------------------
// Misc
//-------------------------------------------------------------------------
void Engine::SetState(android_app *state) {
    app_ = state;
    doubletap_detector_.SetConfiguration(app_->config);
    drag_detector_.SetConfiguration(app_->config);
    pinch_detector_.SetConfiguration(app_->config);
    tap_detector_.SetConfiguration(app_->config);
}

void Engine::TransformPosition(Vec2 &vec) {
    vec = Vec2(2.0f, 2.0f) * vec
            / Vec2(gl_context_->GetScreenWidth(),
                gl_context_->GetScreenHeight()) - Vec2(1.f, 1.f);
}

void Engine::ShowUI() {
    JNIEnv *jni;
    app_->activity->vm->AttachCurrentThread(&jni, nullptr);

    //Default class retrieval
    auto clazz = jni->GetObjectClass(app_->activity->clazz);
    auto methodID = jni->GetMethodID(clazz, "showUI", "()V");
    jni->CallVoidMethod(app_->activity->clazz, methodID);

    app_->activity->vm->DetachCurrentThread();
}

void Engine::ShowError(const char *error) {
    JNIEnv *jni;
    app_->activity->vm->AttachCurrentThread(&jni, nullptr);

    //Default class retrieval
    auto clazz = jni->GetObjectClass(app_->activity->clazz);
    auto methodID = jni->GetMethodID(clazz, "updateLabel",
        "(Ljava/lang/String;)V");
    jstring label = jni->NewStringUTF(error);
    jni->CallVoidMethod(app_->activity->clazz, methodID, label);

    app_->activity->vm->DetachCurrentThread();
}

void Engine::UpdateFPS(float fFPS) {
    JNIEnv *jni;
    app_->activity->vm->AttachCurrentThread(&jni, nullptr);

    //Default class retrieval
    auto clazz = jni->GetObjectClass(app_->activity->clazz);
    auto methodID = jni->GetMethodID(clazz, "updateFPS", "(F)V");
    jni->CallVoidMethod(app_->activity->clazz, methodID, fFPS);

    app_->activity->vm->DetachCurrentThread();
}

void Engine::ShowAds() {
    JNIEnv *jni;
    app_->activity->vm->AttachCurrentThread(&jni, nullptr);

    //Default class retrieval
    auto clazz = jni->GetObjectClass(app_->activity->clazz);
    auto methodID = jni->GetMethodID(clazz, "showAds", "()V");
    jni->CallVoidMethod(app_->activity->clazz, methodID);

    app_->activity->vm->DetachCurrentThread();
}

void Engine::UseTle(char *path) {
    LOGI("New TLE file: %s", path);
    auto reader = FileReaderFactory::Get(APP, path);
    renderer_.InitSatelliteMgr(*reader);
    free(path);
}

void Engine::ShowBeam(int num) {
    JNIEnv *jni;
    app_->activity->vm->AttachCurrentThread(&jni, nullptr);

    //Default class retrieval
    auto clazz = jni->GetObjectClass(app_->activity->clazz);
    auto methodID = jni->GetMethodID(clazz, "showBeam",
        "(Ljava/lang/String;FFF)V");
    Satellite &sat = renderer_.GetSatellite(num);
    jstring j_name = jni->NewStringUTF(sat.GetName().c_str());
    jni->CallVoidMethod(app_->activity->clazz, methodID, j_name,
        sat.GetLatitude(), sat.GetLongitude(), sat.GetAltitude());

    app_->activity->vm->DetachCurrentThread();
}

void Engine::HandleMessage(Message msg) {
    switch (msg.cmd) {
    case SHOW_ADS:
        ShowAds();
        break;
    case USE_TLE:
        UseTle(reinterpret_cast<char*>(msg.payload));
        break;
    case SHOW_BEAM:
        ShowBeam(reinterpret_cast<int>(msg.payload));
        break;
    }
}
