//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id: tempotext.h -1   $
//
//  Copyright (C) 2002-2009 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __TEMPOTEXTPROPERTIES_H__
#define __TEMPOTEXTPROPERTIES_H__

#include "libmscore/text.h"
#include "ui_tempoproperties.h"

class TempoText;

//---------------------------------------------------------
//   TempoProperties
//    Dialog
//---------------------------------------------------------

class TempoProperties : public QDialog, public Ui::TempoProperties {
      Q_OBJECT

      TempoText* tempoText;

   private slots:
      void saveValues();

   public:
      TempoProperties(TempoText*, QWidget* parent = 0);
      };

#endif
