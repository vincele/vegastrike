Name: vegastrike
Summary: Vegastrike - a free 3D space fight simulator
Version: 0.2.1cvs
Release: 1
Copyright: GPL
Group: X11/GL/Games
Source: http://prdownloads.sourceforge.net/vegastrike/vegastrike-0.2.1cvs.tar.gz
Packager: Alexander Rawass <alexannika@users.sourceforge.net>
BuildRoot: /tmp/vsbuild
Prefix: /usr/local

%description
Vegastrike is a free 3D space fight simulator under the GPL

Vega Strike is an Interactive Flight Simulator/Real Time Stratagy being
 developed for Linux and Windows in 3d OpenGL... With stunning Graphics,
 Vega Strike will be a hit for all gamers!!!


%prep
rm -rf $RPM_BUILD_ROOT/*
%setup -n vegastrike-0.2.1cvs

%build
./configure --prefix=/usr/local --enable-debug
make

%install
make DESTDIR=$RPM_BUILD_ROOT install

cd $RPM_BUILD_ROOT/usr/local/games/vegastrike/data
mv vegastrike vegastrike-bin
mv select glselect

%clean
rm -rf $RPM_BUILD_ROOT/*

%files
%{prefix}/games/vegastrike/data/vegastrike
%{prefix}/games/vegastrike/data/select
