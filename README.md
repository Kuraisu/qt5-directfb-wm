# qt5-directfb-wm
Simple Window Manager for Qt5 DirectFB backend.

# Rationale
In Qt4 DirectFB backend had access to simple Window Manager which was part of Qt Embedded platform. Qt5 platform was redesigned from ground up and there were no place for that Window Manager in new platform. Qt5 applications in DirectFB were lacking window decorations and all other WM capabilities.

Unfortunately in Qt5 platform such simple WM is out of place, so here it's implemented directly in DirectFB backend and could be considered a hack.

# Patch
To see WM as a patch, check this commit: [b5a31627](/../../commit/b5a3162722d21726923d8531b323be078e012d51)
