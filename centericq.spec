Version: 0.1
Summary: Console ncurses based ICQ client
Name: centericq
Release: 3
Copyright: GPL
Group: Applications/Communication
Source: http://konst.org.ua/download/%{name}-%{version}.tar.gz
URL: http://konst.org.ua/centericq/
Packager: Konstantin Klyagin <konst@konst.org.ua>
BuildRoot: /var/tmp/%{name}-buildroot/

%description
Console icq client for Linux. Has a useful ncurses menu- and window-driven
interface, allows to view contact list as a menu, send/receive messages,
URLs and contacts, search for users, view users' details, register new ICQ
UIN if one hasn't got one, maintain the contact list directly from the
program, have one's own ignore list and notify a user when new mail arrives.
Also if can associate events with sounds and you can customize them for users
personally.

%prep
%setup

%build
./configure --prefix=/usr
make

%install
rm -rf $RPM_BUILD_ROOT
make prefix=$RPM_BUILD_ROOT/usr sysconfdir=$RPM_BUILD_ROOT/etc install
find $RPM_BUILD_ROOT/usr -type f -print | grep -v '\/(README|COPYING|INSTALL|TODO|ChangeLog|AUTHORS|FAQ)$' | \
    sed "s@^$RPM_BUILD_ROOT@@g" > %{name}-%{version}-filelist

%clean
rm -rf $RPM_BUILD_ROOT

%files -f %{name}-%{version}-filelist
%defattr(-, root, root)

%doc README COPYING INSTALL TODO ChangeLog FAQ AUTHORS

%changelog
