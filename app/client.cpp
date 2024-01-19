#include "common.h"
#include "stb_image.h"

int main(int argc, char **argv) {
  Shmemaphore shma4("ibis");

  for (int i = 0; i < 10; i++) {
    shma4.waitForRequest();

    uint64_t dataSize = 0;
    dataSize = *((uint64_t *)shma4.getHeader(sizeof(uint64_t)));

    // there is no guaranteed \0 at the end of the data, so we need to specify
    // the size to the string ctor
    const char *rawData = (const char *)shma4.getData(dataSize);
    std::string command(rawData, dataSize);

    std::cout << "Server wants " << command << std::endl;
    std::string fname = "img/" + command + ".jpg";

    int w, h, c;
    unsigned char *image = stbi_load(fname.c_str(), &w, &h, &c, 0);
    size_t imageBytes = w * h * c * sizeof(unsigned char);
    std::vector<uint64_t> imageDims = {imageBytes, (uint64_t)w, (uint64_t)h,
                                       (uint64_t)c};
    std::cout << "I'm sending " << imageBytes << " bytes" << std::endl;

    shma4.setHeader((void *)imageDims.data(),
                    imageDims.size() * sizeof(uint64_t));
    shma4.setData(image, imageBytes);

    stbi_image_free(image);

    shma4.responseComplete();
  }

  return 0;
}
