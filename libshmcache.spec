
%define LibShmcacheTool   libshmcache-tool
%define LibShmcacheConfig libshmcache-config
%define LibShmcacheDevel  libshmcache-devel
%define LibShmcacheDebuginfo  libshmcache-debuginfo
%define CommitVersion %(echo $COMMIT_VERSION)

Name: libshmcache
Version: 1.0.5
Release: 1%{?dist}
Summary: a high performance local share memory cache for multi processes
License: LGPL
Group: Arch/Tech
URL:  http://github.com/happyfish100/libshmcache/
Source: http://github.com/happyfish100/libshmcache/%{name}-%{version}.tar.gz

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n) 

BuildRequires: libfastcommon-devel >= 1.0.35

Requires: %__cp %__mv %__chmod %__grep %__mkdir %__install %__id
Requires: libfastcommon >= 1.0.35

%description
libshmcache is a local share memory cache for multi processes.
it is high performance because read is lockless.
this project contains C library and PHP extension.
commit version: %{CommitVersion}

%package tool
Summary: tool commands
Requires: %{name}%{?_isa} = %{version}-%{release}

%description tool
This package provides tool commands
commit version: %{CommitVersion}

%package config
Summary: libshmcache config file

%description config
This package provides libshmcache config file
commit version: %{CommitVersion}

%package devel
Summary: Development header file
Requires: %{name}%{?_isa} = %{version}-%{release}

%description devel
This package provides the header files of libshmcache
commit version: %{CommitVersion}

%prep

%setup -q

%build
cd src && make clean && make
cd tools && make clean && make

%install
rm -rf %{buildroot}
cd src && DESTDIR=$RPM_BUILD_ROOT make install
cd tools && DESTDIR=$RPM_BUILD_ROOT make install
mkdir -p $RPM_BUILD_ROOT/etc/
cp -f ../../conf/libshmcache.conf $RPM_BUILD_ROOT/etc/

%post

%preun

%postun

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
/usr/lib64/libshmcache.so*
/usr/lib/libshmcache.so*

%files tool
/usr/bin/shmcache_set
/usr/bin/shmcache_get
/usr/bin/shmcache_delete
/usr/bin/shmcache_stats
/usr/bin/shmcache_remove_all

%files config
/etc/libshmcache.conf

%files devel
%defattr(-,root,root,-)
/usr/include/shmcache/*

%changelog
* Fri Dec 23 2016  Yu Qing <yuqing@yongche.com>
- first RPM release (1.0)
