/****************************************************************************
**
** Copyright (C) 2018 Pelagicore AG
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtIVI module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-QTAS$
** Commercial License Usage
** Licensees holding valid commercial Qt Automotive Suite licenses may use
** this file in accordance with the commercial license agreement provided
** with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and The Qt Company.  For
** licensing terms and conditions see https://www.qt.io/terms-conditions.
** For further information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
** SPDX-License-Identifier: LGPL-3.0
**
****************************************************************************/

#ifndef QIVIMEDIADEVICEDISCOVERYMODELBACKENDINTERFACE_H
#define QIVIMEDIADEVICEDISCOVERYMODELBACKENDINTERFACE_H

#include <QtIviCore/QIviFeatureInterface>
#include <QtIviCore/QIviServiceObject>
#include <QtIviMedia/qtivimediaglobal.h>

QT_BEGIN_NAMESPACE

class Q_QTIVIMEDIA_EXPORT QIviMediaDeviceDiscoveryModelBackendInterface : public QIviFeatureInterface
{
    Q_OBJECT
public:
    explicit QIviMediaDeviceDiscoveryModelBackendInterface(QObject *parent = nullptr);

    virtual void initialize() = 0;

Q_SIGNALS:

    //The ownership will be transferred to discoveryModel. This will reset the model with the new content, using add/remove is recommended
    void availableDevices(const QList<QIviServiceObject*> devices);
    void deviceAdded(QIviServiceObject *device);
    void deviceRemoved(QIviServiceObject *device);
};

#define QIviMediaDeviceDiscovery_iid "org.qt-project.qtivi.MediaDiscovery/1.0"

QT_END_NAMESPACE

#endif // QIVIMEDIADEVICEDISCOVERYMODELBACKENDINTERFACE_H
