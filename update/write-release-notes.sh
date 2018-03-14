#!/bin/bash

# writes an HTML page for changes inr the latest release.
# take advantage of the fact that change log generator
# places 3 new lines after each chapter for a new release.

cat CHANGELOG.md | awk '{
  print;
  if ($0 == "") ++new_lines; else new_lines = 0;
  if (new_lines >= 3) exit;
}' | marked --gfm > release-notes.html
