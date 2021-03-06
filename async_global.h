/****************************************************************************
**
** Copyright (C) 2019 Ömer Göktaş
** Contact: omergoktas.com
**
** This file is part of the Async library.
**
** The Async is free software: you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public License
** version 3 as published by the Free Software Foundation.
**
** The Async is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with the Async. If not, see
** <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef ASYNC_GLOBAL_H
#define ASYNC_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(ASYNC_LIBRARY) // takes precedence when combined with others
#  define ASYNC_EXPORT Q_DECL_EXPORT
#elif defined(ASYNC_INCLUDE_STATIC)
#  define ASYNC_EXPORT
#else
#  define ASYNC_EXPORT Q_DECL_IMPORT
#endif

#endif // ASYNC_GLOBAL_H
