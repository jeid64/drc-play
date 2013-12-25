// Copyright (c) 2013, Pierre Bourdon, All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

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
