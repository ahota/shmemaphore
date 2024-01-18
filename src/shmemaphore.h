#include "sem.h"
#include "shmem.h"

class Shmemaphore {
  public:
    Shmemaphore(const std::string &name, size_t shmSize, bool owner = false);
    // ~Shmemaphore() noexcept(false); // I don't know if I need this

    void setData(const void *data, const size_t length);
    const void *getData(const size_t dataSize = 0);

    void requestComplete();
    void responseComplete();
  private:
    Semaphore requestSem;
    Semaphore responseSem;

    SharedMemorySegment headerSeg;
    SharedMemorySegment dataSeg;
};
