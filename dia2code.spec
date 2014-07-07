%define name dia2code
%define ver 0.8.7
%define prefix /usr
%define rel 1

######################################################################
Summary: Dia2Code generates code from a Dia diagram
Name: %name
Version: %ver
Release: %rel
License: GPL
Distribution: N/A
Vendor: N/A
Group: Development/Tools
Requires: libxml2
Buildroot: /var/tmp/%{name}-%{ver}-root
Source: http://download.sourceforge.net/dia2code/dia2code-%{ver}.tar.gz
Packager: Oliver Kellogg <okellogg@users.souceforge.net>
URL: http://dia2code.sourceforge.net/
######################################################################
%description
Dia2Code is a small utility used to generate code from a Dia diagram.
Dia is a program to make diagrams. If you didn't know it, 
you might consider checking its  homepage first.
http://www.lysator.liu.se/~alla/dia/
Dia2Code is still under development, but you may find this version useful.
######################################################################
%prep
%setup -q
######################################################################
%build

if [ -x ./configure ]; then
  CFLAGS=$RPM_OPT_FLAGS ./configure  --prefix=%{prefix}
else
  CFLAGS=$RPM_OPT_FLAGS ./autogen.sh --prefix=%{prefix}
fi
make
######################################################################
%install
rm -fr $RPM_BUILD_ROOT
make prefix=$RPM_BUILD_ROOT%{prefix} install-strip
mkdir $RPM_BUILD_ROOT/usr/man
mkdir $RPM_BUILD_ROOT/usr/man/man1
cp dia2code.1 $RPM_BUILD_ROOT/usr/man/man1
######################################################################
%clean
rm -rf $RPM_BUILD_ROOT

######################################################################
%files
%defattr(-,root,root)
%doc AUTHORS ChangeLog COPYING README TODO dia2code.lsm

%{prefix}/bin/dia2code
%{prefix}/man/man1/dia2code.1.gz

#####################################################################
%changelog
* Sat Sep 15 2001 Javier O'Hara <joh314@users.sourceforge.net>
- Added the libxml2 dependency.
- Assigned the group to Development/Tools.
- Took out the kaptain script, as it would generate a dependency on Kaptain.
- Added the man page.
* Tue Sep 04 2001 Richard Torkar <Richard.Torkar@htu.se>
- Made sure dia2code.kaptn is copied, this should really be made by a autogen.sh script
- Added kaptain as a prereq
* Sun Sep 02 2001 Javier O'Hara <joh314@users.sourceforge.net>
- Added dia2code.kaptn to files section
* Wed Jan 24 2001 Richard Torkar <ds98rito@thn.htu.se>
- Made the spec file for dia2code
- Contacted the maintainer Javier O'Hara
