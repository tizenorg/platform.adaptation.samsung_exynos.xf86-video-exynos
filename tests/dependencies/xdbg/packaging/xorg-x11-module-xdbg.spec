Name: xorg-x11-module-xdbg
Summary: Xserver debug module
Version: 0.1.7
Release:    1
Group:      System/Libraries
License:    MIT
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(xorg-server)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xext)
BuildRequires:  pkgconfig(xorg-macros)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(bigreqsproto)
BuildRequires:  pkgconfig(compositeproto)
BuildRequires:  pkgconfig(damageproto)
BuildRequires:  pkgconfig(dmxproto)
BuildRequires:  pkgconfig(dri2proto)
BuildRequires:  pkgconfig(fixesproto)
BuildRequires:  pkgconfig(fontsproto)
BuildRequires:  pkgconfig(gestureproto)
BuildRequires:  pkgconfig(inputproto)
BuildRequires:  pkgconfig(kbproto)
BuildRequires:  pkgconfig(randrproto)
BuildRequires:  pkgconfig(recordproto)
BuildRequires:  pkgconfig(renderproto)
BuildRequires:  pkgconfig(resourceproto)
BuildRequires:  pkgconfig(scrnsaverproto)
BuildRequires:  pkgconfig(videoproto)
BuildRequires:  pkgconfig(xcmiscproto)
BuildRequires:  pkgconfig(xextproto)
BuildRequires:  pkgconfig(xf86bigfontproto)
BuildRequires:  pkgconfig(xf86dgaproto)
BuildRequires:  pkgconfig(xf86driproto)
BuildRequires:  pkgconfig(xf86vidmodeproto)
BuildRequires:  pkgconfig(xineramaproto)
BuildRequires:  pkgconfig(xproto)
BuildRequires:  pkgconfig(libdrm)


%description
This package provides the runtime debug library and module for debug of inside X server.

%package devel
Summary: X server runtime debug library development package
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires: pkgconfig(xorg-server)
Requires: pkgconfig(x11)

%description devel
X server runtime debug library development package

%prep
%setup -q

%build
%reconfigure --disable-static --prefix=/usr \
  CFLAGS="$CFLAGS -Wall -Werror" \
  LDFLAGS="$LDFLAGS -Wl,--hash-style=both -Wl,--as-needed"
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp -af COPYING %{buildroot}/usr/share/license/%{name}
%make_install

%remove_docs

%files
%defattr(-,root,root,-)
/usr/share/license/%{name}
%{_bindir}/xdbg
%{_libdir}/libxdbg-log.so.*
%{_libdir}/xorg/modules/libxdbg.so

%files devel
%dir %{_includedir}/xdbg/
%{_includedir}/xdbg/*.h
%{_libdir}/libxdbg-log.so
%{_libdir}/pkgconfig/xdbg.pc

