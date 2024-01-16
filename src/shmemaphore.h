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
    perror("sem_open(3) error");
    throw std::runtime_error("Semaphore() Failed to create semaphore");
  }
}

Semaphore::~Semaphore() noexcept(false) {
  int err = sem_close(sem);
  if (err == -1) {
    perror("sem_close(3) error");
    throw std::runtime_error("~Semaphore() Failed to close semaphore");
  }
  err = sem_unlink(name.c_str());
  if (err == -1 && errno != ENOENT) {
    perror("sem_unlink(3) error");
    throw std::runtime_error("~Semaphore() Failed to unlink semaphore");
  }
}

void Semaphore::post() {
  int err = sem_post(sem);
  if (err == -1) {
    perror("sem_post(3) error");
    throw std::runtime_error("post() Failed to increment semaphore");
  }
}

void Semaphore::wait() {
  int err = sem_wait(sem);
  if (err == -1) {
    perror("sem_wait(3) error");
    throw std::runtime_error("wait() Failed to decrement semaphore");
  }
}

SharedMemorySegment::SharedMemorySegment(const std::string &_name, size_t _size,
                                         bool create)
    : name(_name), size(_size) {
  if (create) shm_unlink(name.c_str());  // clean up in case a segment persisted

  fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0777);
  if (fd == -1) {
    perror("shm_open error");
    throw std::runtime_error(
        "SharedMemorySegment() Failed to open shared memory segment");
  }

  if (create) {
    int err = ftruncate(fd, size);
    if (err == -1) {
      perror("ftruncate(2) error");
      throw std::runtime_error(
          "SharedMemorySegment() Failed to resize shared memory segment");
    }
  }
}

SharedMemorySegment::~SharedMemorySegment() noexcept(false) {
  int err = close(fd);
  if (err == -1) {
    perror("close(2) error");
    throw std::runtime_error(
        "~SharedMemorySegment() Failed to close shared memory file descriptor");
  }

  err = shm_unlink(name.c_str());
  if (err == -1 && errno != ENOENT) {
    perror("shm_unlink(3) error");
    throw std::runtime_error(
        "~SharedMemorySegment() Failed to unlink shared memory segment");
  }
}

void SharedMemorySegment::map(const off_t length) {
  if (length >= size) {  // don't shrink
    size = length;
    // NOTE: this will NOT work on macOS
    // macOS only allows one ftruncate per lifetime
    int err = ftruncate(fd, length);
    if (err == -1) {
      perror("ftruncate(2) error");
      throw std::runtime_error("map() Failed to resize shared memory segment");
    }
  }
  data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    perror("mmap(2) error");
    throw std::runtime_error("map() Failed to map shared memory segment");
  }
}

void SharedMemorySegment::unmap() {
  int err = munmap(data, size);
  if (err == -1) {
    perror("munmap(2) error");
    throw std::runtime_error("unmap() Failed to unmap shared memory segment");
  }
}

void SharedMemorySegment::setData(const void *data, const size_t length) {
  std::memcpy(this->data, data, length);
}

const void *SharedMemorySegment::getData() { return data; }
