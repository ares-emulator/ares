#include "video-output.hpp"
#include <nall/encode/png.hpp>

using namespace nall;

namespace headless {

auto resampleNearest(
  const std::vector<u32>& source,
  u32 sourceWidth,
  u32 sourceHeight,
  u32 sourcePitch,
  u32 targetWidth,
  u32 targetHeight
) -> std::vector<u32> {
  std::vector<u32> output;
  if(!sourceWidth || !sourceHeight || !targetWidth || !targetHeight) return output;

  output.resize((size_t)targetWidth * targetHeight);
  auto sourceStride = sourcePitch >> 2;
  for(u32 y : range(targetHeight)) {
    auto sy = (u64)y * sourceHeight / targetHeight;
    for(u32 x : range(targetWidth)) {
      auto sx = (u64)x * sourceWidth / targetWidth;
      output[(size_t)y * targetWidth + x] = source[(size_t)sy * sourceStride + sx];
    }
  }
  return output;
}

auto saveCapturedFramePng(
  const string& outputPath,
  const std::vector<u32>& frame,
  u32 pitch,
  u32 width,
  u32 height,
  const std::vector<ares::Node::Video::Screen>& screens
) -> SaveFrameResult {
  if(frame.empty() || pitch == 0 || height == 0) return SaveFrameResult::NoFrameCaptured;

  u32 targetWidth = width;
  u32 targetHeight = height;
  if(!screens.empty() && screens[0]) {
    targetWidth = screens[0]->width() * screens[0]->scaleX();
    targetHeight = screens[0]->height() * screens[0]->scaleY();
    if(!targetWidth) targetWidth = width;
    if(!targetHeight) targetHeight = height;
  }

  const u32* pngData = frame.data();
  u32 pngPitch = pitch;
  u32 pngWidth = width;
  u32 pngHeight = height;

  auto resized = std::vector<u32>{};
  if(targetWidth != width || targetHeight != height) {
    resized = resampleNearest(frame, width, height, pitch, targetWidth, targetHeight);
    if(!resized.empty()) {
      pngData = resized.data();
      pngPitch = targetWidth * sizeof(u32);
      pngWidth = targetWidth;
      pngHeight = targetHeight;
    }
  }

  if(!Encode::PNG::RGB8(outputPath, pngData, pngPitch, pngWidth, pngHeight)) return SaveFrameResult::EncodeFailed;
  return SaveFrameResult::Success;
}

}
