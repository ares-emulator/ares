use std::ops::Deref;

pub enum Handle<'a, T> {
    Borrowed(&'a T),
    Owned(T),
}

impl<T> Deref for Handle<'_, T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        match self {
            Handle::Borrowed(r) => &r,
            Handle::Owned(r) => &r,
        }
    }
}
