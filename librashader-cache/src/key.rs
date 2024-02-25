/// Trait for objects that can be used as part of a key for a cached object.
pub trait CacheKey {
    /// Get a byte representation of the object that
    /// will be fed into the hash.
    fn hash_bytes(&self) -> &[u8];
}

impl CacheKey for u32 {
    fn hash_bytes(&self) -> &[u8] {
        &bytemuck::bytes_of(&*self)
    }
}

impl CacheKey for i32 {
    fn hash_bytes(&self) -> &[u8] {
        &bytemuck::bytes_of(&*self)
    }
}

impl CacheKey for &[u8] {
    fn hash_bytes(&self) -> &[u8] {
        self
    }
}

impl CacheKey for Vec<u8> {
    fn hash_bytes(&self) -> &[u8] {
        &self
    }
}

impl CacheKey for Vec<u32> {
    fn hash_bytes(&self) -> &[u8] {
        bytemuck::cast_slice(&self)
    }
}

impl CacheKey for &str {
    fn hash_bytes(&self) -> &[u8] {
        // need to be explicit
        self.as_bytes()
    }
}
