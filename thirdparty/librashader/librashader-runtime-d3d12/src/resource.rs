use std::mem::ManuallyDrop;
use std::ops::Deref;
use windows::Win32::Graphics::Direct3D12::ID3D12Resource;

pub trait ResourceHandleStrategy<H> {
    const NEEDS_CLEANUP: bool;
    unsafe fn obtain(handle: &H) -> ManuallyDrop<Option<ID3D12Resource>>;

    /// Run a clean-up handler if the resource type needs clean up.
    fn cleanup_handler<F: FnOnce()>(cleanup_fn: F) {
        if Self::NEEDS_CLEANUP {
            cleanup_fn();
        }
    }
}

pub trait ObtainResourceHandle {
    fn handle(&self) -> &ID3D12Resource;
}

impl ObtainResourceHandle for ID3D12Resource {
    fn handle(&self) -> &ID3D12Resource {
        &self
    }
}

impl ObtainResourceHandle for ManuallyDrop<ID3D12Resource> {
    fn handle(&self) -> &ID3D12Resource {
        self.deref()
    }
}

// this isn't actually used anymore but keep it around just in case.
pub struct IncrementRefcount;

impl ResourceHandleStrategy<ManuallyDrop<ID3D12Resource>> for IncrementRefcount {
    const NEEDS_CLEANUP: bool = true;

    unsafe fn obtain(
        handle: &ManuallyDrop<ID3D12Resource>,
    ) -> ManuallyDrop<Option<ID3D12Resource>> {
        ManuallyDrop::new(Some(handle.deref().clone()))
    }
}

impl ResourceHandleStrategy<ID3D12Resource> for IncrementRefcount {
    const NEEDS_CLEANUP: bool = true;

    unsafe fn obtain(handle: &ID3D12Resource) -> ManuallyDrop<Option<ID3D12Resource>> {
        ManuallyDrop::new(Some(handle.clone()))
    }
}

pub struct OutlivesFrame;

impl ResourceHandleStrategy<ManuallyDrop<ID3D12Resource>> for OutlivesFrame {
    const NEEDS_CLEANUP: bool = false;

    unsafe fn obtain(
        handle: &ManuallyDrop<ID3D12Resource>,
    ) -> ManuallyDrop<Option<ID3D12Resource>> {
        unsafe { std::mem::transmute_copy(handle) }
    }
}
