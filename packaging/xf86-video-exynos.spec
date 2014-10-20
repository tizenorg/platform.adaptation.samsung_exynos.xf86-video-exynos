%bcond_with x

Name:       xf86-video-exynos
Summary:    X.Org X server driver for exynos
Version:    0.0.1
Release:    1
Group:      System/X Hardware Support
License:    Samsung
Source0:    %{name}-%{version}.tar.gz

BuildRequires:  pkgconfig(xorg-macros)
BuildRequires:  pkgconfig(xorg-server)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xproto)
BuildRequires:  pkgconfig(dri3proto)
BuildRequires:  pkgconfig(fontsproto)
BuildRequires:  pkgconfig(randrproto)
BuildRequires:  pkgconfig(renderproto)
BuildRequires:  pkgconfig(videoproto)
BuildRequires:  pkgconfig(resourceproto)
BuildRequires:  pkgconfig(presentproto)
BuildRequires:  pkgconfig(libudev)
BuildRequires:  pkgconfig(libtbm)
BuildRequires:  pkgconfig(xdbg)
BuildRequires:  pkgconfig(libdrm)

%description
This package provides the driver for the Samsung display device exynos

%prep
%setup -q


%build
rm -rf %{buildroot}

%autogen --disable-static \
	CFLAGS="${CFLAGS} -Wall -mfpu=neon -DNO_CRTC_MODE -mfloat-abi=softfp" LDFLAGS="${LDFLAGS} -Wl,--hash-style=both -Wl,--as-needed"


make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp -af COPYING %{buildroot}/usr/share/license/%{name}
%make_install
rm -rf %{buildroot}/usr/share/man

%files
%defattr(-,root,root,-)
%{_libdir}/xorg/modules/drivers/*.so
/usr/share/license/%{name}
