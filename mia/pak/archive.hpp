#pragma once

#include <memory>
#include <vector>

#include <nall/nall.hpp>

struct Archive {
  virtual ~Archive() = default;
  virtual auto entries() const -> const std::vector<nall::string>& = 0;
  virtual auto extract(u32 index) -> std::vector<u8> = 0;
};

auto isArchive(const nall::string& location) -> bool;
auto openArchive(const nall::string& location) -> std::shared_ptr<Archive>;

auto archiveExtractFirstMatching(const std::shared_ptr<Archive>& archive, const std::vector<nall::string>& match) -> std::vector<u8>;
auto archiveExtractByName(const std::shared_ptr<Archive>& archive, const nall::string& name) -> std::vector<u8>;
