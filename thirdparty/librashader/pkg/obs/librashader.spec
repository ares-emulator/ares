Name:     librashader
%define lname librashader0
%define profile optimized
Summary:  RetroArch shaders for all
License:  MPL-2.0
Version: 0.2.0~beta.2
Release: 0
URL:      https://github.com/SnowflakePowered/%{name}
Source0:  librashader-%{version}.tar.xz
Source1:  vendor.tar.xz
Source2:  cargo_config
BuildRequires: ninja-build
BuildRequires: patchelf
BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: cargo
BuildRequires: rust

%description
RetroArch shader runtime

Summary:        RetroArch shader runtime
Provides:       librashader

%prep
%setup -qa1 
mkdir .cargo                # cargo automatically uses this dir
cp %{SOURCE2} .cargo/config # and automatically uses this config

%build
RUSTC_BOOTSTRAP=1 cargo run -p librashader-build-script -- --profile %{profile}

%install
mkdir -p %{buildroot}/%{_libdir}
mkdir -p %{buildroot}/%{_includedir}/librashader
patchelf --set-soname librashader.so.1 target/%{profile}/librashader.so
install -m 0755 target/%{profile}/librashader.so %{buildroot}%{_libdir}/librashader.so
cp target/%{profile}/librashader.h %{buildroot}%{_includedir}/librashader/librashader.h 
cp include/librashader_ld.h %{buildroot}%{_includedir}/librashader/librashader_ld.h 


%files 
%{_libdir}/librashader.so
%if !0%{?suse_version}
%{_libdir}/librashader.so.1
%endif
%{_includedir}/librashader/
