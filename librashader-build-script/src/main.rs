use clap::Parser;
use std::fs::File;
use std::io::{BufWriter, Write};
use std::path::{Path, PathBuf};
use std::process::Command;
use std::{env, fs};

#[derive(Parser, Debug)]
#[command(version, about)]
struct Args {
    #[arg(long, default_value = "debug", global = true)]
    profile: String,
    #[arg(long, global = true)]
    target: Option<String>,
}

pub fn main() {
    // Do not update files on docsrs
    if env::var("DOCS_RS").is_ok() {
        return;
    }

    let args = Args::parse();

    let profile = args.profile;

    let crate_dir = Path::new("librashader-capi");
    println!("Building librashader C API...");

    let mut cmd = Command::new("cargo");
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

    Some(cmd.status().expect("Failed to build librashader-capi"));

    let mut output_dir = PathBuf::from(format!("target/{}", profile));
    if let Some(target) = &args.target {
        output_dir = PathBuf::from(format!("target/{}/{}", target, profile));
    }

    let output_dir = output_dir
        .canonicalize()
        .expect("Could not find output directory.");

    println!("Generating C headers...");

    // Create headers.
    let mut buf = BufWriter::new(Vec::new());
    cbindgen::generate(crate_dir)
        .expect("Unable to generate bindings")
        .write(&mut buf);

    let bytes = buf.into_inner().expect("Unable to extract bytes");
    let string = String::from_utf8(bytes).expect("Unable to create string");
    File::create(output_dir.join("librashader.h"))
        .expect("Unable to open file")
        .write_all(string.as_bytes())
        .expect("Unable to write bindings.");

    println!("Moving artifacts...");
    if cfg!(target_os = "macos") {
        let artifacts = &["liblibrashader_capi.dylib", "liblibrashader_capi.a"];
        for artifact in artifacts {
            let ext = artifact.strip_prefix("lib").unwrap();
            let ext = ext.replace("_capi", "");
            fs::rename(output_dir.join(artifact), output_dir.join(ext)).unwrap();
        }
    } else if cfg!(target_family = "unix") {
        let artifacts = &["liblibrashader_capi.so", "liblibrashader_capi.a"];
        for artifact in artifacts {
            let ext = artifact.strip_prefix("lib").unwrap();
            let ext = ext.replace("_capi", "");
            fs::rename(output_dir.join(artifact), output_dir.join(ext)).unwrap();
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
            println!("Renaming {artifact} to {ext}");
            fs::rename(output_dir.join(artifact), output_dir.join(ext)).unwrap();
        }

        if output_dir.join("librashader_capi.pdb").exists() {
            println!("Renaming librashader_capi.pdb to librashader.pdb");
            fs::rename(
                output_dir.join("librashader_capi.pdb"),
                output_dir.join("librashader.pdb"),
            )
            .unwrap();
        }
    }
}
