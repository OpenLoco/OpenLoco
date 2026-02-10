#!/bin/sh
set -e

# Ensure we are in root directory and have master checkedout
basedir="$(readlink -f `dirname $0`/..)"
cd $basedir
echo "Checking out master"
git checkout master
git pull https://github.com/OpenLoco/OpenLoco.git master

# Get version number for release
DEFAULT_NUM="$(date +'%y.%m')"
read -p "Enter version number for release [$DEFAULT_NUM]: " VERSION_NUMBER
VERSION_NUMBER=${VERSION_NUMBER:-$DEFAULT_NUM}
VERSION_AND_DATE="$VERSION_NUMBER ($(date +'%Y-%m-%d'))"

# Update all relevant files
echo "Set Changelog version: $VERSION_AND_DATE"
sed -i "1s/.*/$VERSION_AND_DATE/" CHANGELOG.md
sed -i "s/Version: \[e\.g\. .*\]/Version: \[e\.g\. $VERSION_NUMBER\]/" .github/ISSUE_TEMPLATE/bug_report.md
sed -i "s/OPENLOCO_VERSION: .*/OPENLOCO_VERSION: $VERSION_NUMBER/" .github/workflows/ci.yml

# Get relevant section of the changelog for the release tag message
CHANGELOG=$(perl -0777 -e 'print <> =~ /-----.*?\n(.*?)\n.* \(.*\)\n------.*?/s;' CHANGELOG.md)
echo "Changelog for version v$VERSION_NUMBER: $CHANGELOG"

# Commit and tag the changes
git commit CHANGELOG.md .github/ISSUE_TEMPLATE/bug_report.md .github/workflows/ci.yml -m"Update version numbers for v$VERSION_NUMBER"
git tag -a v$VERSION_NUMBER -m"Release $VERSION_NUMBER

$CHANGELOG"

# Push the result
git push https://github.com/OpenLoco/OpenLoco.git master --follow-tags
