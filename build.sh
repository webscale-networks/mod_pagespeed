#!/bin/bash

set -e
set -x

PRODUCT_VERSION=$1
BUILDTYPE=$2
REVISION=$3

if [ -z "$WORKSPACE" ]; then
  WORKSPACE=$PWD
fi
if ! test -f "$WORKSPACE/mod_pagespeed/build.sh" -a -f "$WORKSPACE/mod_pagespeed/install/Makefile"; then
  printf "Something is wrong with the workspace\n" >&2
  exit 1
fi

# BRANCH_NAME is set by jenkins to the name of the branch or tag being built.
if [ -z "$BRANCH_NAME" ]; then
  printf "BRANCH_NAME must be set\n" >&2
  exit 1
fi

if [ -z "$PRODUCT_VERSION" ]; then
  printf "PRODUCT_VERSION must be set\n" >&2
  exit 1
fi

if [ -z "$BUILDTYPE" ]; then
  printf "BUILDTYPE must be set\n" >&2
  exit 1
fi

SVN_OUT="file:///repo/pagespeed/$PRODUCT_VERSION"
if [ "$BUILDTYPE" = "Release" ]; then
  if svn info "$SVN_OUT" >/dev/null 2>&1; then
    printf "Pagespeed %s already built. Tag another version.\n" "$PRODUCT_VERSION" >&2
    exit 1
  fi
fi

WORKSPACE=$PWD

cd mod_pagespeed
python build/gyp_chromium --depth=.

if [ -n "$REVISION" ]; then
  sed -i.orig -Ee "s/LASTCHANGE=[^ ]+/LASTCHANGE=$REVISION/" build/lastchange.sh
fi

cat <<EOF >rebuild
make --jobs=4 \
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
export APACHE_CONF_DIR=$APACHE_ROOT/conf
export BINDIR=$DESTDIR/usr/local/bin
export STAGING_DIR=$WORKSPACE/mod_pagespeed.install
export MOD_PAGESPEED_CACHE=$DESTDIR/var/cache/mod_pagespeed
export MOD_PAGESPEED_LOG=$DESTDIR/var/log/pagespeed

mkdir -p "$APACHE_MODULES" "$APACHE_CONF_DIR" "$BINDIR"

# The debug config is included by default, but contains a bunch of stuff we do not want
# The makefile defaults to using the Release build, but we may want to override this
sed -e 's/debug.conf.template *$//' -e "s/Release/$BUILDTYPE/g" \
  <mod_pagespeed/install/Makefile \
  >mod_pagespeed/install/Makefile.lagrange
BUILDTYPE=$BUILDTYPE \
make -C mod_pagespeed/install \
  -e \
  -f Makefile.lagrange \
  staging install

# No need to keep the Apache 2.2 version
rm pagespeed/usr/local/httpd-lg/modules/mod_pagespeed.so

sed -i -e "s@$DESTDIR@@g" $DESTDIR/usr/local/httpd-lg/conf/pagespeed.conf

rm -rf "$APACHE_DOC_ROOT"

cd "$DESTDIR"
if svn info "$SVN_OUT" >/dev/null 2>&1; then
  svn delete "$SVN_OUT" -m "Delete for replacement from $BRANCH_NAME"
fi
svn import --no-ignore --quiet . $SVN_OUT -m "Update pagespeed/$PRODUCT_VERSION from $BRANCH_NAME"
