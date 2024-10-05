use crate::parse::remove_if;
use crate::parse::value::Value;
use crate::{
    ParameterMeta, PassConfig, PassMeta, Scale2D, Scaling, ShaderPreset, TextureConfig, TextureMeta,
};
use vec_extract_if_polyfill::MakeExtractIf;

pub fn resolve_values(mut values: Vec<Value>) -> ShaderPreset {
    let textures: Vec<TextureConfig> =
        MakeExtractIf::extract_if(&mut values, |f| matches!(*f, Value::Texture { .. }))
            .map(|value| {
                if let Value::Texture {
                    name,
                    filter_mode,
                    wrap_mode,
                    mipmap,
                    path,
                } = value
                {
                    TextureConfig {
                        path,
                        meta: TextureMeta {
                            name,
                            wrap_mode,
                            filter_mode,
                            mipmap,
                        },
                    }
                } else {
                    unreachable!("values should all be of type Texture")
                }
            })
            .collect();
    let parameters: Vec<ParameterMeta> =
        MakeExtractIf::extract_if(&mut values, |f| matches!(*f, Value::Parameter { .. }))
            .map(|value| {
                if let Value::Parameter(name, value) = value {
                    ParameterMeta { name, value }
                } else {
                    unreachable!("values should be all of type parameters")
                }
            })
            .collect();

    let mut shaders = Vec::new();
    let shader_count =
        remove_if(&mut values, |v| matches!(*v, Value::ShaderCount(_))).map_or(0, |value| {
            if let Value::ShaderCount(count) = value {
                count
            } else {
                unreachable!("value should be of type shader_count")
            }
        });

    #[cfg(feature = "parse_legacy_glsl")]
    let feedback_pass = remove_if(&mut values, |v| matches!(*v, Value::FeedbackPass(_)))
        .map(|value| {
            if let Value::FeedbackPass(pass) = value {
                pass
            } else {
                unreachable!("value should be of type feedback_pass")
            }
        })
        .unwrap_or(0);

    for shader in 0..shader_count {
        if let Some(Value::Shader(id, name)) = remove_if(
            &mut values,
            |v| matches!(*v, Value::Shader(shader_index, _) if shader_index == shader),
        ) {
            let shader_values: Vec<Value> =
                MakeExtractIf::extract_if(&mut values, |v| v.shader_index() == Some(shader))
                    .collect();
            let scale_type = shader_values.iter().find_map(|f| match f {
                Value::ScaleType(_, value) => Some(*value),
                _ => None,
            });

            let mut scale_type_x = shader_values.iter().find_map(|f| match f {
                Value::ScaleTypeX(_, value) => Some(*value),
                _ => None,
            });

            let mut scale_type_y = shader_values.iter().find_map(|f| match f {
                Value::ScaleTypeY(_, value) => Some(*value),
                _ => None,
            });

            if scale_type.is_some() {
                // scale takes priority
                // https://github.com/libretro/RetroArch/blob/fcbd72dbf3579eb31721fbbf0d89a139834bcce9/gfx/video_shader_parse.c#L310
                scale_type_x = scale_type;
                scale_type_y = scale_type;
            }

            let scale_valid = scale_type_x.is_some() || scale_type_y.is_some();

            let scale = shader_values.iter().find_map(|f| match f {
                Value::Scale(_, value) => Some(*value),
                _ => None,
            });

            let mut scale_x = shader_values.iter().find_map(|f| match f {
                Value::ScaleX(_, value) => Some(*value),
                _ => None,
            });

            let mut scale_y = shader_values.iter().find_map(|f| match f {
                Value::ScaleY(_, value) => Some(*value),
                _ => None,
            });

            if scale.is_some() {
                // scale takes priority
                // https://github.com/libretro/RetroArch/blob/fcbd72dbf3579eb31721fbbf0d89a139834bcce9/gfx/video_shader_parse.c#L310
                scale_x = scale;
                scale_y = scale;
            }

            let shader = PassConfig {
                path: name,
                meta: PassMeta {
                    id,
                    alias: shader_values.iter().find_map(|f| match f {
                        Value::Alias(_, value) => Some(value.clone()),
                        _ => None,
                    }),
                    filter: shader_values
                        .iter()
                        .find_map(|f| match f {
                            Value::FilterMode(_, value) => Some(*value),
                            _ => None,
                        })
                        .unwrap_or_default(),
                    wrap_mode: shader_values
                        .iter()
                        .find_map(|f| match f {
                            Value::WrapMode(_, value) => Some(*value),
                            _ => None,
                        })
                        .unwrap_or_default(),
                    frame_count_mod: shader_values
                        .iter()
                        .find_map(|f| match f {
                            Value::FrameCountMod(_, value) => Some(*value),
                            _ => None,
                        })
                        .unwrap_or(0),
                    srgb_framebuffer: shader_values
                        .iter()
                        .find_map(|f| match f {
                            Value::SrgbFramebuffer(_, value) => Some(*value),
                            _ => None,
                        })
                        .unwrap_or(false),
                    float_framebuffer: shader_values
                        .iter()
                        .find_map(|f| match f {
                            Value::FloatFramebuffer(_, value) => Some(*value),
                            _ => None,
                        })
                        .unwrap_or(false),
                    mipmap_input: shader_values
                        .iter()
                        .find_map(|f| match f {
                            Value::MipmapInput(_, value) => Some(*value),
                            _ => None,
                        })
                        .unwrap_or(false),
                    scaling: Scale2D {
                        valid: scale_valid,
                        x: Scaling {
                            scale_type: scale_type_x.unwrap_or_default(),
                            factor: scale_x.unwrap_or_default(),
                        },
                        y: Scaling {
                            scale_type: scale_type_y.unwrap_or_default(),
                            factor: scale_y.unwrap_or_default(),
                        },
                    },
                },
            };

            shaders.push(shader)
        }
    }

    ShaderPreset {
        #[cfg(feature = "parse_legacy_glsl")]
        feedback_pass,
        pass_count: shader_count,
        passes: shaders,
        textures,
        parameters,
    }
}
