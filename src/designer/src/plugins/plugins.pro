TEMPLATE = subdirs
CONFIG += ordered

REQUIRES = !CONFIG(static,shared|static)
# contains(QT_CONFIG, opengl): SUBDIRS += tools/view3d
contains(QT_CONFIG, webkit): SUBDIRS += qwebview
contains(QT_CONFIG, phonon): SUBDIRS += phononwidgets
contains(QT_CONFIG, declarative): SUBDIRS += qdeclarativeview
