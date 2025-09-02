#pragma once

#include <nall/platform.hpp>
#include <nall/chrono.hpp>
#include <nall/directory.hpp>
#include <nall/file.hpp>
#include <nall/function.hpp>
#include <nall/hid.hpp>
#include <nall/image.hpp>
#include <nall/matrix.hpp>
#include <nall/matrix-multiply.hpp>
#include <nall/queue.hpp>
#include <nall/range.hpp>
#include <nall/ranges-helpers.hpp>
#include <nall/set.hpp>
#include <nall/shared-pointer.hpp>
#include <nall/string.hpp>
#include <nall/thread.hpp>
#include <nall/unique-pointer.hpp>
#include <vector>
#include <ranges>
#include <nall/dsp/resampler/cubic.hpp>
#include <nall/hash/crc32.hpp>

using nall::atomic;
using nall::function;
using nall::index_of;
using nall::lock_guard;
using nall::mutex;
using nall::queue;
using nall::recursive_mutex;
using nall::shared_pointer;
using nall::string;
using nall::tuple;
using nall::unique_pointer;

namespace ruby {

#include <ruby/video/video.hpp>
#include <ruby/audio/audio.hpp>
#include <ruby/input/input.hpp>

}
