#pragma once

#include <fcntl.h> // O_ flags
#include <semaphore.h>
#include <sys/stat.h> // mode constants

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

class Semaphore
{
 public:
  Semaphore(const std::string &_name, int _value = 0) : name(_name)
  {
    sem = sem_open(name.c_str(), O_CREAT, 0777, _value);
    if (sem == SEM_FAILED) {
      std::string what("Failed to create semaphore [sem_open(3)]: ");
      what += std::strerror(errno);
      throw std::runtime_error(what);
    }
  }

  ~Semaphore() noexcept(false)
  {
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

  void post()
  {
    int err = sem_post(sem);
    if (err == -1) {
      std::string what("Failed to increment semaphore [sem_post(3)]: ");
      what += std::strerror(errno);
      throw std::runtime_error(what);
    }
  }

  void wait()
  {
    int err = sem_wait(sem);
    if (err == -1) {
      std::string what("Failed to decrement semaphore [sem_wait(3)]: ");
      what += std::strerror(errno);
      throw std::runtime_error(what);
    }
  }

 private:
  std::string name;
  sem_t *sem;
};
