/// A hashmap optimized for small sets of size less than 32 with a fast hash implementation.
///
/// Used widely for shader reflection.
pub type FastHashMap<K, V> =
    halfbrown::SizedHashMap<K, V, core::hash::BuildHasherDefault<rustc_hash::FxHasher>, 32>;

/// A string with small string optimizations up to 23 bytes.
pub type ShortString = smartstring::SmartString<smartstring::LazyCompact>;
