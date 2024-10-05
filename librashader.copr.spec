%global commit 1dca2a97d03fc6aa531a03ba7aaa9ca3dbcb5a61
%global shortcommit %(c=%{commit}; echo ${c:0:7})

Name:     librashader
%define lname librashader0
%define profile optimized
Version:    {{{ git_dir_version }}}
Release:  %autorelease
Summary:  RetroArch shaders for all
License:  MPL-2.0
URL:      https://github.com/SnowflakePowered/%{name}
%undefine _disable_source_fetch
Source:   {{{ git_dir_pack }}}
BuildRequires: gcc
BuildRequires: git
BuildRequires: g++
BuildRequires: ninja-build
BuildRequires: patchelf
BuildRequires: rustc
BuildRequires: cargo

%description
RetroArch shader runtime

Summary:        RetroArch shader runtime
Provides:       librashader

%prep
{{{ git_dir_setup_macro }}}

%build
# need to use stable compiler, but enable nightly features
RUSTC_BOOTSTRAP=1 cargo run -p librashader-build-script -- --profile %{profile}

%install
mkdir -p %{buildroot}/%{_libdir}
mkdir -p %{buildroot}/%{_includedir}/librashader
patchelf --set-soname librashader.so.2 target/%{profile}/librashader.so
install -m 0755 target/%{profile}/librashader.so %{buildroot}%{_libdir}/librashader.so
cp target/%{profile}/librashader.h %{buildroot}%{_includedir}/librashader/librashader.h 
cp include/librashader_ld.h %{buildroot}%{_includedir}/librashader/librashader_ld.h 


%files 
%{_libdir}/librashader.so
%{_libdir}/librashader.so.2
%{_includedir}/librashader/
