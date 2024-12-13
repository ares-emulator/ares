set -eu
case ${GITHUB_REF} in
  refs/tags/*) suffix="-${GITHUB_REF#refs/tags/}" ;;
  refs/heads/master) suffix="-nightly" ;;
  *) suffix="" ;;
esac

srcdir="${GITHUB_WORKSPACE}/src"
bindir="${GITHUB_WORKSPACE}/bin"

# Hack: Workaround for GitHub artifacts losing attributes.
chmod +x ${bindir}/ares-macos-universal/ares.app/Contents/MacOS/ares

for package in macos-universal
do
  mkdir "${package}"
  cd "${package}"

  # Package ares.
  outdir=ares${suffix}
  mkdir ${outdir}
  mkdir ${outdir}-dSYMs
  cp -ar ${bindir}/ares-${package}-dSYMs/*.dSYM ${outdir}-dSYMs
  cp -ar ${bindir}/ares-${package}/*.app ${outdir}
  zip -r ../ares-${package}.zip ${outdir}
  zip -r ../ares-${package}-dSYMs.zip ${outdir}-dSYMs

  cd -
done

for package in windows-x64 windows-clang-cl-x64 windows-clang-cl-arm64
do
  mkdir "${package}"
  cd "${package}"

  # Package ares.
  outdir=ares${suffix}
  mkdir ${outdir}
  mkdir ${outdir}-PDBs
  cp -ar ${bindir}/ares-${package}-PDBs/*.pdb ${outdir}-PDBs
  zip -r ../ares-${package}-PDBs.zip ${outdir}-PDBs
  rm -rf ${bindir}/ares-${package}/PDBs
  cp -ar ${bindir}/ares-${package}/* ${outdir}
  zip -r ../ares-${package}.zip ${outdir}
  cd -
done
