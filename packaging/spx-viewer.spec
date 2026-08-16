Name:           spx-viewer
Version:        1.0.0
Release:        1%{?dist}
Summary:        Viewer for Apple System Information SPX reports
License:        MIT
URL:            https://github.com/mingistech/Apple-System-Information-for-Linux
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.16
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
BuildRequires:  desktop-file-utils
Requires:       qt6-qtbase-gui

%description
SPX Viewer is a Linux desktop application for opening Apple System
Information .spx reports. It parses the XML/property-list structure and
presents hardware, network, and software details in a browsable interface.

This package does not collect information from the Linux computer. It is a
viewer for reports generated on macOS with System Information → File → Save.

%prep
%setup -q

%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop

%files
%license LICENSE
%doc README.md
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/mime/packages/%{name}.xml

%changelog
* Sun Aug 16 2026 Brandon <mingistech@users.noreply.github.com> - 1.0.0-1
- Initial RPM package
