#pragma once

#include <map>

// Message commands
enum {
    USE_TLE, SHOW_BEAM
};

struct Message {
    int cmd;
    void *payload;
};

typedef void (*HandleMessagePtr)(Message msg);

void PostMessage(int queue_id, Message msg);
void PostMessage(Message msg);
int AddMessageQueue(android_poll_source *src);
void InitMessageQueue(ALooper* looper);
Message ReadMessageQueue(int pipe_read);

struct MessageQueueEntry {
    int message_pipe_[2];
};

class MessageQueue {
    std::map<int, MessageQueueEntry> entries_;
    ALooper* looper_;
    static int current_id_;
public:
    MessageQueue() :
                looper_(nullptr) {
    }
    int AddMessageQueue(android_poll_source *src);
    void PostMessage(int queue_id, Message msg);
    void PostMessage(Message msg);
    void Init(ALooper* looper) {
        looper_ = looper;
    }
};
