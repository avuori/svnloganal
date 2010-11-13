#!/bin/sh

VERSION=55  # CHANGE THIS ON EACH RELEASE


PACKAGE=svnloganal.$VERSION.tar.gz
TARGET="/tmp/svnloganal.$VERSION"
rm -rf $TARGET # there might be one already.
mkdir $TARGET
git archive master | tar -x -C $TARGET
cd /tmp/
tar czf $PACKAGE "svnloganal.$VERSION"
rm -rf $TARGET
echo "Version $VERSION package ready: /tmp/$PACKAGE"
