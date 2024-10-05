use crate::{PreprocessError, ShaderParameter};
use librashader_common::ImageFormat;
use nom::bytes::complete::{is_not, tag, take_while};

use librashader_common::map::ShortString;
use nom::character::complete::{multispace0, multispace1};
use nom::combinator::opt;
use nom::number::complete::float;
use nom::sequence::delimited;
use nom::IResult;
use std::str::FromStr;

#[derive(Debug)]
pub(crate) struct ShaderMeta {
    pub(crate) format: ImageFormat,
    pub(crate) parameters: Vec<ShaderParameter>,
    pub(crate) name: Option<ShortString>,
}

fn parse_parameter_string(input: &str) -> Result<ShaderParameter, PreprocessError> {
    fn parse_parameter_string_name(input: &str) -> IResult<&str, (&str, &str)> {
        let (input, _) = tag("#pragma parameter ")(input)?;
        let (input, name) = take_while(|c| c != ' ' && c != '\t')(input)?;
        let (input, _) = multispace1(input)?;
        let (input, description) = delimited(tag("\""), is_not("\""), tag("\""))(input)?;
        let (input, _) = multispace1(input)?;
        Ok((input, (name, description)))
    }

    fn parse_parameter_string_inner<'a, 'b>(
        name: &'a str,
        description: &'a str,
        input: &'b str,
    ) -> IResult<&'b str, ShaderParameter> {
        let (input, initial) = float(input)?;
        let (input, _) = multispace1(input)?;
        let (input, minimum) = float(input)?;
        let (input, _) = multispace1(input)?;
        let (input, maximum) = float(input)?;

        // Step is actually optional and defaults to 0.02
        // This behaviour can be seen in shaders like
        // crt/crt-slangtest-cubic.slangp
        // which doesn't have a step argument
        // #pragma parameter OUT_GAMMA "Monitor Output Gamma" 2.2 1.8 2.4

        // https://github.com/libretro/slang-shaders/blob/0e2939787076e4a8a83be89175557fde23abe837/crt/shaders/crt-slangtest/parameters.inc#L1
        let (input, _) = multispace0(input)?;
        let (input, step) = opt(float)(input)?;
        Ok((
            input,
            ShaderParameter {
                id: name.into(),
                description: description.to_string(),
                initial,
                minimum,
                maximum,
                step: step.unwrap_or(0.02),
            },
        ))
    }

    let Ok((params, (name, description))) = parse_parameter_string_name(input) else {
        return Err(PreprocessError::PragmaParseError(input.to_string()));
    };

    // some shaders do some really funky things with their pragmas so we need to be lenient and ignore
    // that it can be set at all.
    if let Ok((_, param)) = parse_parameter_string_inner(name, description, params) {
        Ok(param)
    } else {
        Ok(ShaderParameter {
            id: name.into(),
            description: description.to_string(),
            initial: 0f32,
            minimum: 0f32,
            maximum: 0f32,
            step: 0f32,
        })
    }
}

pub(crate) fn parse_pragma_meta(source: impl AsRef<str>) -> Result<ShaderMeta, PreprocessError> {
    let source = source.as_ref();
    let mut parameters: Vec<ShaderParameter> = Vec::new();
    let mut format = ImageFormat::default();
    let mut name = None;
    for line in source.lines() {
        if line.starts_with("#pragma parameter ") {
            let parameter = parse_parameter_string(line)?;
            if let Some(existing) = parameters.iter().find(|&p| p.id == parameter.id) {
                if existing != &parameter {
                    return Err(PreprocessError::DuplicatePragmaError(parameter.id));
                }
            } else {
                parameters.push(parameter);
            }
        }

        if let Some(format_string) = line.strip_prefix("#pragma format ") {
            if format != ImageFormat::Unknown {
                return Err(PreprocessError::DuplicatePragmaError(line.into()));
            }

            let format_string = format_string.trim();
            format = ImageFormat::from_str(format_string)?;

            if format == ImageFormat::Unknown {
                return Err(PreprocessError::UnknownImageFormat);
            }
        }

        if let Some(pragma_name) = line.strip_prefix("#pragma name ") {
            if name.is_some() {
                return Err(PreprocessError::DuplicatePragmaError(line.into()));
            }

            name = Some(ShortString::from(pragma_name.trim()))
        }
    }

    Ok(ShaderMeta {
        name,
        format,
        parameters,
    })
}

#[cfg(test)]
mod test {
    use crate::pragma::parse_parameter_string;
    use crate::ShaderParameter;

    #[test]
    fn parses_parameter_pragma() {
        assert_eq!(ShaderParameter {
            id: "exc".into(),
            description: "orizontal correction hack (games where players stay at center)".to_string(),
            initial: 0.0,
            minimum: -10.0,
            maximum: 10.0,
            step: 0.25
        }, parse_parameter_string(r#"#pragma parameter exc "orizontal correction hack (games where players stay at center)" 0.0 -10.0 10.0 0.25"#).unwrap())
    }

    #[test]
    fn parses_parameter_pragma_test() {
        assert_eq!(ShaderParameter {
            id: "HSM_CORE_RES_SAMPLING_MULT_SCANLINE_DIR".into(),
            description: "          Scanline Dir Multiplier".to_string(),
            initial: 100.0,
            minimum: 25.0,
            maximum: 1600.0,
            step: 25.0
        }, parse_parameter_string(r#"#pragma parameter HSM_CORE_RES_SAMPLING_MULT_SCANLINE_DIR			"          Scanline Dir Multiplier"  100 25 1600 25"#).unwrap())
    }

    #[test]
    fn parses_parameter_pragma_with_no_step() {
        assert_eq!(
            ShaderParameter {
                id: "OUT_GAMMA".into(),
                description: "Monitor Output Gamma".to_string(),
                initial: 2.2,
                minimum: 1.8,
                maximum: 2.4,
                step: 0.02
            },
            parse_parameter_string(
                r#"#pragma parameter OUT_GAMMA "Monitor Output Gamma" 2.2 1.8 2.4"#
            )
            .unwrap()
        )
    }
}
