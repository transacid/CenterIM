Version: 4.21.0
Summary: Console ncurses based IM client. ICQ, Yahoo!, MSN, AIM, IRC, Gadu-Gadu and Jabber protocols are supported. Internal RSS reader and LiveJournal client are also provided.
Name: centericq
License: GPL
Release: 1
Group: Applications/Communication
Source: http://konst.org.ua/download/%{name}-%{version}.tar.gz
URL: http://konst.org.ua/centericq/
Packager: Konstantin Klyagin <konst@konst.org.ua>
BuildRoot: /var/tmp/%{name}-buildroot/
Requires: ncurses >= 4.2, openssl, curl

%description
centericq is a text mode menu- and window-driven IM interface. Currently
ICQ2000, Yahoo!, MSN, AIM TOC, IRC, Gadu-Gadu and Jabber protocols are
supported. It allows you to send, receive, and forward messages, URLs and,
SMSes, mass message send, search for users (including extended "whitepages
search"), view users' details, maintain your contact list directly from the
program (including non-icq contacts), view the messages history,
register a new UIN and update your details, be informed on receiving
email messages, automatically set away after the defined period of
inactivity (on any console), and have your own ignore, visible and
invisible lists. It can also associate events with sounds, has support
for Hebrew and Arabic languages and allows to arrange contacts into
groups. Internal RSS reader and a client for LiveJournal are provided.

%prep
%setup

%build
./configure --prefix=/usr
make

%install
rm -rf $RPM_BUILD_ROOT
make prefix=$RPM_BUILD_ROOT/usr sysconfdir=$RPM_BUILD_ROOT/etc install
find $RPM_BUILD_ROOT/usr -type f -print | grep -v '\/(README|COPYING|INSTALL|TODO|ChangeLog|AUTHORS|FAQ)$' | \
    sed "s@^$RPM_BUILD_ROOT@@g" | sed "s/^\(.*\)$/\1\*/" > %{name}-%{version}-filelist

%clean
rm -rf $RPM_BUILD_ROOT

%files -f %{name}-%{version}-filelist
%defattr(-, root, root)

%doc README COPYING INSTALL TODO ChangeLog FAQ AUTHORS THANKS

%changelog
