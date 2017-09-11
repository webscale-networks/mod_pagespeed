#!/bin/bash

set -e
set -x

if [ -z "$WORKSPACE" ]; then
  WORKSPACE=$PWD
fi
if ! test -f "$WORKSPACE/mod_pagespeed/src/build.sh" -a -f "$WORKSPACE/mod_pagespeed/src/install/Makefile"; then
  printf "Something is wrong with the workspace\n" >&2
  exit 1
fi
    
# BRANCH_NAME is set by jenkins to the name of the branch or tag being built.
if [ -z "$BRANCH_NAME" ]; then
  printf "BRANCH_NAME must be set\n" >&2
  exit 1
fi


SPEC=$(basename $BRANCH_NAME)
echo "Building from $BRANCH_NAME"

SVN_OUT="svn+ssh://svn.webscalenetworks.com/repo/pagespeed/$SPEC"
if [ "${BRANCH_NAME/tags\//}" != "${BRANCH_NAME}" ]; then
  # Build for release when tagged
  BUILDTYPE=Release
  if svn info "$SVN_OUT" >/dev/null 2>&1; then
    printf "Pagespeed %s already built. Tag another version.\n" "$SPEC" >&2
    exit 1
  fi
else
  BUILDTYPE=Debug
fi

WORKSPACE=$PWD

if [ ! -d depot_tools ]; then
  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
  cd depot_tools
  git checkout --quiet --force c20f470011e2ea4d81527976f3bded2c13e258af
  cd ..
fi
export DEPOT_TOOLS_UPDATE=0
export PATH=$PWD/depot_tools:$PATH

cd mod_pagespeed
gclient config https://github.com/webscale-networks/mod_pagespeed.git --unmanaged --name=src
gclient sync --force --jobs=1

cd src
cat <<EOF >rebuild
make --jobs=4 \
  AR.host=$PWD/build/wrappers/ar.sh \
  AR.target=$PWD/build/wrappers/ar.sh \
  BUILDTYPE=$BUILDTYPE \
  all \
  mod_pagespeed_test \
  pagespeed_automatic_test
out/$BUILDTYPE/mod_pagespeed_test || true
out/$BUILDTYPE/pagespeed_automatic_test || true
EOF
chmod +x rebuild
./rebuild

cd $WORKSPACE

export DESTDIR=$WORKSPACE/pagespeed
rm -rf "$DESTDIR"

export APACHE_ROOT=$DESTDIR/usr/local/httpd-lg
export APACHE_MODULES=$APACHE_ROOT/modules
export APACHE_CONTROL_PROGRAM=$APACHE_ROOT/modules
export APACHE_DOC_ROOT=$(mktemp -d)
export APACHE_USER=$(id -un)
export APACHE_CONF_D=$APACHE_ROOT/conf
export BINDIR=$DESTDIR/usr/local/bin
export STAGING_DIR=$WORKSPACE/mod_pagespeed.install
export MOD_PAGESPEED_CACHE=$DESTDIR/var/cache/mod_pagespeed
export MOD_PAGESPEED_LOG=$DESTDIR/var/log/pagespeed

mkdir -p "$APACHE_MODULES" "$APACHE_CONF_D" "$BINDIR"

# The debug config is included by default, but contains a bunch of stuff we do not want
# The makefile defaults to using the Release build, but we may want to override this
sed -e 's/debug.conf.template *$//' -e "s/Release/$BUILDTYPE/g" \
  <mod_pagespeed/src/install/Makefile \
  >mod_pagespeed/src/install/Makefile.lagrange
BUILDTYPE=$BUILDTYPE \
make -C mod_pagespeed/src/install \
  -e \
  -f Makefile.lagrange \
  staging install

# No need to keep the Apache 2.2 version
rm pagespeed/usr/local/httpd-lg/modules/mod_pagespeed.so

sed -i -e "s@$DESTDIR@@g" $DESTDIR/usr/local/httpd-lg/conf/pagespeed.conf

rm -rf "$APACHE_DOC_ROOT"

if [ -z "$PAGESPEED_VERSION" ]; then
  PAGESPEED_VERSION=$(grep '#define MOD_PAGESPEED_VERSION_STRING' \
    mod_pagespeed/src/out/$BUILDTYPE/obj/gen/net/instaweb/public/version.h |
    awk '{print $3}' | tr -d '"')
fi
printf "pagespeed version is %s\n" "$PAGESPEED_VERSION"

if [ -z "$SPEC" ]; then
  exit 0
fi

cd "$DESTDIR"
if svn info "$SVN_OUT" >/dev/null 2>&1; then
  svn delete "$SVN_OUT" -m "Delete for replacement with $PAGESPEED_VERSION"
fi
svn import --no-ignore --quiet . $SVN_OUT -m "Update pagespeed/$SPEC to version $PAGESPEED_VERSION"
