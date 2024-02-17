use crate::error;
use crate::error::FilterChainError;
use icrate::Foundation::{NSRange, NSString};
use icrate::Metal::{
    MTLBuffer, MTLDevice, MTLResource, MTLResourceOptions, MTLResourceStorageModeManaged,
    MTLResourceStorageModeShared,
};
use objc2::rc::Id;
use objc2::runtime::ProtocolObject;
use std::ops::{Deref, DerefMut};

pub struct MetalBuffer {
    buffer: Id<ProtocolObject<dyn MTLBuffer>>,
    size: usize,
    storage_mode: MTLResourceOptions,
}

impl AsRef<ProtocolObject<dyn MTLBuffer>> for MetalBuffer {
    fn as_ref(&self) -> &ProtocolObject<dyn MTLBuffer> {
        self.buffer.as_ref()
    }
}

impl MetalBuffer {
    pub fn new(
        device: &ProtocolObject<dyn MTLDevice>,
        mut size: usize,
        label: &str,
    ) -> error::Result<Self> {
        let storage_mode = if cfg!(all(target_arch = "aarch64", target_vendor = "apple")) {
            MTLResourceStorageModeShared
        } else {
            MTLResourceStorageModeManaged
        };

        // Can't create buffer of size 0.
        if size == 0 {
            size = 16;
        };

        let buffer = device
            .newBufferWithLength_options(size, storage_mode)
            .ok_or(FilterChainError::BufferError)?;

        buffer.setLabel(Some(&*NSString::from_str(label)));

        Ok(Self {
            buffer,
            size,
            storage_mode,
        })
    }

    pub fn flush(&self) {
        // We don't know what was actually written to so...
        if self.storage_mode == MTLResourceStorageModeManaged {
            self.buffer.didModifyRange(NSRange {
                location: 0,
                length: self.size,
            })
        }
    }
}

impl Deref for MetalBuffer {
    type Target = [u8];

    fn deref(&self) -> &Self::Target {
        // SAFETY: the lifetime of this reference must be longer than of the MetalBuffer.
        // Additionally, `MetalBuffer.buffer` is never lent out directly
        unsafe { std::slice::from_raw_parts(self.buffer.contents().as_ptr().cast(), self.size) }
    }
}

impl DerefMut for MetalBuffer {
    fn deref_mut(&mut self) -> &mut Self::Target {
        // SAFETY: the lifetime of this reference must be longer than of the MetalBuffer.
        // Additionally, `MetalBuffer.buffer` is never lent out directly
        unsafe { std::slice::from_raw_parts_mut(self.buffer.contents().as_ptr().cast(), self.size) }
    }
}
