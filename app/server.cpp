#include <errno.h>

#include <random>

#include "common.h"
#include "stb_image_write.h"

int main(int argc, char **argv) {
  Shmemaphore shma4("ibis", true);

  // command to send to client (loads an image)
  std::vector<std::string> commands = {"apple", "ball", "car", "dog"};
  std::mt19937 gen{std::random_device{}()};
  std::uniform_int_distribution<size_t> rng{0, commands.size() - 1};

  for (int i = 0; i < 10; i++) {
    std::string command = commands[rng(gen)];
    std::cout << "I want " << command << std::endl;

    uint64_t dataSize = command.size();
    shma4.setHeader(&dataSize, sizeof(dataSize));
    shma4.setData(command.c_str(), dataSize);

    shma4.requestComplete();
    shma4.waitForResponse();

    size_t bytes = 4 * sizeof(uint64_t);
    uint64_t imageBytes[4];
    std::memcpy(imageBytes, shma4.getHeader(bytes), bytes);

    std::cout << "I'm receiving " << imageBytes[0] << " bytes, "
              << imageBytes[1] << "x" << imageBytes[2] << std::endl;

    // the data is already jpeg encoded binary data, so we can directly dump it
    // to disk
    unsigned char *image = (unsigned char *)shma4.getData(imageBytes[0]);
    std::string filename = command + ".jpg";
    stbi_write_jpg(filename.c_str(), imageBytes[1], imageBytes[2],
                   imageBytes[3], image, 90);
  }

  return 0;
}
