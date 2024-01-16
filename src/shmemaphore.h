#include <fcntl.h>  // O_ flags
#include <semaphore.h>
#include <sys/mman.h>  // shm_open, shm_unlink, mmap
#include <sys/shm.h>   // shmid, shmget, shmat
#include <sys/stat.h>  // mode constants
#include <unistd.h>    // ftruncate

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

class Semaphore {
 public:
  Semaphore(const std::string &_name, int _value = 0);
  ~Semaphore() noexcept(false);

  void post();
  void wait();

 private:
  std::string name;
  sem_t *sem;
};

class SharedMemorySegment {
 public:
  SharedMemorySegment(const std::string &_name, size_t _size,
                      bool create = false);
  ~SharedMemorySegment() noexcept(false);

  void map(const off_t length = 0);
  void unmap();
  void setData(const void *data, const size_t length);
  const void *getData();

 private:
  std::string name;
  size_t size;
  int fd;
  void *data;
};

Semaphore::Semaphore(const std::string &_name, int _value) : name(_name) {
  sem = sem_open(name.c_str(), O_CREAT, 0777, _value);
  if (sem == SEM_FAILED) {
    std::string what("Failed to create semaphore [sem_open(3)]: ");
    what += std::strerror(errno);
    throw std::runtime_error(what);
  }
}

Semaphore::~Semaphore() noexcept(false) {
  int err = sem_close(sem);
  if (err == -1) {
    std::string what("Failed to close semaphore [sem_close(3)]: ");
    what += std::strerror(errno);
    throw std::runtime_error(what);
  }
  err = sem_unlink(name.c_str());
  if (err == -1 && errno != ENOENT) {
    std::string what("Failed to unlink semaphore [sem_unlink(3)]: ");
    what += std::strerror(errno);
    throw std::runtime_error(what);
  }
}

void Semaphore::post() {
  int err = sem_post(sem);
  if (err == -1) {
    std::string what("Failed to increment semaphore [sem_post(3)]: ");
    what += std::strerror(errno);
    throw std::runtime_error(what);
  }
}

void Semaphore::wait() {
  int err = sem_wait(sem);
  if (err == -1) {
    std::string what("Failed to decrement semaphore [sem_wait(3)]: ");
    what += std::strerror(errno);
    throw std::runtime_error(what);
  }
}

SharedMemorySegment::SharedMemorySegment(const std::string &_name, size_t _size,
                                         bool create)
    : name(_name), size(_size) {
  if (create) shm_unlink(name.c_str());  // clean up in case a persisted

  fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0777);
  if (fd == -1) {
    std::string what("Failed to open shared memory [shm_open(3)]: ");
    what += std::strerror(errno);
    throw std::runtime_error(what);
  }

  if (create) {
    int err = ftruncate(fd, size);
    if (err == -1) {
      std::string what("Failed to resize shared memory [ftruncate(2)]: ");
      what += std::strerror(errno);
      throw std::runtime_error(what);
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
  std::memcpy(this->data, data, length);
}

const void *SharedMemorySegment::getData() { return data; }
