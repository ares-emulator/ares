//! librashader instance version helpers.

/// API version type alias.
pub type LIBRASHADER_API_VERSION = usize;
/// ABI version type alias.
pub type LIBRASHADER_ABI_VERSION = usize;

/// The current version of the librashader API.
/// Pass this into `version` for config structs.
///
/// API versions are backwards compatible. It is valid to load
/// a librashader C API instance for all API versions less than
/// or equal to LIBRASHADER_CURRENT_VERSION, and subsequent API
/// versions must remain backwards compatible.
/// ## API Versions
/// - API version 0: 0.1.0
/// - API version 1: 0.2.0
///     - Added rotation, total_subframes, current_subframes to frame options
///     - Added preset context API
///     - Added Metal runtime API
pub const LIBRASHADER_CURRENT_VERSION: LIBRASHADER_API_VERSION = 1;

/// The current version of the librashader ABI.
/// Used by the loader to check ABI compatibility.
///
/// ABI version 0 is reserved as a sentinel value.
///
/// ABI versions are not backwards compatible. It is not
/// valid to load a librashader C API instance for any ABI
/// version not equal to LIBRASHADER_CURRENT_ABI.
///
/// ## ABI Versions
/// - ABI version 0: null instance (unloaded)
/// - ABI version 1: 0.1.0
/// - ABI version 2: 0.5.0
///     - Reduced texture size information needed for some runtimes.
///     - Removed wrapper structs for Direct3D 11 SRV and RTV handles.
///     - Removed `gl_context_init`.
///     - Make viewport handling consistent across runtimes, which are now
///       span the output render target if omitted.
pub const LIBRASHADER_CURRENT_ABI: LIBRASHADER_ABI_VERSION = 2;

/// Function pointer definition for libra_abi_version
pub type PFN_libra_instance_abi_version = extern "C" fn() -> LIBRASHADER_ABI_VERSION;
/// Get the ABI version of the loaded instance.
#[no_mangle]
pub extern "C" fn libra_instance_abi_version() -> LIBRASHADER_ABI_VERSION {
    LIBRASHADER_CURRENT_ABI
}

/// Function pointer definition for libra_abi_version
pub type PFN_libra_instance_api_version = extern "C" fn() -> LIBRASHADER_API_VERSION;
/// Get the API version of the loaded instance.
#[no_mangle]
pub extern "C" fn libra_instance_api_version() -> LIBRASHADER_API_VERSION {
    LIBRASHADER_CURRENT_VERSION
}
