// SPDX-License-Identifier: GPL-2.0-or-later

// define OH_VERSION as the version that is shown on top of the program
// if defined in this file it will tag the commit automatically
// via .git/hooks/post-commit (see below)
// the hook will then renamed from OH_VERSION to LAST_OH_VERSION
//
// if OH_VERSION is undefined (for development commits) OH_BUILD will be shown by OpenHantek

// next line shall define either OH_VERSION or LAST_OH_VERSION
//
#define OH_VERSION "3.3.0.1"


// do not edit below

#ifdef OH_VERSION
#undef VERSION
#define VERSION OH_VERSION
#else
#include "OH_BUILD.h"
#ifdef OH_BUILD
#undef VERSION
#define VERSION OH_BUILD
#endif
#endif

/* content of ".git/hooks/pre-commit":

#!/bin/sh

# this script is automatically run before committing
# it provides version info (shown in top line of the program)
# inspired by: https://gist.github.com/sg-s/2ddd0fe91f6037ffb1bce28be0e74d4e

# this file will be updated automatically by every commit
#
OH_BUILD_H="$(git rev-parse --show-toplevel)"/openhantek/src/OH_BUILD.h

# commit date
#
DATE=$(date +%Y%m%d)

# number of this commit (i.e. number of previous commits + 1)
#
COMMIT=$(( $(git log main --pretty=oneline | wc -l) + 1 ))

# define a string with commit date and number of this commit
#
echo "// Do not edit, will be re-created at each commit!" > ${OH_BUILD_H}
echo "#define OH_BUILD \"$DATE - commit $COMMIT\"" >> ${OH_BUILD_H}

# and finally stage the change
#
git add ${OH_BUILD_H}

*/

/* content of ".git/hooks/post-commit":

#!/bin/bash

# this file is called automatically after a commit
# it tags the commit if a version is defined in the version file
# inspired by: https://coderwall.com/p/mk18zq/automatic-git-version-tagging-for-npm-modules

# this file was updated during development (by script build/MK_NEW_VER)
#
OH_VERSION_H="$(git rev-parse --show-toplevel)"/openhantek/src/OH_VERSION.h

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

*/
