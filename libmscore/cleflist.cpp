//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id:$
//
//  Copyright (C) 2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "cleflist.h"
#include "clef.h"
#include "score.h"

//---------------------------------------------------------
//   ClefTypeList::operator==
//---------------------------------------------------------

bool ClefTypeList::operator==(const ClefTypeList& t) const
      {
      return t._concertClef == _concertClef && t._transposingClef == _transposingClef;
      }

//---------------------------------------------------------
//   ClefTypeList::operator!=
//---------------------------------------------------------

bool ClefTypeList::operator!=(const ClefTypeList& t) const
      {
      return t._concertClef != _concertClef || t._transposingClef != _transposingClef;
      }

//---------------------------------------------------------
//   clef
//---------------------------------------------------------

ClefTypeList ClefList::clef(int tick) const
      {
      if (empty())
            return ClefTypeList(CLEF_G, CLEF_G);
      ciClefEvent i = upperBound(tick);
      if (i == begin())
            return ClefTypeList(CLEF_G, CLEF_G);
      --i;
      return i.value();
      }

//---------------------------------------------------------
//   setClef
//---------------------------------------------------------

void ClefList::setClef(int tick, ClefTypeList idx)
      {
qDebug("setClef...\n");
      replace(tick, idx);
      }

//---------------------------------------------------------
//   ClefList::read
//---------------------------------------------------------

void ClefList::read(const QDomElement& de, Score* cs)
      {
      for (QDomElement e = de.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            if (e.tagName() == "clef") {
                  int tick    = e.attribute("tick", "0").toInt();
                  ClefType ct = Clef::clefType(e.attribute("idx", "0"));
                  insert(cs->fileDivision(tick), ClefTypeList(ct, ct));
                  }
            else
                  domError(e);
            }
      }

