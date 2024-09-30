#!/bin/bash

set -e

invalid_args() {
    echo "Usage: $(basename $0) <new_version>|major|minor|patch"
    exit 1
}

match_line() {
    grep --quiet --fixed-strings "$@"
}

get_current_version() {
    local file="$1"
    local keyword="$2"
    local line="$(grep --fixed-strings "set ${keyword}=" "${file}")"
    local pattern='set '"${keyword}"'=([0-9]+)'
    if [[ "${line}" =~ $pattern ]]; then
        echo ${BASH_REMATCH[1]}
    else
        echo 0
    fi
}

old_major=$(get_current_version build.bat VERSION_MAJOR)
old_minor=$(get_current_version build.bat VERSION_MINOR)
old_patch=$(get_current_version build.bat VERSION_PATCH)

old_version="${old_major}.${old_minor}.${old_patch}"

new_version=$1

new_major=$old_major
new_minor=$old_minor
new_patch=$old_patch

version_pattern='([0-9]+)\.([0-9]+)\.([0-9]+)'

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

new_version="${new_major}.${new_minor}.${new_patch}"
echo "updating ${old_version} => ${new_version}"

update_version_number() {
    local file="$1"
    local variable="$2"
    local old_value="$3"
    local new_value="$4"
    if [[ "${old_value}" == "${new_value}" ]]; then
        return
    fi
    sed -i'~' "s/set ${variable}=[0-9]*/set ${variable}=${new_value}/" "${file}"
    rm "${file}~"
}

update_version_number build.bat VERSION_MAJOR $old_major $new_major
update_version_number build.bat VERSION_MINOR $old_minor $new_minor
update_version_number build.bat VERSION_PATCH $old_patch $new_patch
update_version_number xbuild.bat VERSION_MAJOR $old_major $new_major
update_version_number xbuild.bat VERSION_MINOR $old_minor $new_minor
update_version_number xbuild.bat VERSION_PATCH $old_patch $new_patch

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

update_version_string() {
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

# prerequisite:
# cargo install clog-cli
update_changelog() {
    local version="$1"
    clog --from-latest-tag \
         --changelog CHANGELOG.md \
         --repository https://github.com/rime/weasel \
         --setversion "${version}"
}

update_version_string update/testing-appcast.xml
update_version_string update/appcast.xml
update_pub_date update/testing-appcast.xml
update_pub_date update/appcast.xml

# changelog manually, so disable this
#update_changelog "${new_version}"
#${VISUAL:-${EDITOR:-nano}} CHANGELOG.md
#match_line "## ${new_version} " CHANGELOG.md || (
#    echo >&2 "CHANGELOG.md has no changes for version ${new_version}."
#    exit 1
#)
#bash update/write-release-notes.sh

release_message="chore(release): ${new_version} :tada:"
release_tag="${new_version}"
git commit --all --message "${release_message}"
git tag --annotate "${release_tag}" --message "${release_message}"
