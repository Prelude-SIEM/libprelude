%{!?python_sitelib: %global python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}
%{!?python_sitearch: %global python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}
%{!?ruby_sitearch: %global ruby_sitearch %(ruby -rrbconfig -e 'puts Config::CONFIG["sitearchdir"] ')}


Name:  libprelude
Epoch:  1
Version: 1.0.1
Release: 1%{?dist}
Summary: The prelude library
Group:  System Environment/Libraries
License: LGPLv2+
URL:  http://prelude-ids.org/
Source0: http://www.prelude-ids.org/download/releases/%{name}/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: gnutls-devel, python-devel, ruby, ruby-devel, lua-devel
BuildRequires: swig chrpath

%description
Libprelude is a library that guarantees secure connections between
all sensors and the Prelude Manager. Libprelude provides an 
Application Programming Interface (API) for the communication with
Prelude sub-systems, it supplies the necessary functionality for
generating and emitting IDMEF events with Prelude and automates the
saving and re-transmission of data in times of temporary interruption
of one of the components of the system.

%package devel
Summary: Header files and libraries for libprelude development
Group:  System Environment/Libraries
Requires: libprelude = %{epoch}:%{version}-%{release}, automake, gnutls-devel

%description devel
Libraries, include files, etc you can use to develop Prelude IDS
sensors using the Prelude Library.

%package python
Summary: Python bindings for libprelude
Group:  System Environment/Libraries
Requires: libprelude = %{epoch}:%{version}-%{release}

%description python
Python bindings for libprelude.

%package perl
Summary: Perl bindings for libprelude
Group:  System Environment/Libraries
%if 0%{?rhel} && 0%{?rhel} <= 5
BuildRequires: perl
%else
BuildRequires: perl-devel
%endif
Requires: libprelude = %{epoch}:%{version}-%{release}
Requires: perl(:MODULE_COMPAT_%(eval "`%{__perl} -V:version`"; echo $version))

%description perl
Perl bindings for libprelude.

%package ruby
Summary: Ruby bindings for libprelude
Group:  System Environment/Libraries
Requires: libprelude = %{epoch}:%{version}-%{release}
Requires: ruby(abi) = 1.8

%description ruby
Ruby bindings for libprelude.

%prep
%setup -q

%build
%configure --disable-static \
  --with-html-dir=%{_defaultdocdir}/%{name}-%{version}/html \
  --with-perl-installdirs=vendor \
  --enable-easy-bindings

# removing rpath
sed -i.rpath -e 's|LD_RUN_PATH=""||' bindings/Makefile.in
sed -i.rpath -e 's|^sys_lib_dlsearch_path_spec="/lib /usr/lib|sys_lib_dlsearch_path_spec="/%{_lib} %{_libdir}|' libtool

make %{?_smp_mflags} 

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_defaultdocdir}/%{name}-%{version}
mkdir -p %{buildroot}%{perl_vendorarch}
make install DESTDIR=%{buildroot} INSTALL="%{__install} -c -p"
cp -p AUTHORS ChangeLog README NEWS COPYING LICENSE.README HACKING.README \
 %{buildroot}%{_defaultdocdir}/%{name}-%{version}
rm -f %{buildroot}/%{_libdir}/libprelude.la
chmod 755 %{buildroot}%{python_sitearch}/_prelude.so
find %{buildroot} -type f \( -name .packlist -o -name perllocal.pod \) -exec rm -f {} ';'
find %{buildroot} -type f -name '*.bs' -a -size 0 -exec rm -f {} ';'
rm -f %{buildroot}%{_libdir}/*.la
rm -f %{buildroot}%{ruby_sitearch}/PreludeEasy.la
chmod +w %{buildroot}%{perl_vendorarch}/auto/Prelude/Prelude.so
chrpath -d %{buildroot}%{perl_vendorarch}/auto/Prelude/Prelude.so
chmod -w %{buildroot}%{perl_vendorarch}/auto/Prelude/Prelude.so

# Fix time stamp for both 32 and 64 bit libraries
touch -r ./configure.in %{buildroot}%{_sysconfdir}/prelude/default/*

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%{_bindir}/prelude-admin
%{_bindir}/prelude-adduser
%{_libdir}/*.so.*
%{_mandir}/man1/prelude-admin.1.gz
%config(noreplace) %{_sysconfdir}/*
%{_localstatedir}/spool/*
%dir %{_defaultdocdir}/%{name}-%{version}/
%doc %{_defaultdocdir}/%{name}-%{version}/*

%files devel
%defattr(-,root,root)
%{_bindir}/libprelude-config
%{_libdir}/*.so
%{_libdir}/pkgconfig/libprelude.pc
%dir %{_includedir}/libprelude/
%{_includedir}/libprelude/*
%{_datadir}/aclocal/libprelude.m4

%files python
%defattr(-,root,root)
%{python_sitearch}/*

%files perl
%defattr(0755,root,root)
%attr(0644,root,root) %{perl_vendorarch}/Prelude*.pm
%{perl_vendorarch}/auto/Prelude*/

%files ruby
%defattr(-,root,root)
%{ruby_sitearch}/PreludeEasy.so

%changelog
* Wed Jun 15 2011 Vincent Quéméner <vincent.quemener@cs.fr> - 1.0.0-6
- Rebuilt for RHEL6

* Wed Jul 21 2010 David Malcolm <dmalcolm@redhat.com> - 1:1.0.0-5
- Rebuilt for https://fedoraproject.org/wiki/Features/Python_2.7/MassRebuild

* Tue Jun 01 2010 Marcela Maslanova <mmaslano@redhat.com> - 1:1.0.0-4
- Mass rebuild with perl-5.12.0

* Sun May 02 2010 Steve Grubb <sgrubb@redhat.com> - 1.0.0-3
- Fix requires statements

* Fri Apr 30 2010 Steve Grubb <sgrubb@redhat.com> - 1.0.0-2
- New upstream release

* Sat Jan 30 2010 Steve Grubb <sgrubb@redhat.com> - 1.0.0rc1-1
- New upstream release

* Mon Jan 11 2010 Steve Grubb <sgrubb@redhat.com> - 0.9.25-1
- New upstream release

* Mon Dec  7 2009 Stepan Kasal <skasal@redhat.com> - 0.9.24.1-2
- rebuild against perl 5.10.1

* Tue Sep 29 2009 Steve Grubb <sgrubb@redhat.com> - 0.9.24.1-1
- New upstream release

* Sat Aug 8 2009 Manuel "lonely wolf" Wolfshant <wolfy@fedoraproject.org> - 0.9.24-3
- adjust to build in EL-5

* Sat Jul 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.9.24-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Thu Jul 09 2009 Steve Grubb <sgrubb@redhat.com> - 0.9.24-1
- New upstream release

* Mon Jun 08 2009 Steve Grubb <sgrubb@redhat.com> - 0.9.23-1
- New upstream release

* Wed Apr 29 2009 Steve Grubb <sgrubb@redhat.com> - 0.9.22-1
- New upstream release

* Fri Apr 03 2009 Steve Grubb <sgrubb@redhat.com> 0.9.21.2-9
- remove check section, doesn't work on anything except x86 anyways

* Fri Mar 13 2009 Karsten Hopp <karsten@redhat.com> 0.9.21.2-8
- don't buildrequire valgrind on s390x, similar to ppc

* Wed Feb 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.9.21.2-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Sat Jan 24 2009 Steve Grubb <sgrubb@redhat.com> - 0.9.21.2-6
- Rebuild for MySQL 5.1.30

* Fri Dec 05 2008 Steve Grubb <sgrubb@redhat.com> - 0.9.21.2-5
- Rebuild _again_ for Python 2.6

* Thu Dec  4 2008 Michael Schwendt <mschwendt@fedoraproject.org> - 0.9.21.2-4
- Include /usr/include/libpreludecpp directory.

* Tue Dec 02 2008 Steve Grubb <sgrubb@redhat.com> - 0.9.21.2-3
- Disable check target
- Rebuild for Python 2.6

* Mon Oct 13 2008 Steve Grubb <sgrubb@redhat.com> - 0.9.21.2-1
- New upstream bugfix release

* Mon Oct 06 2008 Steve Grubb <sgrubb@redhat.com> - 0.9.21.1-1
- New upstream bugfix release
- resolves: #465228 - prelude-admin is looking for tls.conf in /usr

* Fri Sep 19 2008 Steve Grubb <sgrubb@redhat.com> - 0.9.21-1
- New upstream bugfix release

* Tue Sep 09 2008 Steve Grubb <sgrubb@redhat.com> - 0.9.20.2-1
- New upstream bugfix release

* Fri Sep 05 2008 Steve Grubb <sgrubb@redhat.com> - 0.9.20.1-1
- New upstream bugfix release
- Get rid of rpath and enable test suite except on PPC

* Wed Sep 03 2008 Steve Grubb <sgrubb@redhat.com> - 0.9.20-1
- New upstream release

* Tue Aug 05 2008 Steve Grubb <sgrubb@redhat.com> - 0.9.19-1
- New upstream release with ruby bindings

* Mon Jul 21 2008 Steve Grubb <sgrubb@redhat.com> - 0.9.18.1-1
- New upstream version

* Fri Jul 04 2008 Steve Grubb <sgrubb@redhat.com> - 0.9.17.2-1
- Update to latest upstream and update perl bindings generation (#453932)

* Wed Jun 25 2008 Tomas Mraz <tmraz@redhat.com> - 0.9.17.1-2
- fixed build of perl bindings

* Tue Jun 24 2008 Steve Grubb <sgrubb@redhat.com>
- rebuild for new gnutls

* Fri May 02 2008 Steve Grubb <sgrubb@redhat.com> 0.9.17.1-1
- New upstream version

* Thu Apr 24 2008 Steve Grubb <sgrubb@redhat.com> 0.9.17-1
- New upstream version

* Wed Feb 20 2008 Fedora Release Engineering <rel-eng@fedoraproject.org> - 0.9.16.2-2
- Autorebuild for GCC 4.3

* Wed Jan 23 2008 Steve Grubb <sgrubb@redhat.com> 0.9.16.2-1
- New upstream version

* Mon Jan 14 2008 Steve Grubb <sgrubb@redhat.com> 0.9.16.1-1
- moved to new upstream version 0.9.16.1

* Tue Feb 20 2007 Thorsten Scherf <tscherf@redhat.com> 0.9.13-1
- moved to new upstream version 0.9.13-1

* Fri Jan 05 2007 Thorsten Scherf <tscherf@redhat.com> 0.9.12.1-1
- moved to new upstream version 0.9.12.1

* Tue Dec 30 2006 Thorsten Scherf <tscherf@redhat.com> 0.9.12-6
- fixed x86_86 arch problem

* Tue Dec 30 2006 Thorsten Scherf <tscherf@redhat.com> 0.9.12-5
- added ExcludeArch

* Tue Dec 29 2006 Thorsten Scherf <tscherf@redhat.com> 0.9.12-4
- resolved permission problems
- added new docs 

* Tue Dec 25 2006 Thorsten Scherf <tscherf@redhat.com> 0.9.12-3
- changed dir owner and preserved timestamps when building the package
- resolved rpath problems

* Fri Dec 22 2006 Thorsten Scherf <tscherf@redhat.com> 0.9.12-2
- moved perl_sidearch into perl_vendorarch
- minor corrections in the spec file

* Fri Dec 22 2006 Thorsten Scherf <tscherf@redhat.com> 0.9.12-1
- upgrade to latest upstream version 0.9.12
- minor corrections in the spec file

* Wed Dec 20 2006 Thorsten Scherf <tscherf@redhat.com> 0.9.11-4
- removing smp-flag to debug perl- and python-problems
- added perl-bindings again

* Wed Dec 20 2006 Thorsten Scherf <tscherf@redhat.com> 0.9.11-3
- disabled perl-bindings

* Mon Nov 20 2006 Thorsten Scherf <tscherf@redhat.com> 0.9.11-2
- Some minor fixes in requirements

* Tue Oct 24 2006 Thorsten Scherf <tscherf@redhat.com> 0.9.11-1
- New Fedora build based on release 0.9.11
