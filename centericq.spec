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
./configure --prefix=%{buildroot}/usr/
make

%install
make install

%files
%doc README COPYING INSTALL TODO FAQ ChangeLog

/usr/bin/*
/usr/share/*
