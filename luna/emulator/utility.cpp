#include <nall/encode/bmp.hpp>

auto Emulator::captureScreenshot(const uint32_t* data, uint pitch, uint width, uint height) -> void {
  string filename{Path::desktop(), "luna ", chrono::local::datetime().transform(":", "-"), ".bmp"};
  Encode::BMP::create(filename, data, pitch, width, height, true);
  showMessage("Captured screenshot");
}
