#pragma once

#include <fcntl.h>  // O_ flags
#include <sys/mman.h>  // shm_open, shm_unlink, mmap
#include <sys/stat.h>  // mode constants
#include <unistd.h>    // ftruncate

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>

class SharedMemorySegment {
 public:
  SharedMemorySegment(const std::string &_name, size_t _size,
                      bool create = false);
  ~SharedMemorySegment() noexcept(false);

  void map(const off_t length = 0);
  void unmap();
  void setData(const void *data, const size_t length);
  const void *getData(const size_t dataSize = 0);

 private:
  std::string name;
  size_t size;
  int fd;
  void *data;
};

SharedMemorySegment::SharedMemorySegment(const std::string &_name, size_t _size,
                                         bool create)
    : name(_name), size(_size) {
  if (create) {
    // clean up in case an old block persisted
    shm_unlink(name.c_str());

    fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0777);
    if (fd == -1) {
      std::string what("Failed to open shared memory [shm_open(3)]: ");
      what += std::strerror(errno);
      throw std::runtime_error(what);
    }

    int err = ftruncate(fd, size);
    if (err == -1) {
      std::string what("Failed to resize shared memory [ftruncate(2)]: ");
      what += std::strerror(errno);
      throw std::runtime_error(what);
    }
  } else {
    int counter = 0;
    // poll until this block becomes available by the creator
    while((fd = shm_open(name.c_str(), O_RDWR, 0777)) < 0) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(100ms);
      if (counter++ % 10 == 0)
        std::cerr << "Waiting for shared mem '" << name << "'..." << std::endl;
    }
  }
}

SharedMemorySegment::~SharedMemorySegment() noexcept(false) {
  int err = close(fd);
  if (err == -1) {
    std::string what("Failed to close shared memory [close(2)]: ");
    what += std::strerror(errno);
    throw std::runtime_error(what);
  }

  err = shm_unlink(name.c_str());
  if (err == -1 && errno != ENOENT) {
    std::string what("Failed to unlink shared memory [shm_unlink(3)]: ");
    what += std::strerror(errno);
    throw std::runtime_error(what);
  }
}

void SharedMemorySegment::map(const off_t length) {
  if (length >= size) {  // don't shrink
    size = length;
    // NOTE: this will NOT work on macOS
    // macOS only allows one ftruncate per lifetime
    int err = ftruncate(fd, length);
    if (err == -1) {
      std::string what("Failed to resize shared memory [ftruncate(2)]: ");
      what += std::strerror(errno);
      throw std::runtime_error(what);
    }
  }
  data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    std::string what("Failed to map shared memory [mmap(2)]: ");
    what += std::strerror(errno);
    throw std::runtime_error(what);
  }
}

void SharedMemorySegment::unmap() {
  int err = munmap(data, size);
  if (err == -1) {
    std::string what("Failed to unmap shared memory [munmap(2)]: ");
    what += std::strerror(errno);
    throw std::runtime_error(what);
  }
}

void SharedMemorySegment::setData(const void *data, const size_t length) {
  map(length);
  std::memcpy(this->data, data, length);
  unmap();
}

const void *SharedMemorySegment::getData(const size_t dataSize) {
  map(dataSize);
  return data;
  unmap();
}
