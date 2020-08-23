##**************************************************************************
##
## Copyright (C) 2019 Ömer Göktaş
## Contact: omergoktas.com
##
## This file is part of the Async library.
##
## The Async is free software: you can redistribute it and/or
## modify it under the terms of the GNU Lesser General Public License
## version 3 as published by the Free Software Foundation.
##
## The Async is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public
## License along with the Async. If not, see
## <https://www.gnu.org/licenses/>.
##
##**************************************************************************

QT -= gui
TEMPLATE = lib
TARGET = async
CONFIG += shared strict_c strict_c++ utf8_source hide_symbols
gcc:QMAKE_CXXFLAGS += -pedantic-errors
msvc:QMAKE_CXXFLAGS += -permissive-
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000
DEFINES += ASYNC_LIBRARY

OTHER_FILES += $$PWD/TODO \
               $$PWD/LICENSE

unix {
    target.path = /usr/lib
    INSTALLS += target
}

include(async.pri)