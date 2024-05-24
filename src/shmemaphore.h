#pragma once

#include <algorithm>

#include "sem.h"
#include "shmem.h"

std::string copyAndReplace(const std::string &in, char oldChar, char newChar) {
  std::string out(in);
  std::replace(out.begin(), out.end(), oldChar, newChar);
  return out;
}

class Shmemaphore {
 public:
  Shmemaphore(const std::string &_name, bool owner = false);

  void setHeader(const void *data, const size_t length);
  void setData(const void *data, const size_t length);
  const void *getHeader(const size_t dataSize = 0);
  const void *getData(const size_t dataSize = 0);

  void waitForRequest();
  void waitForResponse();
  void requestComplete();
  void responseComplete();

  void sendString(const std::string &message);
  std::string recvString();
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

Shmemaphore::Shmemaphore(const std::string &name, bool _owner)
    : owner(_owner),
      suffix(copyAndReplace(name, '/', '_')),
      nameSem(std::string("/SEM_") + suffix),
      nameSeg(std::string("/SEG_") + suffix),
      requestSem(nameSem + "_REQ", 0),
      responseSem(nameSem + "_RES", 0),
      headerSeg(nameSeg + "_HEAD", 1, owner),
      dataSeg(nameSeg + "_DATA", 1, owner) {}

void Shmemaphore::setHeader(const void *data, const size_t length) {
  headerSeg.setData(data, length);
}
void Shmemaphore::setData(const void *data, const size_t length) {
  dataSeg.setData(data, length);
}
const void *Shmemaphore::getHeader(const size_t dataSize) {
  return headerSeg.getData(dataSize);
}
const void *Shmemaphore::getData(const size_t dataSize) {
  return dataSeg.getData(dataSize);
}

void Shmemaphore::waitForResponse() { responseSem.wait(); }
void Shmemaphore::requestComplete() { requestSem.post(); }
void Shmemaphore::waitForRequest() { requestSem.wait(); }
void Shmemaphore::responseComplete() { responseSem.post(); }

void Shmemaphore::sendString(const std::string &message)
{
  uint64_t messageSize = message.size();
  setHeader(&messageSize, sizeof(messageSize));
  setData(message.c_str(), messageSize);

  if (owner)
    requestComplete();
  else
    responseComplete();
}

std::string Shmemaphore::recvString()
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
