use crate::{PreprocessError, ShaderParameter};
use librashader_common::ImageFormat;
use nom::bytes::complete::{is_not, tag, take_while};

use nom::character::complete::multispace1;
use nom::number::complete::float;
use nom::sequence::delimited;
use nom::IResult;
use std::str::FromStr;

#[derive(Debug)]
pub(crate) struct ShaderMeta {
    pub(crate) format: ImageFormat,
    pub(crate) parameters: Vec<ShaderParameter>,
    pub(crate) name: Option<String>,
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
        let (input, _) = multispace1(input)?;
        let (input, step) = float(input)?;
        Ok((
            input,
            ShaderParameter {
                id: name.to_string(),
                description: description.to_string(),
                initial,
                minimum,
                maximum,
                step,
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
            id: name.to_string(),
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
                return Err(PreprocessError::DuplicatePragmaError(line.to_string()));
            }

            let format_string = format_string.trim();
            format = ImageFormat::from_str(format_string)?;

            if format == ImageFormat::Unknown {
                return Err(PreprocessError::UnknownImageFormat);
            }
        }

        if line.starts_with("#pragma name ") {
            if name.is_some() {
                return Err(PreprocessError::DuplicatePragmaError(line.to_string()));
            }

            name = Some(line.trim().to_string())
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
            id: "exc".to_string(),
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
            id: "HSM_CORE_RES_SAMPLING_MULT_SCANLINE_DIR".to_string(),
            description: "          Scanline Dir Multiplier".to_string(),
            initial: 100.0,
            minimum: 25.0,
            maximum: 1600.0,
            step: 25.0
        }, parse_parameter_string(r#"#pragma parameter HSM_CORE_RES_SAMPLING_MULT_SCANLINE_DIR			"          Scanline Dir Multiplier"  100 25 1600 25"#).unwrap())
    }
}
