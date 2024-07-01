auto Program::refreshRateHint(ares::Node::Video::Screen node, double refreshRate) -> void {
  ruby::video.refreshRateHint(node, refreshRate);
}

auto Program::resetPalette(ares::Node::Video::Screen node) -> void {
  ruby::video.resetPalette(node);
}

auto Program::resetSprites(ares::Node::Video::Screen node) -> void {
  ruby::video.resetSprites(node);
}

auto Program::setViewport(ares::Node::Video::Screen node, u32 x, u32 y, u32 width, u32 height) -> void {
  ruby::video.setViewport(node, x, y, width, height);
}

auto Program::setOverscan(ares::Node::Video::Screen node, bool overscan) -> void {
  ruby::video.setOverscan(node, overscan);
}

auto Program::setSize(ares::Node::Video::Screen node, u32 width, u32 height) -> void {
  ruby::video.setSize(node, width, height);
}

auto Program::setScale(ares::Node::Video::Screen node, f64 scaleX, f64 scaleY) -> void {
  ruby::video.setScale(node, scaleX, scaleY);
}

auto Program::setAspect(ares::Node::Video::Screen node, f64 aspectX, f64 aspectY) -> void {
  ruby::video.setAspect(node, aspectX, aspectY);
}

auto Program::setSaturation(ares::Node::Video::Screen node, f64 saturation) -> void {
  ruby::video.setSaturation(node, saturation);
}

auto Program::setGamma(ares::Node::Video::Screen node, f64 gamma) -> void {
  ruby::video.setGamma(node, gamma);
}

auto Program::setLuminance(ares::Node::Video::Screen node, f64 luminance) -> void {
  ruby::video.setLuminance(node, luminance);
}

auto Program::setFillColor(ares::Node::Video::Screen node, u32 fillColor) -> void {
  ruby::video.setFillColor(node, fillColor);
}

auto Program::setColorBleed(ares::Node::Video::Screen node, bool colorBleed) -> void {
  ruby::video.setColorBleed(node, colorBleed);
}

auto Program::setColorBleedWidth(ares::Node::Video::Screen node, u32 width) -> void {
  ruby::video.setColorBleedWidth(node, width);
}

auto Program::setInterframeBlending(ares::Node::Video::Screen node, bool interframeBlending) -> void {
  ruby::video.setInterframeBlending(node, interframeBlending);
}

auto Program::setRotation(ares::Node::Video::Screen node, u32 rotation) -> void {
  ruby::video.setRotation(node, rotation);
}

auto Program::setProgressive(ares::Node::Video::Screen node, bool progressiveDouble) -> void {
  ruby::video.setProgressive(node, progressiveDouble);
}

auto Program::setInterlace(ares::Node::Video::Screen node, bool interlaceField) -> void {
  ruby::video.setInterlace(node, interlaceField);
}

auto Program::attachSprite(ares::Node::Video::Screen node, ares::Node::Video::Sprite sprite) -> void {
  ruby::video.attachSprite(node, sprite);
}

auto Program::detachSprite(ares::Node::Video::Screen node, ares::Node::Video::Sprite sprite) -> void {
  ruby::video.detachSprite(node, sprite);
}

auto Program::colors(ares::Node::Video::Screen node, u32 colors, function<n64 (n32)> color) -> void {
  ruby::video.colors(node, colors, color);
}
