# QProcessInfo
Simple Qt class to enumerate running processes on a system

# Building
Should just be able to drop into any Qt project and go, it deliberately doesn't have any dependencies that shouldn't already be there (entirely Qt code on unix, only kernel32.dll on windows).

# TODO
Only tested briefly on Windows and Linux, enough for my purposes. Window titles are only present on windows and if xdotool is available on linux, on other systems they're always empty.

# License
2-clause BSD license. Go hog wild!
