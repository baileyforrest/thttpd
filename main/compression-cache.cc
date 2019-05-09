#include "main/compression-cache.h"

#include <forward_list>

#include "base/err.h"
#include "base/file-reader.h"
#include "base/logging.h"
#include "base/scoped-destructor.h"
#include "base/zlib-deflate-reader.h"

// static
Result<std::shared_ptr<CompressionCache::CachedFile>>
CompressionCache::CachedFile::Create(absl::string_view path) {
  ChunkList chunks;

  auto file_reader = TRY(FileReader::Create(path));
  auto zlib_reader = TRY(ZlibDeflateReader::Create(&file_reader));

  auto insert_at = chunks.before_begin();
  size_t chunk_length = 0;
  bool eof = false;
  size_t total_size = 0;

  while (!eof) {
    insert_at = chunks.emplace_after(insert_at);
    chunk_length = 0;
    while (chunk_length < insert_at->size()) {
      ssize_t num_read =
          TRY(zlib_reader->Read({insert_at->data() + chunk_length,
                                 insert_at->size() - chunk_length}));
      if (num_read == -1) {
        eof = true;
        break;
      }
      total_size += num_read;
    }
  }

  return std::make_shared<CachedFile>(std::move(chunks), chunk_length,
                                      total_size);
}

CompressionCache::CachedFile::CachedFile(ChunkList chunks,
                                         size_t last_chunk_length, size_t size)
    : chunks_(std::move(chunks)),
      last_chunk_length_(last_chunk_length),
      size_(size) {}

CompressionCache::File::File(std::shared_ptr<CachedFile> file)
    : file_(std::move(file)), cur_chunk_(file_->chunks().cbegin()) {}

CompressionCache::CompressionCache(size_t max_size_bytes)
    : max_size_bytes_(max_size_bytes),
      unlocked_path_to_cached_file_(std::make_shared<PathToCachedFile>()),
      task_runner_(TaskRunner::Create()) {}

void CompressionCache::RequestFile(absl::string_view path,
                                   FileCallback callback) {
  std::string str_path(path);

  // Fast path: Check if the cache file is in the unlocked cache.
  {
    auto unlocked_path_to_cached_file =
        atomic_load(&unlocked_path_to_cached_file_);
    auto it = unlocked_path_to_cached_file->find(str_path);
    if (it != unlocked_path_to_cached_file->end()) {
      callback(File(it->second));
      return;
    }
  }

  task_runner_->PostTask(BindOnce(&CompressionCache::RequestFileSlowPath, this,
                                  std::move(str_path), std::move(callback),
                                  TaskRunner::CurrentTaskRunner()));
}

void CompressionCache::RequestFileSlowPath(std::string path,
                                           FileCallback callback,
                                           std::shared_ptr<TaskRunner> caller) {
  ABSL_ASSERT(task_runner_->IsCurrentThread());

  // Check the real |path_to_cached_file_|.
  {
    auto it = path_to_cached_file_.find(path);
    if (it != path_to_cached_file_.end()) {
      // TODO(bcf): Need some heuristic for when to update
      // unlocked_path_to_cache_file_.

      callback(File(it->second));
      return;
    }
  }

  auto pending_read_it = path_to_pending_read_.emplace().first;

  // Cached file doesn't exist, need to load it. Let the calling thread read the
  // file for load balancing and then send it back to us.

  auto& pending_callbacks = pending_read_it->second.callbacks;
  pending_callbacks.push_back(std::move(callback));

  // Exit if this isn't the first pending request for this file.
  if (pending_callbacks.size() > 1) {
    return;
  }

  caller->PostTask(BindOnce(&CompressionCache::ReadFile, this, std::move(path),
                            pending_read_it, task_runner_));
}

void CompressionCache::ReadFile(std::string path,
                                PathToPendingRead::iterator pending_read_it,
                                std::shared_ptr<TaskRunner> my_thread) {
  auto file = CachedFile::Create(path);

  my_thread->PostTask(BindOnce(&CompressionCache::OnReadFile, this,
                               std::move(path), pending_read_it,
                               std::move(file)));
}

void CompressionCache::OnReadFile(std::string path,
                                  PathToPendingRead::iterator pending_read_it,
                                  Result<std::shared_ptr<CachedFile>> file) {
  ABSL_ASSERT(task_runner_->IsCurrentThread());

  auto pending_read = std::move(pending_read_it->second);
  path_to_pending_read_.erase(pending_read_it);

  if (!file.ok()) {
    for (const auto& callback : pending_read.callbacks) {
      callback(file.err());
    }

    return;
  }

  path_to_cached_file_.emplace(std::move(path), *file);
  for (const auto& callback : pending_read.callbacks) {
    callback(File(*file));
  }
}
