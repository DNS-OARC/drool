Name:           drool
Version:        1.0.0.rc.1
Release:        1%{?dist}
Summary:        DNS Replay Tool
Group:          Productivity/Networking/DNS/Utilities

License:        BSD-3-Clause
URL:            https://github.com/DNS-OARC/drool
Source0:        %{name}_%{version}.orig.tar.gz

BuildRequires:  libpcap-devel libev-devel
BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  libtool

%description
DNS Replay Tool


%prep
%setup -q -n %{name}_%{version}


%build
sh autogen.sh
%configure
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%check
make test


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%{_bindir}/*
%{_datadir}/doc/*
%{_mandir}/man1/*
%{_mandir}/man5/*


%changelog
* Mon Sep 18 2017 Jerry Lundström <lundstrom.jerry@gmail.com> 1.0.0.rc.1-1
- Release 1.0.0-rc.1
  * Mostly documentation updates and clarifications but also a new timing
    mode `best_effort` to send packets as fast as possible without warnings
    and a correct usage of `nanosleep()` on systems that does not support
    `clock_nanosleep()`.
  * Commits:
    114f984 Fix #74: Best effort timing mode, fix usage of `nanosleep()`
    a32201c Rework development notes
    3999214 Add examples and default values
    1e70ab9 Design documentation
    6e530ee Fix #64: More documentation around context and client_pool,
            and how configuration relates to each other.
* Tue Aug 22 2017 Jerry Lundström <lundstrom.jerry@gmail.com> 1.0.0.beta.4-1
- Release 1.0.0-beta.4
  * Compatibility fixes and packaging.
  * Commits:
    f6b6251 Fix #66: Check for libev/ev.h also
    8dff723 Code formatting
    a5af21d Update pcap-thread to v2.1.3
    e622612 Prepare SPEC for OSB/COPR
    3c8fbb0 Fix #57: Update README with build tools and environment setup
    456d586 Add debian packaging files
    a0a74cb Add dependencies
* Wed Mar 29 2017 Jerry Lundström <lundstrom.jerry@gmail.com> 1.0.0.beta.3-1
- Release 1.0.0-beta.3
  * Various compatibility fixes across GNU/Linux, FreeBSD, OpenBSD and
    OS X, along with a bugfix in pcap-thread.
  * Special thanks to Brian Carpenter (@geeknik) for fuzzing drool.
  * Commits:
    82273cc Fix #50: Compat for OS X
    fc9d87c Issue #50: Update pcap-thread to v2.1.2
    04817f0 const not needed on size_t
    7ab1811 Update submodules: sllq v1.0.0, parseconf v1.0.0 and omg-dns
            v1.0.0
    242c67a Fix CID 1421861
    9f3914f Add Travis and Coverity badges
    ae33c90 Check for `clock_gettime()` in librt, needed for older glibc
    594c877 Add Travis-CI
    b7e19a2 Fix compiler warnings and check errors from `sigwait()`
    3b9f2dc Fix #43, fix #44, fix #45: Update pcap-thread to fix one off
            in IP layer
* Sat Mar 25 2017 Jerry Lundström <lundstrom.jerry@gmail.com> 1.0.0.beta.2-1
- Release 1.0.0-beta.2
  * Some minor changes and documentation updates prior to making the
    repository public.
  * Commits:
    79ce5cc Add description
    3f63cff Update all submodules to latest develop
    0bbe9dc Log format
* Mon Mar 06 2017 Jerry Lundström <lundstrom.jerry@gmail.com> 1.0.0.beta.1-1
- Initial package
