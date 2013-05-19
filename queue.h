#ifndef __DRC_CAP_QUEUE_H_
#define __DRC_CAP_QUEUE_H_

#include <mutex>

// TODO(delroth): make lock free without segfaults

template <typename T>
class LockedQueue {
  public:
    LockedQueue() : first_(nullptr), last_(nullptr) {
    }

    ~LockedQueue() {
        while (first_ != nullptr) {
            T* tmp = first_;
            first_ = tmp->next;
            delete tmp;
        }
    }

    void Push(T* t) {
        std::lock_guard<std::mutex> lk(mutex_);
        t->next = nullptr;

        if (last_) {
            last_->next = t;
            last_ = t;
        } else {
            first_ = last_ = t;
        }
    }

    T* Pop() {
        if (!first_)
            return nullptr;

        std::lock_guard<std::mutex> lk(mutex_);
        if (!first_)
            return nullptr;

        T* ret = first_;
        first_ = first_->next;
        if (!first_)
            last_ = nullptr;

        return ret;
    }

  private:
    T* first_;
    T* last_;
    std::mutex mutex_;
};

#endif
