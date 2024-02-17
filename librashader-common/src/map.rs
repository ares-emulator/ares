/// Fast optimized hash map type for small sets.
pub type FastHashMap<K, V> =
    halfbrown::SizedHashMap<K, V, core::hash::BuildHasherDefault<rustc_hash::FxHasher>, 32>;

pub use halfbrown;
