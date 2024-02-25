/// General trait for ring buffers.
pub trait RingBuffer<T> {
    /// Get a borrow the current item.
    fn current(&self) -> &T;

    /// Get a mutable borrow to the current item.
    fn current_mut(&mut self) -> &mut T;

    /// Move to the next item in the ring buffer.
    fn next(&mut self);

    fn current_index(&self) -> usize;
}

impl<T, const SIZE: usize> RingBuffer<T> for InlineRingBuffer<T, SIZE> {
    fn current(&self) -> &T {
        &self.items[self.index]
    }

    fn current_mut(&mut self) -> &mut T {
        &mut self.items[self.index]
    }

    fn next(&mut self) {
        self.index += 1;
        if self.index >= SIZE {
            self.index = 0
        }
    }

    fn current_index(&self) -> usize {
        self.index
    }
}

/// A ring buffer that stores its contents inline.
pub struct InlineRingBuffer<T, const SIZE: usize> {
    items: [T; SIZE],
    index: usize,
}

impl<T, const SIZE: usize> InlineRingBuffer<T, SIZE>
where
    T: Copy,
    T: Default,
{
    pub fn new() -> Self {
        Self {
            items: [T::default(); SIZE],
            index: 0,
        }
    }

    /// Get a borrow to all the items in this ring buffer.
    pub fn items(&self) -> &[T; SIZE] {
        &self.items
    }

    /// Get a mutable borrow to all the items in this ring buffer.
    pub fn items_mut(&mut self) -> &mut [T; SIZE] {
        &mut self.items
    }
}

/// A ring buffer that stores its contents in a box
pub struct BoxRingBuffer<T> {
    items: Box<[T]>,
    index: usize,
}

impl<T> BoxRingBuffer<T>
where
    T: Copy,
    T: Default,
{
    pub fn new(size: usize) -> Self {
        Self {
            items: vec![T::default(); size].into_boxed_slice(),
            index: 0,
        }
    }
}

impl<T> BoxRingBuffer<T> {
    pub fn from_vec(items: Vec<T>) -> Self {
        Self {
            items: items.into_boxed_slice(),
            index: 0,
        }
    }

    /// Get a borrow to all the items in this ring buffer.
    pub fn items(&self) -> &[T] {
        &self.items
    }

    /// Get a mutable borrow to all the items in this ring buffer.
    pub fn items_mut(&mut self) -> &mut [T] {
        &mut self.items
    }
}

impl<T> From<Vec<T>> for BoxRingBuffer<T> {
    fn from(value: Vec<T>) -> Self {
        BoxRingBuffer::from_vec(value)
    }
}

impl<T> RingBuffer<T> for BoxRingBuffer<T> {
    fn current(&self) -> &T {
        &self.items[self.index]
    }

    fn current_mut(&mut self) -> &mut T {
        &mut self.items[self.index]
    }

    fn next(&mut self) {
        self.index += 1;
        if self.index >= self.items.len() {
            self.index = 0
        }
    }

    fn current_index(&self) -> usize {
        self.index
    }
}
