Name:           warzone2100
Version:        2.3_beta8
Release:        1%{?dist}
Summary:        Innovative 3D real-time strategy
%define		distributor wz2100.net

Group:          Amusements/Games
License:        GPLv2+ and CC-BY-SA
URL:            http://wz2100.net/
Source0:        http://download.gna.org/warzone/releases/2.2/%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: physfs-devel openal-devel bison flex zip
BuildRequires: libjpeg-devel libpng-devel libogg-devel libvorbis-devel
BuildRequires: popt-devel gettext
BuildRequires: automake >= 1.8
BuildRequires: desktop-file-utils

%description
Warzone 2100 was one of the first 3D RTS games ever. It was released commercially 
by Pumpkin Studies in 1999, and released in 2004 under the GPL. Development was
continued by the Warzone 2100 Resurrection project to produce this improved version.

%prep
%setup -q

%build
echo wc_uri=tags/%{version} > autorevision.conf
%configure --disable-rpath --disable-debug --with-distributor=%{distributor}
make %{_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
rm -rf $RPM_BUILD_ROOT%{_defaultdocdir}
desktop-file-install --vendor=%{distributor} \
        --dir=$RPM_BUILD_ROOT%{_datadir}/applications/   \
        $RPM_BUILD_ROOT%{_datadir}/applications/%{name}.desktop
rm $RPM_BUILD_ROOT%{_datadir}/applications/%{name}.desktop
%find_lang %{name}

%clean
rm -rf $RPM_BUILD_ROOT

%files -f %{name}.lang
%defattr(-,root,root,-)
%{_bindir}/warzone2100
%{_datadir}/icons/warzone*
%{_datadir}/applications/%{distributor}-%{name}.desktop
%{_datadir}/warzone2100
%doc AUTHORS ChangeLog COPYING COPYING.NONGPL COPYING.README

%changelog
* Mon May 11 2009 Per Inge Mathisen <per mathisen at gmail.com>
- 2.2_rc1 version

* Sat Mar 07 2009 Per Inge Mathisen <per mathisen at gmail.com>
- First version - spec file for the most part shamelessly copied from the Fedora project
