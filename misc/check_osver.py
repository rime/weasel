#!/usr/bin/env python
import sys
ret = 0
xp = "Microsoft Windows XP"
for line in sys.stdin:
    if line.startswith(xp):
        ret = 5
        break
exit(ret)
