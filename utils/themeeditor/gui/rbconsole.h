/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: rbconsole.h 27775 2010-08-12 05:30:31Z bieber $
 *
 * Copyright (C) 2010 Robert Bieber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef RBCONSOLE_H
#define RBCONSOLE_H

#include <QWidget>

namespace Ui {
    class RBConsole;
}

class RBConsole : public QWidget {
    Q_OBJECT
public:
    RBConsole(QWidget *parent = 0);
    ~RBConsole();

    void addWarning(QString warning);

private:
    Ui::RBConsole *ui;
};

#endif // RBCONSOLE_H
