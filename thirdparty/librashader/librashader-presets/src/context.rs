// pub use librashader_presets_context::*;

//! Shader preset wildcard replacement context handling.
//!
//! Implements wildcard replacement of shader paths specified in
//! [RetroArch#15023](https://github.com/libretro/RetroArch/pull/15023).
use once_cell::sync::Lazy;
use regex::bytes::Regex;
use rustc_hash::FxHashMap;
use std::collections::VecDeque;
use std::fmt::{Debug, Display, Formatter};
use std::ops::Add;
use std::path::{Component, Path, PathBuf};

/// Valid video driver or runtime. This list is non-exhaustive.
#[repr(u32)]
#[non_exhaustive]
#[derive(Debug, Copy, Clone)]
pub enum VideoDriver {
    /// None  (`null`)
    None = 0,
    /// OpenGL Core (`glcore`)
    GlCore,
    /// Legacy OpenGL (`gl`)
    Gl,
    /// Vulkan (`vulkan`)
    Vulkan,
    /// Direct3D 9 (`d3d9_hlsl`)
    Direct3D9Hlsl,
    /// Direct3D 11 (`d3d11`)
    Direct3D11,
    /// Direct3D12 (`d3d12`)
    Direct3D12,
    /// Metal (`metal`)
    Metal,
}

impl Display for VideoDriver {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            VideoDriver::None => f.write_str("null"),
            VideoDriver::GlCore => f.write_str("glcore"),
            VideoDriver::Gl => f.write_str("gl"),
            VideoDriver::Vulkan => f.write_str("vulkan"),
            VideoDriver::Direct3D11 => f.write_str("d3d11"),
            VideoDriver::Direct3D9Hlsl => f.write_str("d3d9_hlsl"),
            VideoDriver::Direct3D12 => f.write_str("d3d12"),
            VideoDriver::Metal => f.write_str("metal"),
        }
    }
}

/// Valid extensions for shader extensions.
#[repr(u32)]
#[derive(Debug, Copy, Clone)]
pub enum ShaderExtension {
    /// `.slang`
    Slang = 0,
    /// `.glsl`
    Glsl,
    /// `.cg`
    Cg,
}

impl Display for ShaderExtension {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            ShaderExtension::Slang => f.write_str("slang"),
            ShaderExtension::Glsl => f.write_str("glsl"),
            ShaderExtension::Cg => f.write_str("cg"),
        }
    }
}

/// Valid extensions for shader presets
#[repr(u32)]
#[derive(Debug, Copy, Clone)]
pub enum PresetExtension {
    /// `.slangp`
    Slangp = 0,
    /// `.glslp`
    Glslp,
    /// `.cgp`
    Cgp,
}

impl Display for PresetExtension {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            PresetExtension::Slangp => f.write_str("slangp"),
            PresetExtension::Glslp => f.write_str("glslp"),
            PresetExtension::Cgp => f.write_str("cgp"),
        }
    }
}

/// Rotation of the viewport.
#[repr(u32)]
#[derive(Debug, Copy, Clone)]
pub enum Rotation {
    /// Zero
    Zero = 0,
    /// 90 degrees
    Right = 1,
    /// 180 degrees
    Straight = 2,
    /// 270 degrees
    Reflex = 3,
}

impl From<u32> for Rotation {
    fn from(value: u32) -> Self {
        let value = value % 4;
        match value {
            0 => Rotation::Zero,
            1 => Rotation::Right,
            2 => Rotation::Straight,
            3 => Rotation::Reflex,
            _ => unreachable!(),
        }
    }
}

impl Add for Rotation {
    type Output = Rotation;

    fn add(self, rhs: Self) -> Self::Output {
        let lhs = self as u32;
        let out = lhs + rhs as u32;
        Rotation::from(out)
    }
}

impl Display for Rotation {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            Rotation::Zero => f.write_str("0"),
            Rotation::Right => f.write_str("90"),
            Rotation::Straight => f.write_str("180"),
            Rotation::Reflex => f.write_str("270"),
        }
    }
}

/// Orientation of  aspect ratios
#[repr(u32)]
#[derive(Debug, Copy, Clone)]
pub enum Orientation {
    /// Vertical orientation.
    Vertical = 0,
    /// Horizontal orientation.
    Horizontal,
}

impl Display for Orientation {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            Orientation::Vertical => f.write_str("VERT"),
            Orientation::Horizontal => f.write_str("HORZ"),
        }
    }
}

/// An item representing a variable that can be replaced in a path preset.
#[derive(Debug, Clone)]
pub enum ContextItem {
    /// The content directory of the game (`CONTENT-DIR`)
    ContentDirectory(String),
    /// The name of the libretro core (`CORE`)
    CoreName(String),
    /// The filename of the game (`GAME`)
    GameName(String),
    /// The name of the preset (`PRESET`)
    Preset(String),
    /// The name of the preset directory (`PRESET_DIR`)
    PresetDirectory(String),
    /// The video driver (runtime) (`VID-DRV`)
    VideoDriver(VideoDriver),
    /// The extension of shader types supported by the driver (`VID-DRV-SHADER-EXT`)
    VideoDriverShaderExtension(ShaderExtension),
    /// The extension of shader presets supported by the driver (`VID-DRV-PRESET-EXT`)
    VideoDriverPresetExtension(PresetExtension),
    /// The rotation that the core is requesting (`CORE-REQ-ROT`)
    CoreRequestedRotation(Rotation),
    /// Whether or not to allow core-requested rotation (`VID-ALLOW-CORE-ROT`)
    AllowCoreRotation(bool),
    /// The rotation the user is requesting (`VID-USER-ROT`)
    UserRotation(Rotation),
    /// The final rotation (`VID-FINAL-ROT`) calculated by the sum of `VID-USER-ROT` and `CORE-REQ-ROT`
    FinalRotation(Rotation),
    /// The user-adjusted screen orientation (`SCREEN-ORIENT`)
    ScreenOrientation(Rotation),
    /// The orientation of the viewport aspect ratio (`VIEW-ASPECT-ORIENT`)
    ViewAspectOrientation(Orientation),
    /// The orientation of the aspect ratio requested by the core (`CORE-ASPECT-ORIENT`)
    CoreAspectOrientation(Orientation),
    /// An external, arbitrary context variable.
    ExternContext(String, String),
}

impl ContextItem {
    fn toggle_str(v: bool) -> &'static str {
        if v {
            "ON"
        } else {
            "OFF"
        }
    }

    pub fn key(&self) -> &str {
        match self {
            ContextItem::ContentDirectory(_) => "CONTENT-DIR",
            ContextItem::CoreName(_) => "CORE",
            ContextItem::GameName(_) => "GAME",
            ContextItem::Preset(_) => "PRESET",
            ContextItem::PresetDirectory(_) => "PRESET_DIR",
            ContextItem::VideoDriver(_) => "VID-DRV",
            ContextItem::CoreRequestedRotation(_) => "CORE-REQ-ROT",
            ContextItem::AllowCoreRotation(_) => "VID-ALLOW-CORE-ROT",
            ContextItem::UserRotation(_) => "VID-USER-ROT",
            ContextItem::FinalRotation(_) => "VID-FINAL-ROT",
            ContextItem::ScreenOrientation(_) => "SCREEN-ORIENT",
            ContextItem::ViewAspectOrientation(_) => "VIEW-ASPECT-ORIENT",
            ContextItem::CoreAspectOrientation(_) => "CORE-ASPECT-ORIENT",
            ContextItem::VideoDriverShaderExtension(_) => "VID-DRV-SHADER-EXT",
            ContextItem::VideoDriverPresetExtension(_) => "VID-DRV-PRESET-EXT",
            ContextItem::ExternContext(key, _) => key,
        }
    }
}

impl Display for ContextItem {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            ContextItem::ContentDirectory(v) => f.write_str(v),
            ContextItem::CoreName(v) => f.write_str(v),
            ContextItem::GameName(v) => f.write_str(v),
            ContextItem::Preset(v) => f.write_str(v),
            ContextItem::PresetDirectory(v) => f.write_str(v),
            ContextItem::VideoDriver(v) => f.write_fmt(format_args!("{}", v)),
            ContextItem::CoreRequestedRotation(v) => {
                f.write_fmt(format_args!("{}-{}", self.key(), v))
            }
            ContextItem::AllowCoreRotation(v) => f.write_fmt(format_args!(
                "{}-{}",
                self.key(),
                ContextItem::toggle_str(*v)
            )),
            ContextItem::UserRotation(v) => f.write_fmt(format_args!("{}-{}", self.key(), v)),
            ContextItem::FinalRotation(v) => f.write_fmt(format_args!("{}-{}", self.key(), v)),
            ContextItem::ScreenOrientation(v) => f.write_fmt(format_args!("{}-{}", self.key(), v)),
            ContextItem::ViewAspectOrientation(v) => {
                f.write_fmt(format_args!("{}-{}", self.key(), v))
            }
            ContextItem::CoreAspectOrientation(v) => {
                f.write_fmt(format_args!("{}-{}", self.key(), v))
            }
            ContextItem::VideoDriverShaderExtension(v) => f.write_fmt(format_args!("{}", v)),
            ContextItem::VideoDriverPresetExtension(v) => f.write_fmt(format_args!("{}", v)),
            ContextItem::ExternContext(_, v) => f.write_fmt(format_args!("{}", v)),
        }
    }
}

/// A preset wildcard context.
///
/// Any items added after will have higher priority
/// when passed to the shader preset parser.
///
/// When passed to the preset parser, the preset parser
/// will automatically add inferred items at lowest priority.
///
/// Any items added by the user will override the automatically
/// inferred items.
#[derive(Debug, Clone)]
pub struct WildcardContext(VecDeque<ContextItem>);

impl WildcardContext {
    /// Create a new wildcard context.
    pub fn new() -> Self {
        Self(VecDeque::new())
    }

    /// Prepend an item to the context builder.
    pub fn prepend_item(&mut self, item: ContextItem) {
        self.0.push_front(item);
    }

    /// Append an item to the context builder.
    /// The new item will take precedence over all items added before it.
    pub fn append_item(&mut self, item: ContextItem) {
        self.0.push_back(item);
    }

    /// Prepend sensible defaults for the given video driver.
    ///
    /// Any values added, either previously or afterwards will not be overridden.
    pub fn add_video_driver_defaults(&mut self, video_driver: VideoDriver) {
        self.prepend_item(ContextItem::VideoDriverPresetExtension(
            PresetExtension::Slangp,
        ));
        self.prepend_item(ContextItem::VideoDriverShaderExtension(
            ShaderExtension::Slang,
        ));
        self.prepend_item(ContextItem::VideoDriver(video_driver));
    }

    /// Prepend default entries from the path of the preset.
    ///
    /// Any values added, either previously or afterwards will not be overridden.
    pub fn add_path_defaults(&mut self, path: impl AsRef<Path>) {
        let path = path.as_ref();
        if let Some(preset_name) = path.file_stem() {
            let preset_name = preset_name.to_string_lossy();
            self.prepend_item(ContextItem::Preset(preset_name.into()))
        }

        if let Some(preset_dir_name) = path.parent().and_then(|p| {
            if !p.is_dir() {
                return None;
            };
            p.file_name()
        }) {
            let preset_dir_name = preset_dir_name.to_string_lossy();
            self.prepend_item(ContextItem::PresetDirectory(preset_dir_name.into()))
        }
    }

    pub fn to_hashmap(mut self) -> FxHashMap<String, String> {
        let mut map = FxHashMap::default();
        let last_user_rot = self
            .0
            .iter()
            .rfind(|i| matches!(i, ContextItem::UserRotation(_)));
        let last_core_rot = self
            .0
            .iter()
            .rfind(|i| matches!(i, ContextItem::CoreRequestedRotation(_)));

        let final_rot = match (last_core_rot, last_user_rot) {
            (Some(ContextItem::UserRotation(u)), None) => Some(ContextItem::FinalRotation(*u)),
            (None, Some(ContextItem::CoreRequestedRotation(c))) => {
                Some(ContextItem::FinalRotation(*c))
            }
            (Some(ContextItem::UserRotation(u)), Some(ContextItem::CoreRequestedRotation(c))) => {
                Some(ContextItem::FinalRotation(*u + *c))
            }
            _ => None,
        };

        if let Some(final_rot) = final_rot {
            self.prepend_item(final_rot);
        }

        for item in self.0 {
            map.insert(String::from(item.key()), item.to_string());
        }

        map
    }
}

#[rustversion::since(1.74)]
pub(crate) fn apply_context(path: &mut PathBuf, context: &FxHashMap<String, String>) {
    use std::ffi::{OsStr, OsString};

    static WILDCARD_REGEX: Lazy<Regex> = Lazy::new(|| Regex::new("\\$([A-Z-_]+)\\$").unwrap());
    if context.is_empty() {
        return;
    }
    // Don't want to do any extra work if there's no match.
    if !WILDCARD_REGEX.is_match(path.as_os_str().as_encoded_bytes()) {
        return;
    }

    let mut new_path = PathBuf::with_capacity(path.capacity());
    for component in path.components() {
        match component {
            Component::Normal(path) => {
                let haystack = path.as_encoded_bytes();

                let replaced =
                    WILDCARD_REGEX.replace_all(haystack, |caps: &regex::bytes::Captures| {
                        let Some(name) = caps.get(1) else {
                            return caps[0].to_vec();
                        };

                        let Ok(key) = std::str::from_utf8(name.as_bytes()) else {
                            return caps[0].to_vec();
                        };
                        if let Some(replacement) = context.get(key) {
                            return OsString::from(replacement.to_string()).into_encoded_bytes();
                        }
                        return caps[0].to_vec();
                    });

                // SAFETY: The original source is valid encoded bytes, and our replacement is
                // valid encoded bytes. This upholds the safety requirements of `from_encoded_bytes_unchecked`.
                new_path.push(unsafe { OsStr::from_encoded_bytes_unchecked(&replaced.as_ref()) })
            }
            _ => new_path.push(component),
        }
    }

    // If no wildcards are found within the path, or the path after replacing the wildcards does not exist on disk, the path returned will be unaffected.
    if let Ok(true) = new_path.try_exists() {
        *path = new_path;
    }
}

#[rustversion::before(1.74)]
pub(crate) fn apply_context(path: &mut PathBuf, context: &FxHashMap<String, String>) {
    use os_str_bytes::RawOsStr;
    static WILDCARD_REGEX: Lazy<Regex> = Lazy::new(|| Regex::new("\\$([A-Z-_]+)\\$").unwrap());
    if context.is_empty() {
        return;
    }
    let path_str = RawOsStr::new(path.as_os_str());
    let path_bytes = path_str.to_raw_bytes();
    // Don't want to do any extra work if there's no match.
    if !WILDCARD_REGEX.is_match(&path_bytes) {
        return;
    }

    let mut new_path = PathBuf::with_capacity(path.capacity());
    for component in path.components() {
        match component {
            Component::Normal(path) => {
                let haystack = RawOsStr::new(path);
                let haystack = haystack.to_raw_bytes();

                let replaced =
                    WILDCARD_REGEX.replace_all(&haystack, |caps: &regex::bytes::Captures| {
                        let Some(name) = caps.get(1) else {
                            return caps[0].to_vec();
                        };

                        let Ok(key) = std::str::from_utf8(name.as_bytes()) else {
                            return caps[0].to_vec();
                        };
                        if let Some(replacement) = context.get(key) {
                            return RawOsStr::from_str(replacement).to_raw_bytes().to_vec();
                        }
                        return caps[0].to_vec();
                    });

                // SAFETY: The original source is valid encoded bytes, and our replacement is
                // valid encoded bytes. This upholds the safety requirements of `from_encoded_bytes_unchecked`.
                new_path.push(RawOsStr::assert_cow_from_raw_bytes(&replaced.as_ref()).to_os_str())
            }
            _ => new_path.push(component),
        }
    }

    // If no wildcards are found within the path, or the path after replacing the wildcards does not exist on disk, the path returned will be unaffected.
    if let Ok(true) = new_path.try_exists() {
        *path = new_path;
    }
}
