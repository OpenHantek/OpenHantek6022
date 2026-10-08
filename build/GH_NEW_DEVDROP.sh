#!/bin/sh

set -u

# this is a "devdrop" release
RELEASE=devdrop
COMMENT=""

DATE=$(date +%Y%m%d:%H%M%S)

REPO=$(basename $(git remote get-url origin) .git)
OWNER=$(git remote get-url origin | cut -d: -f2 | cut -d/ -f1)

# must be exported to work for "hub" command
export GH_TOKEN=$(cat ~/.config/GitHub/REPO_TOKEN)

# remove the 'devdrop' tag from local git and remote GitHub
git tag --delete $RELEASE
git push --delete origin $RELEASE
# remove old 'devdrop' release from GitHub
gh release delete --yes $RELEASE

# push the changes
git push

# get the latest release
LATEST_RELEASE=$(git describe --abbrev=0)
RELEASE_VERSION=${RELEASE}-$(git describe --long)

# create a md link to the latest release
LATEST_RELEASE_WITH_LINK="[${LATEST_RELEASE}](https://github.com/${OWNER}/${REPO}/releases/tag/${LATEST_RELEASE})"

# get changes between latest release and this commit
GIT_LOG=$(git log --pretty=" - %s [%h]" ${LATEST_RELEASE}..HEAD | sed 's/ *$//g')

if [ -z "$GIT_LOG" ]; then
    RELEASE_NOTES=$(echo "${RELEASE}\n\n### ${RELEASE_VERSION}\n${COMMENT}There were no changes since release ${LATEST_RELEASE_WITH_LINK}.")
else
    RELEASE_NOTES=$(echo "${RELEASE}\n\n### ${RELEASE_VERSION}\n${COMMENT}Changes since release ${LATEST_RELEASE_WITH_LINK}:\n${GIT_LOG}\n")
fi

# create a changelog that is used by "gh release create"
CHANGELOG=CHANGELOG_since_${LATEST_RELEASE}
echo "$RELEASE_NOTES" | tee $CHANGELOG

# create a new 'devdrop' release
# this triggers GitHub actions to build and upload the release assets
gh release create --prerelease --title "${RELEASE_VERSION}" --notes-file "$CHANGELOG" $RELEASE

# finally forget the exported variable
unset GH_TOKEN
