/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>


template<typename T> class QList;

class AbstractRenderView;
class DataObject;
class RendererImplementation;


class RendererImplementationSwitch
{
public:
    explicit RendererImplementationSwitch(AbstractRenderView & renderView);
    ~RendererImplementationSwitch();

    void findSuitableImplementation(const QList<DataObject *> & dataObjects);
    RendererImplementation & currentImplementation();

private:
    AbstractRenderView & m_view;
    std::unique_ptr<RendererImplementation> m_currentImpl;
    std::unique_ptr<RendererImplementation> m_nullImpl;

private:
    RendererImplementationSwitch(const RendererImplementationSwitch &) = delete;
    void operator=(const RendererImplementationSwitch &) = delete;
};
