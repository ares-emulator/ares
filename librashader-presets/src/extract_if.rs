//! This is a stable polyfill for [`Vec::extract_if`](https://github.com/rust-lang/rust/issues/43244).

use core::ptr;
use core::slice;

/// Polyfill trait for [`Vec::extract_if`](https://github.com/rust-lang/rust/issues/43244).
pub(crate) trait MakeExtractIf<T> {
    /// Creates an iterator which uses a closure to determine if an element should be removed.
    ///
    /// If the closure returns true, then the element is removed and yielded.
    /// If the closure returns false, the element will remain in the vector and will not be yielded
    /// by the iterator.
    ///
    /// If the returned `ExtractIf` is not exhausted, e.g. because it is dropped without iterating
    /// or the iteration short-circuits, then the remaining elements will be retained.
    ///
    /// Note that `extract_if` also lets you mutate every element in the filter closure,
    /// regardless of whether you choose to keep or remove it.
    ///
    /// # Examples
    ///
    /// Splitting an array into evens and odds, reusing the original allocation:
    ///
    /// ```
    /// use vec_extract_if_polyfill::MakeExtractIf;
    /// let mut numbers = vec![1, 2, 3, 4, 5, 6, 8, 9, 11, 13, 14, 15];
    ///
    /// let evens = numbers.extract_if(|x| *x % 2 == 0).collect::<Vec<_>>();
    /// let odds = numbers;
    ///
    /// assert_eq!(evens, vec![2, 4, 6, 8, 14]);
    /// assert_eq!(odds, vec![1, 3, 5, 9, 11, 13, 15]);
    /// ```
    fn extract_if<F>(&mut self, filter: F) -> ExtractIf<T, F>
    where
        F: FnMut(&mut T) -> bool;
}

impl<T> MakeExtractIf<T> for Vec<T> {
    fn extract_if<F>(&mut self, filter: F) -> ExtractIf<T, F>
    where
        F: FnMut(&mut T) -> bool,
    {
        let old_len = self.len();

        // Guard against us getting leaked (leak amplification)
        unsafe {
            self.set_len(0);
        }

        ExtractIf {
            vec: self,
            idx: 0,
            del: 0,
            old_len,
            pred: filter,
        }
    }
}
/// An iterator which uses a closure to determine if an element should be removed.
///
/// This struct is created by [`Vec::extract_if`].
/// See its documentation for more.
///
/// # Example
///
/// ```
/// use vec_extract_if_polyfill::MakeExtractIf;
///
/// let mut v = vec![0, 1, 2];
/// let iter = v.extract_if(|x| *x % 2 == 0);
/// ```
#[derive(Debug)]
#[must_use = "iterators are lazy and do nothing unless consumed"]
pub struct ExtractIf<'a, T, F>
where
    F: FnMut(&mut T) -> bool,
{
    vec: &'a mut Vec<T>,
    /// The index of the item that will be inspected by the next call to `next`.
    idx: usize,
    /// The number of items that have been drained (removed) thus far.
    del: usize,
    /// The original length of `vec` prior to draining.
    old_len: usize,
    /// The filter test predicate.
    pred: F,
}

impl<T, F> Iterator for ExtractIf<'_, T, F>
where
    F: FnMut(&mut T) -> bool,
{
    type Item = T;

    fn next(&mut self) -> Option<T> {
        unsafe {
            while self.idx < self.old_len {
                let i = self.idx;
                let v = slice::from_raw_parts_mut(self.vec.as_mut_ptr(), self.old_len);
                let drained = (self.pred)(&mut v[i]);
                // Update the index *after* the predicate is called. If the index
                // is updated prior and the predicate panics, the element at this
                // index would be leaked.
                self.idx += 1;
                if drained {
                    self.del += 1;
                    return Some(ptr::read(&v[i]));
                } else if self.del > 0 {
                    let del = self.del;
                    let src: *const T = &v[i];
                    let dst: *mut T = &mut v[i - del];
                    ptr::copy_nonoverlapping(src, dst, 1);
                }
            }
            None
        }
    }

    fn size_hint(&self) -> (usize, Option<usize>) {
        (0, Some(self.old_len - self.idx))
    }
}

impl<T, F> Drop for ExtractIf<'_, T, F>
where
    F: FnMut(&mut T) -> bool,
{
    fn drop(&mut self) {
        unsafe {
            if self.idx < self.old_len && self.del > 0 {
                // This is a pretty messed up state, and there isn't really an
                // obviously right thing to do. We don't want to keep trying
                // to execute `pred`, so we just backshift all the unprocessed
                // elements and tell the vec that they still exist. The backshift
                // is required to prevent a double-drop of the last successfully
                // drained item prior to a panic in the predicate.
                let ptr = self.vec.as_mut_ptr();
                let src = ptr.add(self.idx);
                let dst = src.sub(self.del);
                let tail_len = self.old_len - self.idx;
                src.copy_to(dst, tail_len);
            }
            self.vec.set_len(self.old_len - self.del);
        }
    }
}

#[cfg(test)]
mod test {
    use crate::extract_if::MakeExtractIf;
    #[test]
    fn drain_filter_empty() {
        let mut vec: Vec<i32> = vec![];

        {
            let mut iter = vec.extract_if(|_| true);
            assert_eq!(iter.size_hint(), (0, Some(0)));
            assert_eq!(iter.next(), None);
            assert_eq!(iter.size_hint(), (0, Some(0)));
            assert_eq!(iter.next(), None);
            assert_eq!(iter.size_hint(), (0, Some(0)));
        }
        assert_eq!(vec.len(), 0);
        assert_eq!(vec, vec![]);
    }

    #[test]
    fn drain_filter_zst() {
        let mut vec = vec![(), (), (), (), ()];
        let initial_len = vec.len();
        let mut count = 0;
        {
            let mut iter = vec.extract_if(|_| true);
            assert_eq!(iter.size_hint(), (0, Some(initial_len)));
            while let Some(_) = iter.next() {
                count += 1;
                assert_eq!(iter.size_hint(), (0, Some(initial_len - count)));
            }
            assert_eq!(iter.size_hint(), (0, Some(0)));
            assert_eq!(iter.next(), None);
            assert_eq!(iter.size_hint(), (0, Some(0)));
        }

        assert_eq!(count, initial_len);
        assert_eq!(vec.len(), 0);
        assert_eq!(vec, vec![]);
    }

    #[test]
    fn drain_filter_false() {
        let mut vec = vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

        let initial_len = vec.len();
        let mut count = 0;
        {
            let mut iter = vec.extract_if(|_| false);
            assert_eq!(iter.size_hint(), (0, Some(initial_len)));
            for _ in iter.by_ref() {
                count += 1;
            }
            assert_eq!(iter.size_hint(), (0, Some(0)));
            assert_eq!(iter.next(), None);
            assert_eq!(iter.size_hint(), (0, Some(0)));
        }

        assert_eq!(count, 0);
        assert_eq!(vec.len(), initial_len);
        assert_eq!(vec, vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
    }

    #[test]
    fn drain_filter_true() {
        let mut vec = vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

        let initial_len = vec.len();
        let mut count = 0;
        {
            let mut iter = vec.extract_if(|_| true);
            assert_eq!(iter.size_hint(), (0, Some(initial_len)));
            while let Some(_) = iter.next() {
                count += 1;
                assert_eq!(iter.size_hint(), (0, Some(initial_len - count)));
            }
            assert_eq!(iter.size_hint(), (0, Some(0)));
            assert_eq!(iter.next(), None);
            assert_eq!(iter.size_hint(), (0, Some(0)));
        }

        assert_eq!(count, initial_len);
        assert_eq!(vec.len(), 0);
        assert_eq!(vec, vec![]);
    }

    #[test]
    fn drain_filter_complex() {
        {
            //                [+xxx++++++xxxxx++++x+x++]
            let mut vec = vec![
                1, 2, 4, 6, 7, 9, 11, 13, 15, 17, 18, 20, 22, 24, 26, 27, 29, 31, 33, 34, 35, 36,
                37, 39,
            ];

            let removed = vec.extract_if(|x| *x % 2 == 0).collect::<Vec<_>>();
            assert_eq!(removed.len(), 10);
            assert_eq!(removed, vec![2, 4, 6, 18, 20, 22, 24, 26, 34, 36]);

            assert_eq!(vec.len(), 14);
            assert_eq!(
                vec,
                vec![1, 7, 9, 11, 13, 15, 17, 27, 29, 31, 33, 35, 37, 39]
            );
        }

        {
            //                [xxx++++++xxxxx++++x+x++]
            let mut vec = vec![
                2, 4, 6, 7, 9, 11, 13, 15, 17, 18, 20, 22, 24, 26, 27, 29, 31, 33, 34, 35, 36, 37,
                39,
            ];

            let removed = vec.extract_if(|x| *x % 2 == 0).collect::<Vec<_>>();
            assert_eq!(removed.len(), 10);
            assert_eq!(removed, vec![2, 4, 6, 18, 20, 22, 24, 26, 34, 36]);

            assert_eq!(vec.len(), 13);
            assert_eq!(vec, vec![7, 9, 11, 13, 15, 17, 27, 29, 31, 33, 35, 37, 39]);
        }

        {
            //                [xxx++++++xxxxx++++x+x]
            let mut vec = vec![
                2, 4, 6, 7, 9, 11, 13, 15, 17, 18, 20, 22, 24, 26, 27, 29, 31, 33, 34, 35, 36,
            ];

            let removed = vec.extract_if(|x| *x % 2 == 0).collect::<Vec<_>>();
            assert_eq!(removed.len(), 10);
            assert_eq!(removed, vec![2, 4, 6, 18, 20, 22, 24, 26, 34, 36]);

            assert_eq!(vec.len(), 11);
            assert_eq!(vec, vec![7, 9, 11, 13, 15, 17, 27, 29, 31, 33, 35]);
        }

        {
            //                [xxxxxxxxxx+++++++++++]
            let mut vec = vec![
                2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19,
            ];

            let removed = vec.extract_if(|x| *x % 2 == 0).collect::<Vec<_>>();
            assert_eq!(removed.len(), 10);
            assert_eq!(removed, vec![2, 4, 6, 8, 10, 12, 14, 16, 18, 20]);

            assert_eq!(vec.len(), 10);
            assert_eq!(vec, vec![1, 3, 5, 7, 9, 11, 13, 15, 17, 19]);
        }

        {
            //                [+++++++++++xxxxxxxxxx]
            let mut vec = vec![
                1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20,
            ];

            let removed = vec.extract_if(|x| *x % 2 == 0).collect::<Vec<_>>();
            assert_eq!(removed.len(), 10);
            assert_eq!(removed, vec![2, 4, 6, 8, 10, 12, 14, 16, 18, 20]);

            assert_eq!(vec.len(), 10);
            assert_eq!(vec, vec![1, 3, 5, 7, 9, 11, 13, 15, 17, 19]);
        }
    }
}
