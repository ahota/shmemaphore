#pragma once

#include <algorithm>

#include "sem.h"
#include "shmem.h"

inline std::string copyAndReplace(const std::string &in, char oldChar, char newChar)
{
  std::string out(in);
  std::replace(out.begin(), out.end(), oldChar, newChar);
  return out;
}

class Shmemaphore
{
 public:
  Shmemaphore(const std::string &name, bool _owner = false)
      : owner(_owner),
        suffix(copyAndReplace(name, '/', '_')),
        nameSem(std::string("/SEM_") + suffix),
        nameSeg(std::string("/SEG_") + suffix),
        requestSem(nameSem + "_REQ", 0),
        responseSem(nameSem + "_RES", 0),
        headerSeg(nameSeg + "_HEAD", 1, owner),
        dataSeg(nameSeg + "_DATA", 1, owner)
  {}

  void setHeader(const void *data, const size_t length)
  {
    headerSeg.setData(data, length);
  }
  void setData(const void *data, const size_t length)
  {
    dataSeg.setData(data, length);
  }
  const void *getHeader(const size_t dataSize = 0)
  {
    return headerSeg.getData(dataSize);
  }
  const void *getData(const size_t dataSize = 0)
  {
    return dataSeg.getData(dataSize);
  }

  void waitForResponse() { responseSem.wait(); }
  void requestComplete() { requestSem.post(); }
  void waitForRequest() { requestSem.wait(); }
  void responseComplete() { responseSem.post(); }

  void sendString(const std::string &message)
  {
    uint64_t messageSize = message.size();
    setHeader(&messageSize, sizeof(messageSize));
    setData(message.c_str(), messageSize);

    if (owner)
      requestComplete();
    else
      responseComplete();
  }

  std::string recvString()
  {
    if (owner)
      waitForResponse();
    else
      waitForRequest();

    uint64_t dataSize;
    dataSize = *(uint64_t *)getHeader(sizeof(dataSize));
    const char *raw = (char *)getData(dataSize);
    std::string message(raw, dataSize);
    return message;
  }

 private:
  std::string suffix;
  std::string nameSem;
  std::string nameSeg;
  bool owner;

  Semaphore requestSem;
  Semaphore responseSem;

  SharedMemorySegment headerSeg;
  SharedMemorySegment dataSeg;
};
