Name:       xorg-x11-drv-exynos
Summary:    X.Org X server driver for exynos
Version:    0.0.1
Release:    1
ExclusiveArch:  %arm
Group:      Graphics & UI Framework/X Window System
License:    MIT
Source0:    %{name}-%{version}.tar.gz
Source1001:     xorg-x11-drv-exynos.manifest
BuildRequires:  prelink
BuildRequires:  cmake
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
BuildRequires:  pkgconfig(xdbg)
BuildRequires:  pkgconfig(libdrm)
BuildRequires:  pkgconfig(libdrm_exynos)
BuildRequires:  pkgconfig(check)

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

%cmake . 
make %{?jobs:-j%jobs}
# -- ctest --output-on-failure .

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
%attr(644,root,root) %{_sysconfdir}/X11/xorg.conf.d/10-exynos.conf
%config(noreplace) %{_sysconfdir}/X11/xorg.conf.d/10-exynos.conf
# {_mandir}/man/man4/*
# << files exynos
