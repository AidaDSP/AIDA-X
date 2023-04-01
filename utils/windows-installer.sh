#!/bin/bash

set -e

cd $(dirname ${0})

# ---------------------------------------------------------------------------------------------------------------------

BUILDDIR="${1}"
VERSION="${2}"

if [ -z "${BUILDDIR}" ] || [ -z "${VERSION}" ]; then
    echo "usage: ${0} <BUILDDIR> <VERSION>"
    exit 1
fi

if [ ! -d "${BUILDDIR}" ]; then
    echo "BUILDDIR does not exist"
    exit 1
fi

# ---------------------------------------------------------------------------------------------------------------------

mkdir -p "${BUILDDIR}/innosetup-6.0.5"

dlfile="${BUILDDIR}/innosetup-6.0.5.exe"
pkgdir="$(realpath ${BUILDDIR}/innosetup-6.0.5)"
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

ls -al "${BUILDDIR}/bin"

sed \
    -e "s|@VERSION@|${VERSION}|g" \
    -e "s|@BINDIR@|Z:${BUILDDIR}/bin|g" \
    -e "s|@UTILSDIR@|Z:${PWD}|g" \
    inno/win64.iss > "${BUILDDIR}/win64.iss"

xvfb-run wine "${iscc}" "Z:${BUILDDIR}/win64.iss"

# ---------------------------------------------------------------------------------------------------------------------
