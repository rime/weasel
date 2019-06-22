#!/bin/bash

set -e

invalid_args() {
    echo "Usage: $(basename $0) <old_version> <new_version>|major|minor|patch"
    exit 1
}

match_line() {
    grep --quiet --fixed-strings "$@"
}

version_pattern='([0-9]+)\.([0-9]+)\.([0-9]+)'

old_version=$1
new_version=$2
echo "old_version='$old_version'"
echo "new_version='$new_version'"

if [[ $old_version =~ $version_pattern ]]; then
    old_major=${BASH_REMATCH[1]}
    old_minor=${BASH_REMATCH[2]}
    old_patch=${BASH_REMATCH[3]}
else
    invalid_args
fi

new_major=$old_major
new_minor=$old_minor
new_patch=$old_patch

if [[ $new_version =~ $version_pattern ]]; then
    new_major=${BASH_REMATCH[1]}
    new_minor=${BASH_REMATCH[2]}
    new_patch=${BASH_REMATCH[3]}
elif [[ $new_version = 'major' ]]; then
    ((++new_major))
    new_minor=0
    new_patch=0
elif [[ $new_version = 'minor' ]]; then
    ((++new_minor))
    new_patch=0
elif [[ $new_version = 'patch' ]]; then
    ((++new_patch))
else
    invalid_args
fi

# verify old_version is current version.
match_line "WEASEL_VERSION_STR \"${old_version}\"" include/WeaselVersion.h &&
    match_line "WEASEL_VERSION=${old_version}" build.bat &&
    match_line "version: ${old_version}" appveyor.yml || (
        echo >&2 "${old_version} doesn't match current version."
        exit 1
    )

old_version="${old_major}.${old_minor}.${old_patch}"
new_version="${new_major}.${new_minor}.${new_patch}"
echo "updating ${old_version} => ${new_version}"


if [[ $OSTYPE =~ darwin ]]; then
    L_BOUND='[[:<:]]'
    R_BOUND='[[:>:]]'
else
    L_BOUND='\<'
    R_BOUND='\>'
fi

SEP='\([,.]\)'

version_pattern="${L_BOUND}${old_major}${SEP}${old_minor}${SEP}${old_patch}${R_BOUND}"
replacement=${new_major}'\1'${new_minor}'\2'${new_patch}

edit_rc_file() {
    local file="$1"
    iconv -f 'utf-16le' -t 'utf-8' "${file}" | \
        sed "s/${version_pattern}/${replacement}/g" | \
        iconv -f 'utf-8' -t 'utf-16le' > "${file}".new
    mv "${file}".new "${file}"
}

edit_source_file() {
    local file="$1"
    sed -i'~' "s/${version_pattern}/${replacement}/g" "${file}"
    rm "${file}~"
}

update_pub_date() {
    local file="$1"
    local today=$(date -R)
    sed -i'~' "s/<pubDate>[^<]*/<pubDate>${today}/" "${file}"
    rm "${file}~"
}

update_changelog() {
    local version="$1"
    clog --from-latest-tag \
         --changelog CHANGELOG.md \
         --repository https://github.com/rime/weasel \
         --setversion "${version}"
}

edit_rc_file WeaselServer/WeaselServer.rc

edit_source_file include/WeaselVersion.h
edit_source_file build.bat
edit_source_file appveyor.yml

edit_source_file update/testing-appcast.xml
edit_source_file update/appcast.xml
update_pub_date update/testing-appcast.xml
update_pub_date update/appcast.xml

update_changelog "${new_version}"
${VISUAL:-${EDITOR:-vim}} CHANGELOG.md
match_line "## ${new_version} " CHANGELOG.md || (
    echo >&2 "CHANGELOG.md has no changes for version ${new_version}."
    exit 1
)
bash update/write-release-notes.sh

release_message="chore(release): ${new_version} :tada:"
release_tag="${new_version}"
git commit --all --message "${release_message}"
git tag --annotate "${release_tag}" --message "${release_message}"
