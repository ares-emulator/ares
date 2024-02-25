%global commit 1dca2a97d03fc6aa531a03ba7aaa9ca3dbcb5a61
%global shortcommit %(c=%{commit}; echo ${c:0:7})

Name:     librashader
%define lname librashader0
%define profile optimized
Version:  0.2.0~beta.2
Release:  %autorelease
Summary:  RetroArch shaders for all
License:  MPL-2.0
URL:      https://github.com/SnowflakePowered/%{name}
%undefine _disable_source_fetch
Source:  https://github.com/SnowflakePowered/%{name}/archive/%{commit}/%{name}-%{shortcommit}.tar.gz
BuildRequires: gcc
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
%autosetup -n %{name}-%{commit}

%build
# need to use stable compiler, but enable nightly features
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
%{_includedir}/librashader/
