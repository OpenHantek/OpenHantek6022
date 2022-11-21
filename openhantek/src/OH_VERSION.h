// SPDX-License-Identifier: GPL-2.0-or-later

// define OH_VERSION as the version that is shown on top of the program
// if defined in this file it will tag the commit automatically
// via .git/hooks/post-commit (see below)
// the hook will then renamed from OH_VERSION to LAST_OH_VERSION
//
// if OH_VERSION is undefined VERSION ($COMMIT_DATE-$COMMIT_HASH) will be shown by OpenHantek

// next line shall define either OH_VERSION or LAST_OH_VERSION
//
#define LAST_OH_VERSION "3.3.1"


// do not edit below

#ifdef GIT_DESCRIBE
#undef VERSION
#define VERSION GIT_DESCRIBE
#elif defined OH_VERSION
#undef VERSION
#define VERSION OH_VERSION
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
