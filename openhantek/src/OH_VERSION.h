// SPDX-License-Identifier: GPL-2.0-or-later

// set OH_VERSION to define a release
// if defined in this file it will tag the commit automatically
// via .git/hooks/post-commit (see below)
// the hook will then renamed from OH_VERSION to LAST_OH_VERSION
//


// next line shall define either OH_VERSION or LAST_OH_VERSION
//
#define LAST_OH_VERSION "3.3.2.2"


// do not edit below

// VERSION (git describe --tags --dirty) will be shown by OpenHantek
// if VERSION is not defined then use OH_VERSION
// if this is also not defined fall back to build date

#ifndef VERSION
#ifdef OH_VERSION
#define VERSION OH_VERSION
#else
#define VERSION __DATE__
#endif
#endif

/* content of ".git/hooks/post-commit":

#!/bin/bash

# this file is called automatically after a commit
# it tags the commit if a version is defined in the version file
# inspired by: https://coderwall.com/p/mk18zq/automatic-git-version-tagging-for-npm-modules

# this file was updated during development (by script build/MK_NEW_VER)
#
OPENHANTEK="$(git rev-parse --show-toplevel)"
OH_VERSION_H="$OPENHANTEK/openhantek/src/OH_VERSION.h"

# check if the last commit changed the entry OH_VERSION in file ...OH_VERSION.h and extract the new version
#
OH_VERSION=$(git diff HEAD^..HEAD -- ${OH_VERSION_H} | awk '/^\+#define OH_VERSION/ { print $3 }' | tr -d '"')

# if commit was marked as OH_VERSION then tag it accordingly and change entry to LAST_OH_VERSION
#
if [ "$OH_VERSION" != "" ]; then
    git tag -a $OH_VERSION -m "$(git log -1 --format=%s)"
    sed -i 's|^#define[[:blank:]]*OH_VERSION|#define LAST_OH_VERSION|g' $OH_VERSION_H
    echo "Created a new tag: $OH_VERSION"
fi

# update the build system with next build
touch $OPENHANTEK/CMakeLists.txt

*/
