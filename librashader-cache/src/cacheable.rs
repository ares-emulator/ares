/// Trait for objects that are cacheable.
pub trait Cacheable {
    fn from_bytes(bytes: &[u8]) -> Option<Self>
    where
        Self: Sized;

    fn to_bytes(&self) -> Option<Vec<u8>>;
}

impl Cacheable for Vec<u8> {
    fn from_bytes(bytes: &[u8]) -> Option<Self> {
        Some(Vec::from(bytes))
    }

    fn to_bytes(&self) -> Option<Vec<u8>> {
        Some(self.to_vec())
    }
}
