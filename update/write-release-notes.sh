#!/bin/bash

# writes an HTML page for changes in the latest release

cat <<EOF > release-notes.html
<!DOCTYPE HTML>
<html>
<head>
<meta charset="utf-8">
</head>
<body>
EOF

# take advantage of the fact that change log generator
# places 3 new lines after each chapter for a new release.
cat CHANGELOG.md | awk '{
  print;
  if ($0 == "") ++new_lines; else new_lines = 0;
  if (new_lines >= 3) exit;
}' | marked >> release-notes.html

cat <<EOF >> release-notes.html
</body>
</html>
EOF
