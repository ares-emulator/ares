#pragma once

#include <nall/decode/zip.hpp>

namespace nall::Decode {

// Transitional semantic alias used by archive-backed disc code. ZIP itself
// implements Archive; no wrapper state or duplicate extraction path exists.
using ZIPArchive = ZIP;

}
