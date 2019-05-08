#ifndef MAIN_COMPRESSION_CACHE_H_
#define MAIN_COMPRESSION_CACHE_H_

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "base/reader.h"
#include "base/task-runner.h"

class CompressionCache {
 private:
  enum {
    kChunkSize = 16384,
  };

  using Chunk = std::array<char, kChunkSize>;
  using ChunkList = std::forward_list<Chunk>;

  class CachedFile {
   public:
    static Result<std::shared_ptr<CachedFile>> Create(absl::string_view path);
    CachedFile(ChunkList chunks, size_t last_chunk_length);

    const ChunkList& chunks() const { return chunks_; }
    size_t last_chunk_length() const { return last_chunk_length_; }

   private:
    const ChunkList chunks_;
    const size_t last_chunk_length_ = 0;
  };

 public:
  class File : public Reader {
   public:
    File(File&&) = default;
    File& operator=(File&&) = default;

    // Reader implementation:
    Result<ssize_t> Read(absl::Span<char> buf) override;

   private:
    friend class CompressionCache;
    explicit File(std::shared_ptr<CachedFile> file);

    std::shared_ptr<CachedFile> file_;
    ChunkList::const_iterator cur_chunk_;
    size_t cur_chunk_offset_ = 0;
  };

  explicit CompressionCache(size_t max_size_bytes);
  CompressionCache(const CompressionCache&) = delete;
  CompressionCache& operator=(const CompressionCache&) = delete;

  using FileCallback = std::function<void(Result<File>)>;
  void RequestFile(absl::string_view path, FileCallback callback);

 private:
  using PathToCachedFile =
      absl::flat_hash_map<std::string, std::shared_ptr<CachedFile>>;

  struct PendingRead {
    std::vector<FileCallback> callbacks;
  };
  using PathToPendingRead = absl::flat_hash_map<std::string, PendingRead>;

  void RequestFileSlowPath(std::string path, FileCallback callback,
                           std::shared_ptr<TaskRunner> caller);
  void ReadFile(std::string path, PathToPendingRead::iterator pending_read_it,
                std::shared_ptr<TaskRunner> my_thread);
  void OnReadFile(std::string path, PathToPendingRead::iterator pending_read_it,
                  Result<std::shared_ptr<CachedFile>> file);

  // TODO(bcf): Enforce file size.
  const size_t max_size_bytes_;

  // Fast path unlocked version of |path_to_cached_file_| which can be accessed
  // on any thread. Might be out of date. If there is a miss here, will fallback
  // to slow path.
  ABSL_CACHELINE_ALIGNED std::shared_ptr<const PathToCachedFile>
      unlocked_path_to_cached_file_;

  ABSL_CACHELINE_ALIGNED std::shared_ptr<TaskRunner> task_runner_;

  // Owned by |task_runner_|.
  // TODO(bcf): Invalidation based on inotify.
  PathToCachedFile path_to_cached_file_;
  PathToPendingRead path_to_pending_read_;
};

// Implementation:

inline Result<ssize_t> CompressionCache::File::Read(absl::Span<char> buf) {
  if (cur_chunk_ == file_->chunks().end()) {
    return -1;
  }
  size_t num_read = 0;
  while (num_read < buf.size()) {
    if (cur_chunk_ == file_->chunks().end()) {
      break;
    }
    auto next_chunk = ++cur_chunk_;
    bool is_last_chunk = next_chunk == file_->chunks().end();

    char* write_to = buf.data() + num_read;
    size_t can_write = buf.size() - num_read;

    const char* read_from = cur_chunk_->data() + cur_chunk_offset_;
    size_t chunk_length =
        is_last_chunk ? file_->last_chunk_length() : cur_chunk_->size();
    size_t can_read = chunk_length - cur_chunk_offset_;

    size_t num_to_copy = std::min(can_write, can_read);
    memcpy(write_to, read_from, num_to_copy);

    num_read += num_to_copy;
    cur_chunk_offset_ += num_to_copy;
    if (cur_chunk_offset_ == chunk_length) {
      cur_chunk_ = next_chunk;
    }
  }

  return num_read;
}

#endif  // MAIN_COMPRESSION_CACHE_H_
