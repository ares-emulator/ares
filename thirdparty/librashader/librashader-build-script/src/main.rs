use carlog::*;
use clap::Parser;
use std::fs::File;
use std::io::{BufWriter, Write};
use std::path::{Path, PathBuf};
use std::process::{Command, ExitCode};
use std::{env, fs};

#[derive(Parser, Debug)]
#[command(version, about)]
struct Args {
    #[arg(long, default_value = "debug", global = true)]
    profile: String,
    #[arg(long, global = true)]
    target: Option<String>,
    #[arg(long, default_value_t = false, global = true)]
    stable: bool,
    #[arg(last = true)]
    cargoflags: Vec<String>,
}

pub fn main() -> ExitCode {
    // Do not update files on docsrs
    if env::var("DOCS_RS").is_ok() {
        return ExitCode::SUCCESS;
    }

    let args = Args::parse();

    let profile = args.profile;

    let crate_dir = Path::new("librashader-capi");
    carlog_info!("Building", "librashader C API");

    let cargo = env::var("CARGO").unwrap_or_else(|_| "cargo".to_string());
    let mut cmd = Command::new(&cargo);

    cmd.arg("build");
    cmd.args(["--package", "librashader-capi"]);
    cmd.arg(format!(
        "--profile={}",
        if profile == "debug" { "dev" } else { &profile }
    ));

    // If we're on RUSTC_BOOTSTRAP, it's likely because we're building for a package..
    if env::var("RUSTC_BOOTSTRAP").is_ok() {
        cmd.arg("--ignore-rust-version");
    }

    if let Some(target) = &args.target {
        cmd.arg(format!("--target={}", &target));
    }

    if args.stable {
        carlog_warning!("building librashader with stable Rust compatibility");
        carlog_warning!("C headers will not be generated");
        cmd.args(["--features", "stable"]);
    }
    if !args.cargoflags.is_empty() {
        cmd.args(args.cargoflags);
    }

    let Ok(status) = cmd.status().inspect_err(|err| {
        carlog_error!("failed to build librashader-capi");
        carlog_error!(format!("{err}"));
    }) else {
        return ExitCode::FAILURE;
    };

    if !status.success() {
        return ExitCode::from(status.code().unwrap_or(1) as u8);
    }

    let mut output_dir = PathBuf::from(format!("target/{}", profile));
    if let Some(target) = &args.target {
        output_dir = PathBuf::from(format!("target/{}/{}", target, profile));
    }

    let Ok(output_dir) = output_dir.canonicalize() else {
        carlog_error!("could not find output directory");
        println!("help: are you running the build script from the repository root?");
        return ExitCode::FAILURE;
    };

    if args.stable {
        carlog_warning!("generating C headers is not supported when building for stable Rust");
    } else {
        carlog_info!("Generating", "librashader C API headers");

        // Create headers.
        let mut buf = BufWriter::new(Vec::new());
        let Ok(bindings) = cbindgen::generate(crate_dir).inspect_err(|err| {
            carlog_error!("unable to generate C API headers");
            carlog_error!(format!("{err}"));
        }) else {
            return ExitCode::FAILURE;
        };

        bindings.write(&mut buf);
        let bytes = buf.into_inner().expect("Unable to extract bytes");
        let string = String::from_utf8(bytes).expect("Unable to create string");

        let Ok(mut file) = File::create(output_dir.join("librashader.h")).inspect_err(|err| {
            carlog_error!("unable to open librashader.h");
            carlog_error!(format!("{err}"));
        }) else {
            return ExitCode::FAILURE;
        };

        let Ok(_) = file.write_all(string.as_bytes()).inspect_err(|err| {
            carlog_error!("unable to write to librashader.h");
            carlog_error!(format!("{err}"));
        }) else {
            return ExitCode::FAILURE;
        };
    }

    carlog_info!("Moving", "built artifacts");
    if cfg!(target_os = "macos") {
        let artifacts = &["liblibrashader_capi.dylib", "liblibrashader_capi.a"];
        for artifact in artifacts {
            let ext = artifact.strip_prefix("lib").unwrap();
            let ext = ext.replace("_capi", "");

            let Ok(_) =
                fs::rename(output_dir.join(artifact), output_dir.join(&ext)).inspect_err(|err| {
                    carlog_error!(format!("Unable to rename {artifact} to {}", &ext));
                    carlog_error!(format!("{err}"));
                })
            else {
                return ExitCode::FAILURE;
            };
            carlog_ok!("Renamed", format!("{artifact} to {}", &ext));
        }
    } else if cfg!(target_family = "unix") {
        let artifacts = &["liblibrashader_capi.so", "liblibrashader_capi.a"];
        for artifact in artifacts {
            let ext = artifact.strip_prefix("lib").unwrap();
            let ext = ext.replace("_capi", "");
            let Ok(_) =
                fs::rename(output_dir.join(artifact), output_dir.join(&ext)).inspect_err(|err| {
                    carlog_error!(format!("Unable to rename {artifact} to {}", &ext));
                    carlog_error!(format!("{err}"));
                })
            else {
                return ExitCode::FAILURE;
            };
            carlog_ok!("Renamed", format!("{artifact} to {}", &ext));
        }
    }

    if cfg!(target_os = "windows") {
        let artifacts = &[
            "librashader_capi.dll",
            "librashader_capi.lib",
            "librashader_capi.d",
            "librashader_capi.dll.exp",
            "librashader_capi.dll.lib",
        ];
        for artifact in artifacts {
            let ext = artifact.replace("_capi", "");
            let Ok(_) =
                fs::rename(output_dir.join(artifact), output_dir.join(&ext)).inspect_err(|err| {
                    carlog_error!(format!("Unable to rename {artifact} to {}", &ext));
                    carlog_error!(format!("{err}"));
                })
            else {
                return ExitCode::FAILURE;
            };
            carlog_ok!("Renamed", format!("{artifact} to {}", &ext));
        }

        if output_dir.join("librashader_capi.pdb").exists() {
            fs::rename(
                output_dir.join("librashader_capi.pdb"),
                output_dir.join("librashader.pdb"),
            )
            .unwrap();
            carlog_ok!("Renamed", "librashader_capi.pdb to librashader.pdb");
        }
    }

    ExitCode::SUCCESS
}
