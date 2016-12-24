
%define LibShmcacheDevel  libshmcache-devel
%define LibShmcacheDebuginfo  libshmcache-debuginfo

Name: libshmcache
Version: 1.0.0
Release: 1%{?dist}
Summary: a high performance local share memory cache for multi processes
License: LGPL
Group: Arch/Tech
URL:  http://github.com/happyfish100/libshmcache/
Source: http://github.com/happyfish100/libshmcache/%{name}-%{version}.tar.gz

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n) 

Requires: %__cp %__mv %__chmod %__grep %__mkdir %__install %__id

%description
libshmcache is a local share memory cache for multi processes.
it is high performance because read is lockless.
this project contains C library and PHP extension.

%package devel
Summary: Development header file
Requires: %{name}%{?_isa} = %{version}-%{release}

%description devel
This pakcage provides the header files of libshmcache

%prep
%setup -q

%build
cd src && make clean && make

%install
rm -rf %{buildroot}
cd src && DESTDIR=$RPM_BUILD_ROOT make install

%post

%preun

%postun

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
/usr/lib64/libshmcache.so*
/usr/lib/libshmcache.so*

%files devel
%defattr(-,root,root,-)
/usr/include/shmcache/*

%changelog
* Mon Dec 23 2016  Yu Qing <yuqing@yongche.com>
- first RPM release (1.0)
