#include <errno.h>

#include <random>

#include "common.h"
#include "stb_image_write.h"

int main(int argc, char **argv) {
  Semaphore server_sem(SEMNAME_SERVER, 0);
  Semaphore client_sem(SEMNAME_CLIENT, 0);

  // command to send to client (loads an image)
  std::vector<std::string> commands = {"apple", "ball", "car", "dog"};
  std::mt19937 gen{std::random_device{}()};
  std::uniform_int_distribution<size_t> rng{0, commands.size() - 1};

  for (int i = 0; i < 10; i++) {
    std::string command = commands[rng(gen)];
    std::cout << "I want " << command << std::endl;

    // payload for the NumBytes shared memory segment
    uint64_t dataSize = command.size();

    // describes the size of the data segment
    SharedMemorySegment shmNumBytes(SHMNAME_HEADER, sizeof(uint64_t), true);
    shmNumBytes.map();
    shmNumBytes.setData(&dataSize, sizeof(dataSize));
    shmNumBytes.unmap();

    // create the data segment and pass the command
    SharedMemorySegment shmData(SHMNAME_DATA, dataSize, true);
    shmData.map();
    shmData.setData(command.c_str(), dataSize);
    shmData.unmap();

    // increment the semaphore so the client process can proceed
    server_sem.post();
    client_sem.wait();

    shmNumBytes.map(4 * sizeof(uint64_t));
    uint64_t imageBytes[4];
    std::memcpy(imageBytes, shmNumBytes.getData(), 4 * sizeof(uint64_t));
    shmNumBytes.unmap();

    std::cout << "I'm receiving " << imageBytes[0] << " bytes, "
              << imageBytes[1] << "x" << imageBytes[2] << std::endl;

    shmData.map(imageBytes[0]);
    unsigned char *image = (unsigned char *)shmData.getData();
    std::string filename = command + ".jpg";
    stbi_write_jpg(filename.c_str(), imageBytes[1], imageBytes[2],
                   imageBytes[3], image, 90);
    shmData.unmap();
  }

  return 0;
}
