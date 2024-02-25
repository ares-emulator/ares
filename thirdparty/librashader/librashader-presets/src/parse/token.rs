use crate::error::ParsePresetError;
use crate::parse::Span;
use nom::branch::alt;
use nom::bytes::complete::{is_not, take_until};
use nom::character::complete::{char, line_ending, multispace1, not_line_ending};
use std::ops::RangeFrom;

use nom::combinator::{eof, map_res, value};
use nom::error::{ErrorKind, ParseError};

use nom::sequence::delimited;
use nom::{
    bytes::complete::tag, character::complete::multispace0, AsChar, IResult, InputIter,
    InputLength, InputTake, Slice,
};

#[derive(Debug)]
pub struct Token<'a> {
    pub key: Span<'a>,
    pub value: Span<'a>,
}

/// Return the input slice up to the first occurrence of the parser,
/// and the result of the parser on match.
/// If the parser never matches, returns an error with code `ManyTill`
pub fn take_up_to<Input, Output, Error: ParseError<Input>, P>(
    mut parser: P,
) -> impl FnMut(Input) -> IResult<Input, (Input, Output), Error>
where
    P: FnMut(Input) -> IResult<Input, Output, Error>,
    Input: InputLength + InputIter + InputTake,
{
    move |i: Input| {
        let input = i;
        for (index, _) in input.iter_indices() {
            let (rest, front) = input.take_split(index);
            match parser(rest) {
                Ok((remainder, output)) => return Ok((remainder, (front, output))),
                Err(_) => continue,
            }
        }
        Err(nom::Err::Error(Error::from_error_kind(
            input,
            ErrorKind::ManyTill,
        )))
    }
}

fn parse_assignment(input: Span) -> IResult<Span, ()> {
    let (input, _) = multispace0(input)?;
    let (input, _) = tag("=")(input)?;
    let (input, _) = multispace0(input)?;
    Ok((input, ()))
}

fn unbalanced_quote<I>(input: I) -> IResult<I, ()>
where
    I: Slice<RangeFrom<usize>> + InputIter + InputLength,
    <I as InputIter>::Item: AsChar,
    I: Copy,
{
    if let Ok((input, _)) = eof::<_, ()>(input) {
        Ok((input, ()))
    } else {
        let (input, _) = char('"')(input)?;
        Ok((input, ()))
    }
}

fn extract_from_quotes(input: Span) -> IResult<Span, Span> {
    // Allow unbalanced quotes because some presets just leave an open quote.
    let (input, between) = delimited(char('"'), is_not("\""), unbalanced_quote)(input)?;
    let (input, _) = opt_whitespace(input)?;
    let (input, _) = eof(input)?;
    Ok((input, between))
}

fn multiline_comment(i: Span) -> IResult<Span, Span> {
    delimited(tag("/*"), take_until("*/"), tag("*/"))(i)
}

fn single_comment(i: Span) -> IResult<Span, Span> {
    delimited(
        alt((tag("//"), tag("#"))),
        not_line_ending,
        alt((line_ending, eof)),
    )(i)
}

fn opt_whitespace(i: Span) -> IResult<Span, ()> {
    value(
        (), // Output is thrown away.
        multispace0,
    )(i)
}

fn optional_quotes(input: Span) -> IResult<(), Span> {
    let input = if let Ok((_, between)) = extract_from_quotes(input) {
        between
    } else {
        input
    };
    Ok(((), input))
}

fn parse_reference(input: Span) -> IResult<Span, Token> {
    let (input, key) = tag("#reference")(input)?;
    let (input, _) = multispace1(input)?;
    let (input, (_, value)) = map_res(not_line_ending, optional_quotes)(input)?;
    Ok((input, Token { key, value }))
}
fn parse_key_value(input: Span) -> IResult<Span, Token> {
    let (input, (key, _)) = take_up_to(parse_assignment)(input)?;
    let (input, (_, value)) = map_res(not_line_ending, optional_quotes)(input)?;
    let (_, value) =
        take_until::<_, _, nom::error::Error<Span>>("//")(value).unwrap_or((input, value));
    let (_, value) =
        take_until::<_, _, nom::error::Error<Span>>("#")(value).unwrap_or((input, value));
    let (_, (_, value)) = map_res(not_line_ending, optional_quotes)(value)?;
    Ok((input, Token { key, value }))
}

fn parse_tokens(mut span: Span) -> IResult<Span, Vec<Token>> {
    let mut values = Vec::new();
    while !span.is_empty() {
        // important to munch whitespace first.
        if let Ok((input, _)) = opt_whitespace(span) {
            span = input;
        }
        // handle references before comments because comments can start with #
        if let Ok((input, token)) = parse_reference(span) {
            span = input;
            values.push(token);
            continue;
        }
        if let Ok((input, _)) = multiline_comment(span) {
            span = input;
            continue;
        }
        if let Ok((input, _)) = single_comment(span) {
            span = input;
            continue;
        }
        let (input, token) = parse_key_value(span)?;
        span = input;
        values.push(token)
    }
    Ok((span, values))
}

pub fn do_lex(input: &str) -> Result<Vec<Token>, ParsePresetError> {
    let span = Span::new(input.trim_end());
    let (_, tokens) = parse_tokens(span).map_err(|e| match e {
        nom::Err::Error(e) | nom::Err::Failure(e) => {
            let input: Span = e.input;
            println!("{:?}", input);
            ParsePresetError::LexerError {
                offset: input.location_offset(),
                row: input.location_line(),
                col: input.get_column(),
            }
        }
        _ => ParsePresetError::LexerError {
            offset: 0,
            row: 0,
            col: 0,
        },
    })?;
    Ok(tokens)
}

#[cfg(test)]
mod test {
    use crate::parse::token::single_comment;

    #[test]
    fn parses_single_line_comment() {
        let parsed =
            single_comment("// Define textures to be used by the different passes\ntetx=n".into());
        eprintln!("{parsed:?}")
    }
}
