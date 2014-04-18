#include <fcntl.h>
#include <unistd.h>
#include <cassert>

#include "NDKHelper.h"
#include "MessageQueue.h"

int MessageQueue::current_id_ = LOOPER_ID_USER;
MessageQueue g_queue;

using namespace std;

/* External C interface */
void PostMessage(int queue_id, Message msg) {
    g_queue.PostMessage(queue_id, msg);
}

void PostMessage(Message msg) {
    g_queue.PostMessage(msg);
}

int AddMessageQueue(android_poll_source *src) {
    return g_queue.AddMessageQueue(src);
}

void InitMessageQueue(ALooper* looper) {
    g_queue.Init(looper);
}

Message ReadMessageQueue(int pipe_read) {
    Message msg;
    if (read(pipe_read, &msg, sizeof(msg)) != sizeof(msg)) {
        LOGE("No data on message pipe!");
    }
    return msg;
}

/* Internal implementation */

#if 0
static int MessageQueueCb(int fd, int events, void* user) {
    Message msg;
    HandleMessagePtr handler = (HandleMessagePtr)user;
    while (read(fd, (void*)&msg, sizeof(msg)) == sizeof(msg)) {
        /* Do whatever needs doing when a message is
         * received. Provided sizeof(msg) <= PIPE_BUF
         * you should never get short reads. */
        handler(msg);
    }

    return 1;
}
#endif

int MessageQueue::AddMessageQueue(android_poll_source *src) {
    if (!looper_) {
        return -1;
    }

    current_id_++;
    src->id = current_id_;
    MessageQueueEntry entry;
    if (pipe(entry.message_pipe_)) {
        LOGE("could not create pipe: %s", strerror(errno));
        return -1;
    }
    /* Register the file descriptor to listen on. */
    int msgread = entry.message_pipe_[0];
    ALooper_addFd(looper_, msgread, current_id_, ALOOPER_EVENT_INPUT, nullptr,
        src);
    // Increment id right before the insert
    entries_.insert(pair<int, MessageQueueEntry>(current_id_, entry));
    return msgread;
}

void MessageQueue::PostMessage(int queue_id, Message msg) {
    if (!looper_) {
        return;
    }

    int pipe = entries_[queue_id].message_pipe_[1];
    if (write(pipe, &msg, sizeof(msg)) != sizeof(msg)) {
        LOGE("Failure writing message: %s\n", strerror(errno));
    }
}

void MessageQueue::PostMessage(Message msg) {
    PostMessage(current_id_, msg);
}
