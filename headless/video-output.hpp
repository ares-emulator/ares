#pragma once

#include <ares/ares.hpp>
#include <nall/string.hpp>
#include <vector>

namespace headless {

enum class SaveFrameResult : u32 {
  Success,
  NoFrameCaptured,
  EncodeFailed,
};

auto resampleNearest(
  const std::vector<u32>& source,
  u32 sourceWidth,
  u32 sourceHeight,
  u32 sourcePitch,
  u32 targetWidth,
  u32 targetHeight
) -> std::vector<u32>;

auto saveCapturedFramePng(
  const nall::string& outputPath,
  const std::vector<u32>& frame,
  u32 pitch,
  u32 width,
  u32 height,
  const std::vector<ares::Node::Video::Screen>& screens
) -> SaveFrameResult;

}
