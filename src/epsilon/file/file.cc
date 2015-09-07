
#include "epsilon/file/file.h"

#include <algorithm>
#include <mutex>
#include <unordered_map>

#include <glog/logging.h>

#include "epsilon/util/string.h"

const std::string kMemFilePrefix = "/mem/";

namespace file {

std::mutex mem_file_lock;
std::unordered_map<std::string, std::string> mem_file_contents;

class MemFile final : public File {
public:
  MemFile(const std::string& name, const std::string& mode)
      : name_(name), mode_(mode) {}

  void Open() override {
    if (mode_ == kWriteMode) {
      // Overwrite semantics
      std::lock_guard<std::mutex> l(mem_file_lock);
      mem_file_contents[name_] = "";
    }
  }
  void Close() override {}

  std::string Read(size_t pos, size_t len) override {
    CHECK_EQ(mode_, kReadMode);

    std::lock_guard<std::mutex> l(mem_file_lock);
    auto iter = mem_file_contents.find(name_);
    CHECK(iter != mem_file_contents.end());

    return iter->second.substr(pos, len);
  }

  void Write(const std::string& data) override {
    CHECK_EQ(mode_, kWriteMode);
    std::lock_guard<std::mutex> l(mem_file_lock);
    mem_file_contents[name_] += data;
    VLOG(2) << "Wrote " << name_ << ", " << data.size() << " bytes";
  }

private:
  std::string name_, mode_;
};

std::unique_ptr<File> Open(const std::string& name, const std::string& mode) {
  CHECK(mode == kReadMode || mode == kWriteMode) << "Unknown mode " << mode;
  std::unique_ptr<File> f;
  if (name.compare(0, kMemFilePrefix.size(), kMemFilePrefix) == 0) {
    f.reset(new MemFile(name.substr(kMemFilePrefix.size()), mode));
  } else {
    LOG(FATAL) << "Unknown file type: " << name;
  }

  f->Open();
  return f;
}

}  // namespace file
