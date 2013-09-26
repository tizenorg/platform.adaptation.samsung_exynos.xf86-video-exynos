Name:       xf86-video-exynos
Summary:    X.Org X server driver for exynos
Version:    0.2.93
Release:    1
ExclusiveArch:  %arm
Group:      System/X Hardware Support
License:    Samsung
Source0:    %{name}-%{version}.tar.gz


%description
This package provides the driver for the Samsung display device exynos


%prep
%setup -q


%build

%reconfigure

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%files
%defattr(-,root,root,-)
%{_libdir}/xorg/modules/drivers/*.so

