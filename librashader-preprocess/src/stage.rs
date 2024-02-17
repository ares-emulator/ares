use crate::{PreprocessError, SourceOutput};
use std::str::FromStr;

enum ActiveStage {
    Both,
    Fragment,
    Vertex,
}

impl FromStr for ActiveStage {
    type Err = PreprocessError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "vertex" => Ok(ActiveStage::Vertex),
            "fragment" => Ok(ActiveStage::Fragment),
            _ => Err(PreprocessError::InvalidStage),
        }
    }
}

#[derive(Default)]
pub(crate) struct ShaderOutput {
    pub(crate) fragment: String,
    pub(crate) vertex: String,
}

pub(crate) fn process_stages(source: &str) -> Result<ShaderOutput, PreprocessError> {
    let mut active_stage = ActiveStage::Both;
    let mut output = ShaderOutput::default();

    for line in source.lines() {
        if let Some(stage) = line.strip_prefix("#pragma stage ") {
            let stage = stage.trim();
            active_stage = ActiveStage::from_str(stage)?;
            continue;
        }

        if line.starts_with("#pragma name ")
            || line.starts_with("#pragma format ")
            || line.starts_with("#pragma parameter ")
        {
            continue;
        }

        match active_stage {
            ActiveStage::Both => {
                output.fragment.push_line(line);
                output.vertex.push_line(line);
            }
            ActiveStage::Fragment => {
                output.fragment.push_line(line);
            }
            ActiveStage::Vertex => output.vertex.push_line(line),
        }
    }

    Ok(output)
}
