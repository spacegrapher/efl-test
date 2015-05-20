Name:           transform_feedback_elm
Summary:        transform_feedback app using glview using elm evas to test openglEs 3.0 
Version:        0.0.1
Release:        02
ExclusiveArch:  %arm
License:        Samsung
Source0:        %{name}-%{version}.tar.gz
Provides:       gles3_example
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(native-buffer)
BuildRequires:  pkgconfig(gles20)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(appsvc)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(evas)
BuildRequires:  efl-assist-devel
BuildRequires:  edje-bin


%description
transform_feedback_elm 

%prep
%setup -q

%build
cd %{_builddir}/%{name}-%{version}/
make clean
make

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/opt/usr/gles3_example
cp -a transform_feedback_elm %{buildroot}/opt/usr/gles3_example


%files
%defattr(-,root,root,-)
/opt/usr/gles3_example/transform_feedback_elm

