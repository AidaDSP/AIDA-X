#!/bin/bash

set -e

cd $(dirname ${0})

# ---------------------------------------------------------------------------------------------------------------------

VERSION="${1}"

if [ -z "${VERSION}" ]; then
    echo "usage: ${0} <VERSION>"
    exit 1
fi

# ---------------------------------------------------------------------------------------------------------------------

mkdir -p ../build/innosetup-6.0.5

dlfile="../build/innosetup-6.0.5.exe"
pkgdir="$(realpath ../build/innosetup-6.0.5)"
drivec="${pkgdir}/drive_c"
iscc="${drivec}/InnoSetup/ISCC.exe"

export WINEARCH=win64
export WINEDLLOVERRIDES="mscoree,mshtml="
export WINEPREFIX="${pkgdir}"

# ---------------------------------------------------------------------------------------------------------------------
# download and install innosetup

if [ ! -f "${dlfile}" ]; then
    # FIXME proper dl version
    curl -L https://jrsoftware.org/download.php/is.exe?site=2 -o "${dlfile}"
fi

if [ ! -d "${drivec}" ]; then
    xvfb-run wineboot -u
fi

if [ ! -f "${drivec}/InnoSetup/ISCC.exe" ]; then
    xvfb-run wine "${dlfile}" /allusers /dir=C:\\InnoSetup /nocancel /norestart /verysilent
fi

# ---------------------------------------------------------------------------------------------------------------------
# create innosetup installer

echo "#define VERSION \"${VERSION}\"" > ../build/version.iss
xvfb-run wine "${iscc}" "inno/win64.iss"

# ---------------------------------------------------------------------------------------------------------------------
