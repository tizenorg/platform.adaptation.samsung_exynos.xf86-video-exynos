Name:       xf86-video-exynos
Summary:    X.Org X server driver for exynos
Version:    0.0.1
Release:    1
ExclusiveArch:  %arm
Group:      Graphics & UI Framework/X Window System
License:    MIT
Source0:    %{name}-%{version}.tar.gz
Source1001:     xf86-video-exynos.manifest
BuildRequires:  prelink
BuildRequires:  pkgconfig(xorg-macros)
BuildRequires:  pkgconfig(pixman-1)
BuildRequires:  pkgconfig(xorg-server)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xproto)
BuildRequires:  pkgconfig(fontsproto)
BuildRequires:  pkgconfig(randrproto)
BuildRequires:  pkgconfig(renderproto)
BuildRequires:  pkgconfig(videoproto)
BuildRequires:  pkgconfig(resourceproto)
BuildRequires:  pkgconfig(libudev)
BuildRequires:  pkgconfig(libtbm)
#BuildRequires:  pkgconfig(xdbg)
BuildRequires:  pkgconfig(libdrm)
BuildRequires:  pkgconfig(libdrm_exynos)
BuildRequires:  pkgconfig(check)
Requires:       xf86-misc-exynos4412
%description
This package provides the Display driver for the Samsung device exynos

%prep

%setup -q
# >> setup
cp %{SOURCE1001} .
# << setup

%build
# >> build pre
# << build pre

%reconfigure
make %{?jobs:-j%jobs}

# >> build post
# << build post
%install
# >> install pre
# << install pre
%make_install

# >> install post
execstack -c %{buildroot}%{_libdir}/xorg/modules/drivers/exynos_drv.so
# << install post

%files
# >> files exynos
%defattr(-,root,root,-)
%manifest %{name}.manifest
#license COPYING
%dir %{_libdir}/xorg/modules/drivers
%{_libdir}/xorg/modules/drivers/*.so
%{_mandir}/man4/*
# << files exynos
