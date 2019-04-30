#ifndef BASE_MPSC_QUEUE_H_
#define BASE_MPSC_QUEUE_H_

#include <algorithm>
#include <atomic>
#include <utility>

#include "absl/base/optimization.h"
#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"

// Multi-producer single consumer queue.
template <typename T>
class MpscQueue {
 public:
  // Store a dummy node at the head. This lets Push avoid operating on |head_|
  // pointer.
  MpscQueue() : head_(new Node({})), tail_(head_) {}
  ~MpscQueue() {
    while (head_ != nullptr) {
      Node* next = head_->next;
      delete head_;
      head_ = next;
    }
  }

  bool Empty() const {
    return head_->next.load(std::memory_order_acquire) == nullptr;
  }

  void Push(T val) {
    Node* node = new Node(std::move(val));
    absl::WriterMutexLock lock(&tail_lock_);
    tail_->next.store(node, std::memory_order_release);
    tail_ = node;
  }

  T Pop() {
    ABSL_ASSERT(!Empty());
    Node* dummy = head_;
    Node* next = dummy->next.load(std::memory_order_acquire);
    T ret = std::move(next->value);
    head_ = next;
    delete dummy;
    return ret;
  }

 private:
  struct Node {
    explicit Node(T val) : value(std::move(val)) {}
    std::atomic<Node*> next{nullptr};
    T value{};

    static constexpr int kPadOffset = sizeof(next) + sizeof(T);
    static constexpr int kPadSize = kPadOffset >= ABSL_CACHELINE_SIZE
                                        ? 0
                                        : ABSL_CACHELINE_SIZE - kPadOffset;
    char pad[kPadSize];
  };

  ABSL_CACHELINE_ALIGNED Node* head_ = nullptr;
  ABSL_CACHELINE_ALIGNED Node* tail_ GUARDED_BY(tail_lock_) = nullptr;
  ABSL_CACHELINE_ALIGNED absl::Mutex tail_lock_;
};

#endif  // BASE_MPSC_QUEUE_H_
