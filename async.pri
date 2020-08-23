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

CONFIG      += c++14
DEFINES     += ASYNC_INCLUDE_STATIC
INCLUDEPATH += $$PWD

SOURCES     += $$PWD/async.cpp
HEADERS     += $$PWD/async.h \
               $$PWD/optional.h \
               $$PWD/async_global.h
