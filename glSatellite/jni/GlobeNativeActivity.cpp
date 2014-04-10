#include "Engine.h"
#include "MessageQueue.h"

const char *HELPER_CLASS_NAME = "ca/raido/helper/NDKHelper";

Engine g_engine;
android_poll_source g_poll_src;
int g_msgread;

// Message queue helpers
void HandleMessageWrapper(android_app* app, android_poll_source* source) {
    Message msg = ReadMessageQueue(g_msgread);
    g_engine.HandleMessage(msg);
}

extern "C" JNIEXPORT void JNICALL
Java_ca_raido_globe_GlobeNativeActivity_ShowAds(
        JNIEnv *env, jobject thiz) {
    Message msg = {SHOW_ADS, 0};
    PostMessage(msg);
}

extern "C" JNIEXPORT void JNICALL
Java_ca_raido_globe_GlobeNativeActivity_UseTle(
        JNIEnv *env, jobject thiz, jstring javaString) {
    const char *nativeString = env->GetStringUTFChars(javaString, 0);
    size_t len = strlen(nativeString);
    char *path = reinterpret_cast<char*>(calloc(len + 1, 1));
    strncpy(path, nativeString, len);
    env->ReleaseStringUTFChars(javaString, nativeString);
    Message msg = {USE_TLE, path};
    PostMessage(msg);
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(android_app *state) {
    app_dummy();

    g_engine.SetState(state);

    //Init helper functions
    ndk_helper::JNIHelper::Init(state->activity, HELPER_CLASS_NAME);

    state->userData = &g_engine;
    state->onAppCmd = Engine::HandleCmd;
    state->onInputEvent = Engine::HandleInput;

    InitMessageQueue(state->looper);
    g_poll_src.app = state;
    g_poll_src.process = HandleMessageWrapper;
    g_msgread = AddMessageQueue(&g_poll_src);

#ifdef USE_NDK_PROFILER
    monstartup("libGlobeNativeActivity.so");
#endif

    // Prepare to monitor accelerometer
    g_engine.InitSensors();

    // loop waiting for stuff to do.
    while (true) {
        // Read all pending events.
        int id;
        int events;
        android_poll_source *source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((id = ALooper_pollAll(g_engine.IsReady() ? 0 : -1, nullptr,
                &events, (void **) &source)) >= 0) {
            // Process this event.
            if (source) {
                source->process(state, source);
            }

            g_engine.ProcessSensors(id);

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                g_engine.TermDisplay();
                return;
            }
        }

        if (g_engine.IsReady()) {
            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            g_engine.DrawFrame();
        }
    }
}
