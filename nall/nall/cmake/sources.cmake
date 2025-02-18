target_sources(
  nall
  PRIVATE
    adaptive-array.hpp
    algorithm.hpp
    any.hpp
    arguments.hpp
    arithmetic.hpp
    array-span.hpp
    array-view.hpp
    array.hpp
    atoi.hpp
    bcd.hpp
    bit.hpp
    bump-allocator.hpp
    case-range.hpp
    cd.hpp
    chrono.hpp
    counting-sort.hpp
    directory.cpp
    directory.hpp
    dl.cpp
    dl.hpp
    endian.hpp
    file-buffer.hpp
    file-map.cpp
    file-map.hpp
    file.hpp
    float-env.hpp
    function.hpp
    galois-field.hpp
    hashset.hpp
    hid.hpp
    image.hpp
    induced-sort.hpp
    inline-if.hpp
    inode.cpp
    inode.hpp
    instance.hpp
    interpolation.hpp
    intrinsics.hpp
    ips.hpp
    iterator.hpp
    literals.hpp
    locale.hpp
    location.hpp
    main.hpp
    map.hpp
    matrix-multiply.hpp
    matrix.hpp
    maybe.hpp
    memory.cpp
    memory.hpp
    merge-sort.hpp
    nall.cpp
    nall.hpp
    path.cpp
    path.hpp
    platform.cpp
    platform.hpp
    pointer.hpp
    primitives.hpp
    priority-queue.hpp
    property.hpp
    queue.hpp
    random.cpp
    random.hpp
    range.hpp
    reed-solomon.hpp
    run.cpp
    run.hpp
    serial.hpp
    serializer.hpp
    service.hpp
    set.hpp
    shared-memory.hpp
    shared-pointer.hpp
    stdint.hpp
    string.hpp
    suffix-array.hpp
    terminal.cpp
    terminal.hpp
    thread.cpp
    thread.hpp
    traits.hpp
    unique-pointer.hpp
    utility.hpp
    variant.hpp
    varint.hpp
    vector.hpp
    vfs.hpp
    view.hpp
)

target_sources(nall PRIVATE arithmetic/barrett.hpp arithmetic/natural.hpp arithmetic/unsigned.hpp)

target_sources(nall PRIVATE beat/single/apply.hpp beat/single/create.hpp)

target_sources(
  nall
  PRIVATE cd/crc16.hpp cd/edc.hpp cd/efm.hpp cd/rspc.hpp cd/scrambler.hpp cd/session.hpp cd/sync.hpp
)

target_sources(nall PRIVATE cipher/chacha20.hpp)

target_sources(nall PRIVATE database/odbc.hpp database/sqlite3.hpp)

target_sources(
  nall
  PRIVATE
    decode/base.hpp
    decode/base64.hpp
    decode/bmp.hpp
    decode/bwt.hpp
    decode/chd.hpp
    decode/cue.hpp
    decode/gzip.hpp
    decode/html.hpp
    decode/huffman.hpp
    decode/inflate.hpp
    decode/lzsa.hpp
    decode/mtf.hpp
    decode/png.hpp
    decode/rle.hpp
    decode/url.hpp
    decode/wav.hpp
    decode/zip.hpp
)

target_sources(nall PRIVATE dsp/iir/biquad.hpp dsp/iir/dc-removal.hpp dsp/iir/one-pole.hpp dsp/resampler/cubic.hpp)

target_sources(
  nall
  PRIVATE
    elliptic-curve/curve25519.hpp
    elliptic-curve/ed25519.hpp
    elliptic-curve/modulo25519-optimized.hpp
    elliptic-curve/modulo25519-reference.hpp
)

target_sources(nall PRIVATE emulation/21fx.hpp)

target_sources(
  nall
  PRIVATE
    encode/base.hpp
    encode/base64.hpp
    encode/bmp.hpp
    encode/bwt.hpp
    encode/html.hpp
    encode/huffman.hpp
    encode/lzsa.hpp
    encode/mtf.hpp
    encode/png.hpp
    encode/rle.hpp
    encode/url.hpp
    encode/wav.hpp
    encode/zip.hpp
)

target_sources(nall PRIVATE gdb/Readme.md gdb/server.cpp gdb/server.hpp gdb/watchpoint.hpp)

target_sources(
  nall
  PRIVATE
    hash/crc16.hpp
    hash/crc32.hpp
    hash/crc64.hpp
    hash/hash.hpp
    hash/sha224.hpp
    hash/sha256.hpp
    hash/sha384.hpp
    hash/sha512.hpp
)

target_sources(
  nall
  PRIVATE
    image/blend.hpp
    image/core.hpp
    image/fill.hpp
    image/interpolation.hpp
    image/load.hpp
    image/multifactor.hpp
    image/scale.hpp
    image/static.hpp
    image/utility.hpp
)

target_sources(nall PRIVATE mac/poly1305.hpp)

target_sources(nall PRIVATE posix/service.hpp posix/shared-memory.hpp)

target_sources(
  nall
  PRIVATE
    primitives/bit-field.hpp
    primitives/bit-range.hpp
    primitives/boolean.hpp
    primitives/integer.hpp
    primitives/literals.hpp
    primitives/natural.hpp
    primitives/real.hpp
    primitives/types.hpp
)

target_sources(nall PRIVATE queue/spsc.hpp queue/st.hpp)

target_sources(
  nall
  PRIVATE
    recompiler/amd64/amd64.hpp
    recompiler/amd64/constants.hpp
    recompiler/amd64/emitter.hpp
    recompiler/amd64/encoder-calls-systemv.hpp
    recompiler/amd64/encoder-calls-windows.hpp
    recompiler/amd64/encoder-instructions.hpp
    recompiler/generic/constants.hpp
    recompiler/generic/encoder-calls.hpp
    recompiler/generic/encoder-instructions.hpp
    recompiler/generic/generic.hpp
)

target_sources(
  nall
  PRIVATE
    string/atoi.hpp
    string/cast.hpp
    string/compare.hpp
    string/convert.hpp
    string/core.hpp
    string/find.hpp
    string/format.hpp
    string/match.hpp
    string/pascal.hpp
    string/replace.hpp
    string/split.hpp
    string/trim.hpp
    string/utf8.hpp
    string/utility.hpp
    string/vector.hpp
    string/view.hpp
    string/allocator/adaptive.hpp
    string/allocator/copy-on-write.hpp
    string/allocator/small-string-optimization.hpp
    string/allocator/vector.hpp
    string/eval/evaluator.hpp
    string/eval/literal.hpp
    string/eval/node.hpp
    string/eval/parser.hpp
    string/markup/bml.hpp
    string/markup/find.hpp
    string/markup/node.hpp
    string/markup/xml.hpp
)

target_sources(
  nall
  PRIVATE tcptext/tcp-socket.cpp tcptext/tcp-socket.hpp tcptext/tcptext-server.cpp tcptext/tcptext-server.hpp
)

target_sources(
  nall
  PRIVATE
    vector/access.hpp
    vector/assign.hpp
    vector/compare.hpp
    vector/core.hpp
    vector/iterator.hpp
    vector/memory.hpp
    vector/modify.hpp
    vector/utility.hpp
    vector/specialization/u8.hpp
)

target_sources(
  nall
  PRIVATE
    vfs/attribute.hpp
    vfs/cdrom.hpp
    vfs/directory.hpp
    vfs/disk.hpp
    vfs/file.hpp
    vfs/memory.hpp
    vfs/node.hpp
    vfs/vfs.hpp
)

target_sources(nall PRIVATE cmake/os-macos.cmake cmake/os-windows.cmake cmake/os-linux.cmake cmake/os-freebsd.cmake)

target_sources(nall PRIVATE cmake/sources.cmake)
