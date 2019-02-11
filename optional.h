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

#ifndef OPTIONAL_H
#define OPTIONAL_H

#include <async_global.h>

template<typename T>
class ASYNC_EXPORT Optional final {
public:
    Optional() : m_engaged(false) {}
    Optional(const T& value) { emplace(value); }
    Optional& operator=(const T& value) { emplace(value); }
    explicit operator bool() const { return m_engaged; }
    void reset() { m_engaged = false; }
    const T& value() const { return m_value; }
    void emplace(const T& value) { m_engaged(true); m_value(value); }
private:
    T m_value;
    bool m_engaged;
};

#endif // OPTIONAL_H
