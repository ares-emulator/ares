use crate::error::{ParseErrorKind, ParsePresetError};
use crate::parse::{remove_if, Span, Token};
use crate::{ScaleFactor, ScaleType};
use nom::bytes::complete::tag;
use nom::character::complete::digit1;
use nom::combinator::{eof, map_res};
use std::collections::VecDeque;

use nom::IResult;
use num_traits::cast::ToPrimitive;

use crate::parse::token::do_lex;
use librashader_common::map::FastHashMap;
use librashader_common::{FilterMode, WrapMode};
use std::fs::File;
use std::io::Read;
use std::path::{Path, PathBuf};
use std::str::FromStr;

use crate::context::{apply_context, WildcardContext};
use crate::extract_if::MakeExtractIf;

#[derive(Debug)]
pub enum Value {
    ShaderCount(i32),
    FeedbackPass(i32),
    Shader(i32, PathBuf),
    ScaleX(i32, ScaleFactor),
    ScaleY(i32, ScaleFactor),
    Scale(i32, ScaleFactor),
    ScaleType(i32, ScaleType),
    ScaleTypeX(i32, ScaleType),
    ScaleTypeY(i32, ScaleType),
    FilterMode(i32, FilterMode),
    WrapMode(i32, WrapMode),
    FrameCountMod(i32, u32),
    FloatFramebuffer(i32, bool),
    SrgbFramebuffer(i32, bool),
    MipmapInput(i32, bool),
    Alias(i32, String),
    Parameter(String, f32),
    Texture {
        name: String,
        filter_mode: FilterMode,
        wrap_mode: WrapMode,
        mipmap: bool,
        path: PathBuf,
    },
}

impl Value {
    pub(crate) fn shader_index(&self) -> Option<i32> {
        match self {
            Value::Shader(i, _) => Some(*i),
            Value::ScaleX(i, _) => Some(*i),
            Value::ScaleY(i, _) => Some(*i),
            Value::Scale(i, _) => Some(*i),
            Value::ScaleType(i, _) => Some(*i),
            Value::ScaleTypeX(i, _) => Some(*i),
            Value::ScaleTypeY(i, _) => Some(*i),
            Value::FilterMode(i, _) => Some(*i),
            Value::WrapMode(i, _) => Some(*i),
            Value::FrameCountMod(i, _) => Some(*i),
            Value::FloatFramebuffer(i, _) => Some(*i),
            Value::SrgbFramebuffer(i, _) => Some(*i),
            Value::MipmapInput(i, _) => Some(*i),
            Value::Alias(i, _) => Some(*i),
            _ => None,
        }
    }
}

fn from_int(input: Span) -> Result<i32, ParsePresetError> {
    // Presets like to commit ✨CRIMES✨ and end their lines with a ";".
    // It's too hard to put this in the lexer because we can't tell between
    // semicolon crimes or a valid param/texture name listing.
    let to_parse = input.trim().trim_end_matches(";");
    i32::from_str(to_parse)
        .map_err(|_| ParsePresetError::ParserError {
            offset: input.location_offset(),
            row: input.location_line(),
            col: input.get_column(),
            kind: ParseErrorKind::Int,
        })
        .or_else(|e| {
            // An even more egregious ✨CRIME✨ is using a float as a shader index.
            let result = f32::from_str(to_parse).map_err(|_| e)?;
            let result = result
                .trunc()
                .to_i32()
                .ok_or(ParsePresetError::ParserError {
                    offset: input.location_offset(),
                    row: input.location_line(),
                    col: input.get_column(),
                    kind: ParseErrorKind::Int,
                })?;
            Ok(result)
        })
}

fn from_ul(input: Span) -> Result<u32, ParsePresetError> {
    // Presets like to commit ✨CRIMES✨ and end their lines with a ";".
    // It's too hard to put this in the lexer because we can't tell between
    // semicolon crimes or a valid param/texture name listing.
    u32::from_str(input.trim().trim_end_matches(";")).map_err(|_| ParsePresetError::ParserError {
        offset: input.location_offset(),
        row: input.location_line(),
        col: input.get_column(),
        kind: ParseErrorKind::UnsignedInt,
    })
}

fn from_float(input: Span) -> Result<f32, ParsePresetError> {
    // Presets like to commit ✨CRIMES✨ and end their lines with a ";".
    // It's too hard to put this in the lexer because we can't tell between
    // semicolon crimes or a valid param/texture name listing.
    f32::from_str(input.trim().trim_end_matches(";")).map_err(|_| ParsePresetError::ParserError {
        offset: input.location_offset(),
        row: input.location_line(),
        col: input.get_column(),
        kind: ParseErrorKind::Float,
    })
}

fn from_bool(input: Span) -> Result<bool, ParsePresetError> {
    if let Ok(i) = i32::from_str(input.trim()) {
        return match i {
            1 => Ok(true),
            0 => Ok(false),
            _ => Err(ParsePresetError::ParserError {
                offset: input.location_offset(),
                row: input.location_line(),
                col: input.get_column(),
                kind: ParseErrorKind::Bool,
            }),
        };
    }
    bool::from_str(input.trim()).map_err(|_| ParsePresetError::ParserError {
        offset: input.location_offset(),
        row: input.location_line(),
        col: input.get_column(),
        kind: ParseErrorKind::Bool,
    })
}

fn parse_indexed_key<'a>(key: &'static str, input: Span<'a>) -> IResult<Span<'a>, i32> {
    let (input, _) = tag(key)(input)?;
    let (input, idx) = map_res(digit1, from_int)(input)?;
    let (input, _) = eof(input)?;
    Ok((input, idx))
}

pub const SHADER_MAX_REFERENCE_DEPTH: usize = 16;

// prereq: root_path must be contextualized
fn load_child_reference_strings(
    root_references: Vec<PathBuf>,
    root_path: impl AsRef<Path>,
    context: &FastHashMap<String, String>,
) -> Result<Vec<(PathBuf, String)>, ParsePresetError> {
    let root_path = root_path.as_ref();

    let mut reference_depth = 0;
    let mut reference_strings: VecDeque<(PathBuf, String)> = VecDeque::new();
    let root_references = vec![(root_path.to_path_buf(), root_references)];
    let mut root_references = VecDeque::from(root_references);
    // search needs to be depth first to allow for overrides.
    while let Some((mut reference_root, referenced_paths)) = root_references.pop_front() {
        if reference_depth > SHADER_MAX_REFERENCE_DEPTH {
            return Err(ParsePresetError::ExceededReferenceDepth);
        }
        // enter the current root
        reference_depth += 1;
        // canonicalize current root
        apply_context(&mut reference_root, context);
        let reference_root = reference_root
            .canonicalize()
            .map_err(|e| ParsePresetError::IOError(reference_root.to_path_buf(), e))?;

        // resolve all referenced paths against root
        // println!("Resolving {referenced_paths:?} against {reference_root:?}.");

        for path in referenced_paths {
            let mut path = reference_root.join(path.clone());
            apply_context(&mut path, context);

            let mut path = path
                .canonicalize()
                .map_err(|e| ParsePresetError::IOError(path.clone(), e))?;
            // println!("Opening {:?}", path);
            let mut reference_contents = String::new();
            File::open(&path)
                .map_err(|e| ParsePresetError::IOError(path.clone(), e))?
                .read_to_string(&mut reference_contents)
                .map_err(|e| ParsePresetError::IOError(path.clone(), e))?;

            let mut new_tokens = do_lex(&reference_contents)?;
            let new_references: Vec<PathBuf> = new_tokens
                .extract_if(|token| *token.key.fragment() == "#reference")
                .map(|value| PathBuf::from(*value.value.fragment()))
                .collect();

            path.pop();
            reference_strings.push_front((path.clone(), reference_contents));
            if !new_references.is_empty() {
                root_references.push_front((path, new_references));
            }
        }
    }

    Ok(reference_strings.into())
}

pub(crate) fn parse_preset(
    path: impl AsRef<Path>,
    context: WildcardContext,
) -> Result<Vec<Value>, ParsePresetError> {
    let path = path.as_ref();
    let mut path = path.to_path_buf();
    let context = context.to_hashmap();

    apply_context(&mut path, &context);

    let path = path
        .canonicalize()
        .map_err(|e| ParsePresetError::IOError(path.to_path_buf(), e))?;

    let mut contents = String::new();
    File::open(&path)
        .and_then(|mut f| f.read_to_string(&mut contents))
        .map_err(|e| ParsePresetError::IOError(path.to_path_buf(), e))?;

    let tokens = super::token::do_lex(&contents)?;
    parse_values(tokens, path, context)
}

// prereq: root_path must be contextualized
pub fn parse_values(
    mut tokens: Vec<Token>,
    root_path: impl AsRef<Path>,
    context: FastHashMap<String, String>,
) -> Result<Vec<Value>, ParsePresetError> {
    let mut root_path = root_path.as_ref().to_path_buf();
    if root_path.is_relative() {
        return Err(ParsePresetError::RootPathWasNotAbsolute);
    }
    if !root_path.is_dir() {
        // we don't really care if this doesn't do anything because a non-canonical root path will
        // fail at a later stage during resolution.
        root_path.pop();
    }

    let references: Vec<PathBuf> = tokens
        .extract_if(|token| *token.key.fragment() == "#reference")
        .map(|value| PathBuf::from(*value.value.fragment()))
        .collect();

    // unfortunately we need to lex twice because there's no way to know the references ahead of time.
    // the returned references should have context applied

    let child_strings = load_child_reference_strings(references, &root_path, &context)?;
    let mut all_tokens: Vec<(&Path, Vec<Token>)> = Vec::new();

    for (path, string) in child_strings.iter() {
        // lex the child tokens
        let mut tokens = do_lex(string.as_ref())?;
        tokens.retain(|token| *token.key.fragment() != "#reference");
        all_tokens.push((path.as_path(), tokens))
    }

    // load depth first, so all child tokens are first.
    // Later tokens take precedence.
    all_tokens.push((root_path.as_path(), tokens));

    // collect all possible parameter names.
    let mut parameter_names: Vec<&str> = Vec::new();
    for (_, tokens) in all_tokens.iter_mut() {
        for token in tokens.extract_if(|token| *token.key.fragment() == "parameters") {
            let parameter_name_string: &str = token.value.fragment();
            for parameter_name in parameter_name_string.split(';') {
                parameter_names.push(parameter_name);
            }
        }
    }

    // collect all possible texture names.
    let mut texture_names: Vec<&str> = Vec::new();
    for (_, tokens) in all_tokens.iter_mut() {
        for token in tokens.extract_if(|token| *token.key.fragment() == "textures") {
            let texture_name_string: &str = token.value.fragment();
            for texture_name in texture_name_string.split(';') {
                texture_names.push(texture_name);
            }
        }
    }

    let mut values = Vec::new();
    // resolve shader paths.
    for (path, tokens) in all_tokens.iter_mut() {
        for token in tokens.extract_if(|token| parse_indexed_key("shader", token.key).is_ok()) {
            let (_, index) = parse_indexed_key("shader", token.key).map_err(|e| match e {
                nom::Err::Error(e) | nom::Err::Failure(e) => {
                    let input: Span = e.input;
                    ParsePresetError::ParserError {
                        offset: input.location_offset(),
                        row: input.location_line(),
                        col: input.get_column(),
                        kind: ParseErrorKind::Index("shader"),
                    }
                }
                _ => ParsePresetError::ParserError {
                    offset: 0,
                    row: 0,
                    col: 0,
                    kind: ParseErrorKind::Index("shader"),
                },
            })?;

            let mut relative_path = path.to_path_buf();
            relative_path.push(*token.value.fragment());
            relative_path
                .canonicalize()
                .map_err(|e| ParsePresetError::IOError(relative_path.clone(), e))?;
            values.push(Value::Shader(index, relative_path))
        }
    }

    // resolve texture paths
    let mut textures = Vec::new();
    for (path, tokens) in all_tokens.iter_mut() {
        for token in tokens.extract_if(|token| texture_names.contains(token.key.fragment())) {
            let mut relative_path = path.to_path_buf();
            relative_path.push(*token.value.fragment());
            relative_path
                .canonicalize()
                .map_err(|e| ParsePresetError::IOError(relative_path.clone(), e))?;
            textures.push((token.key, relative_path))
        }
    }

    let mut tokens: Vec<(&Path, Token)> = all_tokens
        .into_iter()
        .flat_map(|(p, token)| token.into_iter().map(move |t| (p, t)))
        .collect();

    for (texture, path) in textures {
        let mipmap = remove_if(&mut tokens, |(_, t)| {
            t.key.starts_with(*texture)
                && t.key.ends_with("_mipmap")
                && t.key.len() == texture.len() + "_mipmap".len()
        })
        .map_or_else(|| Ok(false), |(_, v)| from_bool(v.value))?;

        let linear = remove_if(&mut tokens, |(_, t)| {
            t.key.starts_with(*texture)
                && t.key.ends_with("_linear")
                && t.key.len() == texture.len() + "_linear".len()
        })
        .map_or_else(|| Ok(false), |(_, v)| from_bool(v.value))?;

        let wrap_mode = remove_if(&mut tokens, |(_, t)| {
            t.key.starts_with(*texture)
                && (t.key.ends_with("_wrap_mode") || t.key.ends_with("_repeat_mode"))
                && (t.key.len() == texture.len() + "_wrap_mode".len()
                    || t.key.len() == texture.len() + "_repeat_mode".len())
        })
        // NOPANIC: infallible
        .map_or_else(WrapMode::default, |(_, v)| {
            WrapMode::from_str(&v.value).unwrap()
        });

        // This really isn't supported but crt-torridgristle uses this syntax.
        // Again, don't know how this works in RA but RA's parser isn't as strict as ours.
        let filter = remove_if(&mut tokens, |(_, t)| {
            t.key.starts_with("filter_")
                && t.key.ends_with(*texture)
                && t.key.len() == "filter_".len() + texture.len()
        })
        // NOPANIC: infallible
        .map_or(None, |(_, v)| Some(FilterMode::from_str(&v.value).unwrap()));

        values.push(Value::Texture {
            name: texture.to_string(),
            filter_mode: filter.unwrap_or(if linear {
                FilterMode::Linear
            } else {
                FilterMode::Nearest
            }),
            wrap_mode,
            mipmap,
            path,
        })
    }

    let mut rest_tokens = Vec::new();
    // hopefully no more textures left in the token tree
    for (p, token) in tokens {
        if parameter_names.contains(token.key.fragment()) {
            let param_val = from_float(token.value)
                // This is literally just to work around BEAM_PROFILE in crt-hyllian-sinc-glow.slangp
                // which has ""0'.000000". This somehow works in RA because it defaults to 0, probably.
                // This hack is only used for **known** parameter names. If we tried this for undeclared
                // params (god help me), it would be pretty bad because we lose texture path fallback.
                .unwrap_or(0.0);
            values.push(Value::Parameter(
                token.key.fragment().to_string(),
                param_val,
            ));
            continue;
        }
        if token.key.fragment() == &"shaders" {
            let shader_count = from_int(token.value)?;
            values.push(Value::ShaderCount(shader_count));
            continue;
        }
        if token.key.fragment() == &"feedback_pass" {
            let feedback_pass = from_int(token.value)?;
            values.push(Value::FeedbackPass(feedback_pass));
            continue;
        }
        if let Ok((_, idx)) = parse_indexed_key("filter_linear", token.key) {
            let linear = from_bool(token.value)?;
            values.push(Value::FilterMode(
                idx,
                if linear {
                    FilterMode::Linear
                } else {
                    FilterMode::Nearest
                },
            ));
            continue;
        }

        if let Ok((_, idx)) = parse_indexed_key("wrap_mode", token.key) {
            let wrap_mode = WrapMode::from_str(&token.value).unwrap();
            values.push(Value::WrapMode(idx, wrap_mode));
            continue;
        }

        // crt-geom uses repeat_mode...
        if let Ok((_, idx)) = parse_indexed_key("repeat_mode", token.key) {
            let wrap_mode = WrapMode::from_str(&token.value).unwrap();
            values.push(Value::WrapMode(idx, wrap_mode));
            continue;
        }

        // crt-royale uses 'texture_wrap_mode' instead of 'wrap_mode', I have no idea
        // how this possibly could work in RA, but here it is..
        if let Ok((_, idx)) = parse_indexed_key("texture_wrap_mode", token.key) {
            let wrap_mode = WrapMode::from_str(&token.value).unwrap();
            values.push(Value::WrapMode(idx, wrap_mode));
            continue;
        }

        if let Ok((_, idx)) = parse_indexed_key("frame_count_mod", token.key) {
            let frame_count_mod = from_ul(token.value)?;
            values.push(Value::FrameCountMod(idx, frame_count_mod));
            continue;
        }

        if let Ok((_, idx)) = parse_indexed_key("srgb_framebuffer", token.key) {
            let enabled = from_bool(token.value)?;
            values.push(Value::SrgbFramebuffer(idx, enabled));
            continue;
        }

        if let Ok((_, idx)) = parse_indexed_key("float_framebuffer", token.key) {
            let enabled = from_bool(token.value)?;
            values.push(Value::FloatFramebuffer(idx, enabled));
            continue;
        }

        if let Ok((_, idx)) = parse_indexed_key("mipmap_input", token.key) {
            let enabled = from_bool(token.value)?;
            values.push(Value::MipmapInput(idx, enabled));
            continue;
        }

        // vector-glow-alt-render.slangp uses "mipmap" for pass 1, but "mipmap_input" for everything else.
        if let Ok((_, idx)) = parse_indexed_key("mipmap", token.key) {
            let enabled = from_bool(token.value)?;
            values.push(Value::MipmapInput(idx, enabled));
            continue;
        }

        if let Ok((_, idx)) = parse_indexed_key("alias", token.key) {
            values.push(Value::Alias(idx, token.value.to_string()));
            continue;
        }
        if let Ok((_, idx)) = parse_indexed_key("scale_type", token.key) {
            let scale_type = ScaleType::from_str(token.value.trim())?;
            values.push(Value::ScaleType(idx, scale_type));
            continue;
        }
        if let Ok((_, idx)) = parse_indexed_key("scale_type_x", token.key) {
            let scale_type = ScaleType::from_str(token.value.trim())?;
            values.push(Value::ScaleTypeX(idx, scale_type));
            continue;
        }
        if let Ok((_, idx)) = parse_indexed_key("scale_type_y", token.key) {
            let scale_type = ScaleType::from_str(token.value.trim())?;
            values.push(Value::ScaleTypeY(idx, scale_type));
            continue;
        }
        rest_tokens.push((p, token))
    }

    let mut undeclared_textures = Vec::new();
    for (path, token) in &rest_tokens {
        if let Ok((_, idx)) = parse_indexed_key("scale", token.key) {
            let scale = if values.iter().any(|t| matches!(*t, Value::ScaleType(match_idx, ScaleType::Absolute) if match_idx == idx)) {
                let scale = from_int(token.value)?;
                ScaleFactor::Absolute(scale)
            } else {
                let scale = from_float(token.value)?;
                ScaleFactor::Float(scale)
            };

            values.push(Value::Scale(idx, scale));
            continue;
        }
        if let Ok((_, idx)) = parse_indexed_key("scale_x", token.key) {
            let scale = if values.iter().any(|t| matches!(*t, Value::ScaleType(match_idx, ScaleType::Absolute) | Value::ScaleTypeX(match_idx, ScaleType::Absolute) if match_idx == idx)) {
                let scale = from_int(token.value)?;
                ScaleFactor::Absolute(scale)
            } else {
                let scale = from_float(token.value)?;
                ScaleFactor::Float(scale)
            };

            values.push(Value::ScaleX(idx, scale));
            continue;
        }
        if let Ok((_, idx)) = parse_indexed_key("scale_y", token.key) {
            let scale = if values.iter().any(|t| matches!(*t, Value::ScaleType(match_idx, ScaleType::Absolute) | Value::ScaleTypeY(match_idx, ScaleType::Absolute) if match_idx == idx)) {
                let scale = from_int(token.value)?;
                ScaleFactor::Absolute(scale)
            } else {
                let scale = from_float(token.value)?;
                ScaleFactor::Float(scale)
            };

            values.push(Value::ScaleY(idx, scale));
            continue;
        }

        // handle undeclared parameters after parsing everything else as a last resort.
        if let Ok(param_val) = from_float(token.value) {
            values.push(Value::Parameter(
                token.key.fragment().to_string(),
                param_val,
            ));
        }
        // very last resort, assume undeclared texture (must have extension)
        else if Path::new(token.value.fragment()).extension().is_some()
            && ["_mipmap", "_linear", "_wrap_mode", "_repeat_mode"]
                .iter()
                .all(|k| !token.key.ends_with(k))
        {
            let mut relative_path = path.to_path_buf();
            relative_path.push(*token.value.fragment());
            relative_path
                .canonicalize()
                .map_err(|e| ParsePresetError::IOError(relative_path.clone(), e))?;
            undeclared_textures.push((token.key, relative_path));
        }

        // we tried our best
    }

    // Since there are undeclared textures we need to deal with potential mipmap information.
    for (texture, path) in undeclared_textures {
        let mipmap = remove_if(&mut rest_tokens, |(_, t)| {
            t.key.starts_with(*texture)
                && t.key.ends_with("_mipmap")
                && t.key.len() == texture.len() + "_mipmap".len()
        })
        .map_or_else(|| Ok(false), |(_, v)| from_bool(v.value))?;

        let linear = remove_if(&mut rest_tokens, |(_, t)| {
            t.key.starts_with(*texture)
                && t.key.ends_with("_linear")
                && t.key.len() == texture.len() + "_linear".len()
        })
        .map_or_else(|| Ok(false), |(_, v)| from_bool(v.value))?;

        let wrap_mode = remove_if(&mut rest_tokens, |(_, t)| {
            t.key.starts_with(*texture)
                && (t.key.ends_with("_wrap_mode") || t.key.ends_with("_repeat_mode"))
                && (t.key.len() == texture.len() + "_wrap_mode".len()
                    || t.key.len() == texture.len() + "_repeat_mode".len())
        })
        // NOPANIC: infallible
        .map_or_else(WrapMode::default, |(_, v)| {
            WrapMode::from_str(&v.value).unwrap()
        });

        values.push(Value::Texture {
            name: texture.to_string(),
            filter_mode: if linear {
                FilterMode::Linear
            } else {
                FilterMode::Nearest
            },
            wrap_mode,
            mipmap,
            path,
        })
    }

    // all tokens should be ok to process now.
    Ok(values)
}

#[cfg(test)]
mod test {
    use crate::parse::value::parse_preset;
    use crate::WildcardContext;
    use std::path::PathBuf;

    #[test]
    pub fn parse_basic() {
        let root =
            PathBuf::from("../test/slang-shaders/bezel/Mega_Bezel/Presets/Base_CRT_Presets/MBZ__3__STD__MEGATRON-NTSC.slangp");
        let basic = parse_preset(root, WildcardContext::new());
        eprintln!("{basic:?}");
        assert!(basic.is_ok());
    }
}
