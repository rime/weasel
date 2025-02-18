#!/usr/bin/env bash

show_help() {
  echo "-tag [new_tag]"
  echo "    new tag shall be in format of ^\\d+\\.\\d+\\.\\d+$ and greater than the current one or \$basetag"
  echo "-basetag [basetag name]"
  echo "    specify the base tag (which shall be in format of ^\\d+\\.\\d+\\.\\d+$), default to be the latest number format tag name"
  echo "-updatelog"
  echo "    update CHANGELOG.md if this switch is used"
  echo "-h"
  echo "    show help info"
}

get_old_version() {
  local filePath=$1
  local version
  version=$(grep -E 'VERSION_(MAJOR|MINOR|PATCH)=' "$filePath")
  declare -A version_map

  version_map[major]=$(echo "$version" | grep -oP 'VERSION_MAJOR=\K\d+')
  version_map[minor]=$(echo "$version" | grep -oP 'VERSION_MINOR=\K\d+')
  version_map[patch]=$(echo "$version" | grep -oP 'VERSION_PATCH=\K\d+')
  version_map[str]="${version_map[major]}.${version_map[minor]}.${version_map[patch]}"

  echo "${version_map[major]}"
  echo "${version_map[minor]}"
  echo "${version_map[patch]}"
  echo "${version_map[str]}"
}

parse_new_version() {
  local new_version=$1
  declare -A version

  if [[ $new_version =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
    version[major]=${BASH_REMATCH[1]}
    version[minor]=${BASH_REMATCH[2]}
    version[patch]=${BASH_REMATCH[3]}
  fi

  echo "${version[major]}"
  echo "${version[minor]}"
  echo "${version[patch]}"
}

update_changelog() {
  local new_tag=$1
  local old_tag=$2
  local commits
  commits=$(git log "$old_tag..HEAD" --pretty=format:"%s ([%an](https://github.com/rime/weasel/commit/%H))")

  declare -A groupedCommits
  declare -A groupedTitle
  groupedTitle=(
    [build]="Builds"
    [ci]="Continuous Integration"
    [fix]="Bug Fixes"
    [feat]="Features"
    [docs]="Documents"
    [style]="Code Style"
    [refactor]="Code Refactor"
    [test]="Tests"
    [chore]="Chores"
    [commit]="Commits"
  )

  # 启用不区分大小写的匹配
  shopt -s nocasematch

  while IFS= read -r commit; do
    if [[ $commit =~ ^(build|ci|fix|feat|docs|style|refactor|test|chore)(\([^\)]+\))?: ]]; then
      prefix=${BASH_REMATCH[1],,}  # 提取前缀并转小写
      groupedCommits[$prefix]+="$commit"$'\n'
    else
      groupedCommits["commit"]+="$commit"$'\n'
    fi
  done <<< "$commits"

  # 禁用不区分大小写的匹配
  shopt -u nocasematch

  local changelog=""
  for group in "${!groupedCommits[@]}"; do
    if [[ -n ${groupedCommits[$group]} ]]; then
      changelog+=$'\n'"#### ${groupedTitle[$group]}"$'\n'
      changelog+="${groupedCommits[$group]}"
    fi
  done

  local currentDateTime
  currentDateTime=$(date "+%Y-%m-%d")
  local fileContent
  fileContent=$(<CHANGELOG.md)
  local contentAdd
  contentAdd="<a name=\"$new_tag\"></a>"$'\n'
  contentAdd+="## [$new_tag](https://github.com/rime/weasel/compare/$old_tag...$new_tag)($currentDateTime)"$'\n'
  contentAdd+="$changelog"
  contentAdd=$"$contentAdd"$'\n'

  echo "$contentAdd"
  echo "$contentAdd$fileContent" > CHANGELOG.md
}

faild_exit() {
  echo 
  echo "Orz :( bump version failed!"
  echo "we will revert the modification by command git checkout . "
  echo 
  git checkout .
  exit 1
}

while [[ $# -gt 0 ]]; do
  case $1 in
    -tag)
      tag="$2"
      shift
      ;;
    -basetag)
      basetag="$2"
      shift
      ;;
    -updatelog)
      updatelog=true
      ;;
    -h)
      show_help
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
  shift
done

if ! [[ -f weasel.sln && -d .git ]]; then
  echo "Current directory is: $(pwd), it's not the weasel root directory"
  echo "please run 'update/bump_version.sh' with parameters under the weasel root in bash"
  exit 1
fi

if [[ ! $tag =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "Tag name does not match rule '^\\d+\\.\\d+\\.\\d+$'"
  echo "Please recheck it"
  exit 1
fi

old_version=($(get_old_version "build.bat"))
new_version=($(parse_new_version "$tag"))

if [[ -z "$basetag" ]]; then
  basetag=${old_version[3]}
fi

if [[ ! $basetag =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "Basetag format is not correct, it shall be in format of '^\\d+\\.\\d+\\.\\d+$'"
  echo "Please recheck it"
  exit 1
fi

if [[ "$tag" == "${old_version[3]}" ]]; then
  echo "The new tag: $tag is the same as the old one: ${old_version[3]}"
  exit 1
elif [[ "$tag" < "${old_version[3]}" ]]; then
  echo "$tag is older than ${old_version[3]}, please recheck your target version"
  exit 1
fi

echo "Bumping version to $tag ..."
currentDateTime=$(date --rfc-2822)

# update appcast xml files
sed -i "s|${old_version[3]}|$tag|g" ./update/appcast.xml || faild_exit
sed -i "s|<pubDate>[^<]*</pubDate>|<pubDate>$currentDateTime</pubDate>|g" ./update/appcast.xml || faild_exit
echo "update/appcast.xml updated"

sed -i "s|${old_version[3]}|$tag|g" ./update/testing-appcast.xml || faild_exit
sed -i "s|<pubDate>[^<]*</pubDate>|<pubDate>$currentDateTime</pubDate>|g" ./update/testing-appcast.xml || faild_exit
echo "update/testing-appcast.xml updated"

# update bat files
sed -i "s/VERSION_MAJOR=${old_version[0]}/VERSION_MAJOR=${new_version[0]}/g" ./build.bat || faild_exit
sed -i "s/VERSION_MINOR=${old_version[1]}/VERSION_MINOR=${new_version[1]}/g" ./build.bat || faild_exit
sed -i "s/VERSION_PATCH=${old_version[2]}/VERSION_PATCH=${new_version[2]}/g" ./build.bat || faild_exit
echo "build.bat updated"

sed -i "s/VERSION_MAJOR=${old_version[0]}/VERSION_MAJOR=${new_version[0]}/g" ./xbuild.bat || faild_exit
sed -i "s/VERSION_MINOR=${old_version[1]}/VERSION_MINOR=${new_version[1]}/g" ./xbuild.bat || faild_exit
sed -i "s/VERSION_PATCH=${old_version[2]}/VERSION_PATCH=${new_version[2]}/g" ./xbuild.bat || faild_exit
echo "xbuild.bat updated"

if [[ $updatelog ]]; then
  update_changelog "$tag" "$basetag" || faild_exit
  git add CHANGELOG.md
  echo "CHANGELOG.md updated"
fi

release_message="chore(release): $tag :tada:"
release_tag="$tag"
git add update/*.xml build.bat xbuild.bat
git commit -m "$release_message"
git tag -a "$release_tag" -m "$release_message"
