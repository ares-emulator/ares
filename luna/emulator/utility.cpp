#include <nall/encode/png.hpp>

auto Emulator::captureScreenshot(const uint32_t* data, uint pitch, uint width, uint height) -> void {
  string filename{Path::desktop(), "luna ", chrono::local::datetime().transform(":", "-"), ".png"};
  Encode::PNG::RGB8(filename, data, pitch, width, height);
  showMessage("Captured screenshot");
}
