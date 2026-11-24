#
# RPM spec for pdfmergegs
#
# Build (from the project root):
#   rpmbuild -ta pdfmergegs-1.0.0.tar.gz
# or, inside the source tree:
#   rpmbuild -bb deploy/rpm/pdfmergegs.spec --define "_sourcedir $PWD/.." \
#            --define "_builddir $PWD/../build-rpm"
#
# Tested target: openSUSE Leap 15.6 (KDE Plasma 6), also valid for Tumbleweed.
# The Qt6 package names below are the openSUSE ones; on Fedora use
# qt6-qtbase-devel, qt6-qtpdf-devel, qt6-qttools-devel (identical names).

Name:           pdfmergegs
Version:        1.0.0
Release:        1%{?dist}
Summary:        Merge PDF documents with Ghostscript
License:        GPL-3.0-or-later
URL:            https://github.com/bkumpar/pdfmergegs
Source0:        %{name}-%{version}.tar.gz
BuildArch:      x86_64

BuildRequires:  cmake >= 3.16
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtpdf-devel
BuildRequires:  qt6-qttools-devel
BuildRequires:  desktop-file-utils

Requires:       ghostscript
Requires:       qt6-qtbase
Requires:       qt6-qtpdf

%description
A small Qt 6 desktop utility for merging several PDF documents into one
file. The actual merge is performed by Ghostscript, so the application
offers selectable quality presets (lossless, print, e-mail) instead of
re-implementing PDF writing.

Features: drag and drop from Dolphin, first-page thumbnails, reordering,
detection of digitally signed documents, command line usage
(pdfmergegs file1.pdf file2.pdf) and an English/Croatian interface.

%lang(hr) is not used: the Croatian translation is compiled into the
binary as a Qt resource, so there are no external .qm files to package.

%prep
%autosetup

%build
%cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DCMAKE_INSTALL_LIBDIR=%{_libdir} \
    -DCMAKE_INSTALL_BINDIR=%{_bindir} \
    -DCMAKE_INSTALL_DATADIR=%{_datadir}
%cmake_build

%install
%cmake_install

# Sanity check on the generated desktop entry
desktop-file-validate %{buildroot}%{_datadir}/applications/pdfmergegs.desktop || true

%files
%license LICENSE
%doc README.md
%{_bindir}/pdfmergegs
%{_datadir}/applications/pdfmergegs.desktop
%{_datadir}/icons/hicolor/scalable/apps/pdfmergegs.svg

%changelog
* Tue Nov 24 2026 Boris Kumpar <bkumpar@gmail.com> - 1.0.0-1
- Initial package
