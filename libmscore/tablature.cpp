//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id: tablature.cpp 5568 2012-04-22 10:08:43Z wschweer $
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "tablature.h"
#include "chord.h"
#include "note.h"
#include "score.h"
#include "undo.h"

static int guitarStrings[6] = { 40, 45, 50, 55, 59, 64 };

Tablature guitarTablature(13, 6, guitarStrings);

//---------------------------------------------------------
//   Tablature
//---------------------------------------------------------

bool Tablature::bFretting = false;

Tablature::Tablature(int numFrets, int numStrings, int strings[])
      {
      _frets = numFrets;

      for (int i = 0; i < numStrings; i++)
            stringTable.append(strings[i]);
      }

Tablature::Tablature(int numFrets, QList<int>& strings)
      {
      _frets = numFrets;

      foreach(int i, strings)
            stringTable.append( i);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Tablature::read(const QDomElement& de)
      {

      for (QDomElement e = de.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            const QString& tag(e.tagName());
            if (tag == "frets")
                  _frets = e.text().toInt();
            else if (tag == "string")
                  stringTable.append(e.text().toInt());
            else
                  domError(e);
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Tablature::write(Xml& xml) const
      {
      xml.stag("Tablature");
      xml.tag("frets", _frets);
      foreach(int pitch, stringTable)
            xml.tag("string", pitch);
      xml.etag();
      }

//---------------------------------------------------------
//   convertPitch
//   Finds string and fret for a note.
//
//   Fills *string and *fret with suitable values for pitch
//   using the highest possible string.
//   If note cannot be fretted, uses fret 0 on nearest string and returns false
//
//    Note: Strings are stored internally from lowest (0) to highest (strings()-1),
//          but the returned *string value references strings in reversed, 'visual', order:
//          from highest (0) to lowest (strings()-1)
//---------------------------------------------------------

bool Tablature::convertPitch(int pitch, int* string, int* fret) const
      {
      int strings = stringTable.size();

      // if above max fret on highest string, fret on first string, but return failure
      if(pitch > stringTable.at(strings-1) + _frets) {
            *string = 0;
            *fret   = 0;
            return false;
            }

      // look for a suitable string, starting from the highest
      for (int i = strings-1; i >=0; i--) {
            if(pitch >= stringTable.at(i)) {
                  *string = strings - i - 1;
                  *fret   = pitch - stringTable.at(i);
                  return true;
                  }
            }

      // if no string found, pitch is below lowest string:
      // fret on last string, but return failure
      *string = strings-1;
      *fret   = 0;
      return false;
      }

//---------------------------------------------------------
//   getPitch
//   Returns the pitch corresponding to the string / fret combination
//---------------------------------------------------------

int Tablature::getPitch(int string, int fret) const
      {
      int strings = stringTable.size();
      return stringTable[strings - string - 1] + fret;
      }

//---------------------------------------------------------
//   fret
//    Returns the fret corresponding to the pitch / string combination
//    returns -1 if not possible
//---------------------------------------------------------

int Tablature::fret(int pitch, int string) const
      {
      int strings = stringTable.size();

      if (string < 0 || string >= strings)
            return FRET_NONE;
      int fret = pitch - stringTable[strings - string - 1];
      if (fret < 0 || fret >= _frets)
            return FRET_NONE;
      return fret;
      }

//---------------------------------------------------------
//   fretChord
//    Assigns fretting to all the notes of the chord,
//    re-using existing fretting wherever possible
//
//    Minimizes fret conflicts (multiple notes on the same string)
//    but marks as fretConflict notes which cannot be fretted
//    (outside tablature range) or which cannot be assigned
//    a separate string
//---------------------------------------------------------

void Tablature::fretChord(Chord * chord) const
      {
      int   nFret, nNewFret, nTempFret;
      int   nString, nNewString, nTempString;

      if(bFretting)
            return;
      bFretting = true;

      // we need to keep track of each string we allocate ourselves within this algorithm
      bool bUsed[strings()];                    // initially all strings are available
      for(nString=0; nString<strings(); nString++)
            bUsed[nString] = false;
      // we also need the notes sorted in order of string (from highest to lowest) and then pitch
      QMap<int, Note *> sortedNotes;
      int   idx = 0;
      int   key;
      foreach(Note * note, chord->notes()) {
            key = note->string()*100000;
            if(key < 0)                         // in case string is -1
                  key = -key;
            key -= note->pitch() * 100 + idx;  // disambiguate notes of equal pitch
            sortedNotes.insert(key, note);
            idx++;
            }

      // scan chord notes from highest, matching with strings from the highest
      foreach(Note * note, sortedNotes) {
            nString     = nNewString    = note->string();
            nFret       = nNewFret      = note->fret();
            note->setFretConflict(false);       // assume no conflicts on this note
            // if no fretting yet or current fretting is no longer valid
            if (nString == STRING_NONE || nFret == FRET_NONE || getPitch(nString, nFret) != note->pitch()) {
                  // get a new fretting
                  if(!convertPitch(note->pitch(), &nNewString, &nNewFret) ) {
                        // no way to fit this note in this tab:
                        // mark as fretting conflict
                        note->setFretConflict(true);
                        // store fretting change without affecting chord context
                        if (nFret != nNewFret)
                              note->score()->undoChangeProperty(note, P_FRET, nNewFret);
                        if (nString != nNewString)
                              note->score()->undoChangeProperty(note, P_STRING, nNewString);
                        continue;
                        }

                  // check this note is not using the same string of another note of this chord
                  foreach(Note * note2, sortedNotes) {
                        // if same string...
                        if(note2 != note && note2->string() == nNewString) {
                              // ...attempt to fret this note on its old string
                              if( (nTempFret=fret(note->pitch(), nString)) != FRET_NONE) {
                                    nNewFret   = nTempFret;
                                    nNewString = nString;
                                    }
                              break;
                              }
                        }
                  }

            // check we are not reusing a string we already used
            if(bUsed[nNewString]) {
                  // ...try with each other string, from the highest
                  for(nTempString=0; nTempString < strings(); nTempString++) {
                        if(bUsed[nTempString])
                              continue;
                        if( (nTempFret=fret(note->pitch(), nTempString)) != FRET_NONE) {
                              // suitable string found
                              nNewFret    = nTempFret;
                              nNewString  = nTempString;
                              break;
                              }
                        }
                  // if we run out of strings
                  if(nTempString >= strings()) {
                        // no way to fit this chord in this tab:
                        // mark as fretting conflict this note...
                        note->setFretConflict(true);
//                        // and any note already scanned and set on the same string
//                        for(nCount2=chord->notes().size()-1; nCount2 > nCount; nCount2--) {
//                              note2 = chord->notes().at(nCount2);
//                              if(note2->string() == nNewString)
//                                    note2->setFretConflict(true);
//                              }
                        continue;
                        }
                  }

            // if fretting did change, store as a fret change
            if (nFret != nNewFret)
                  note->score()->undoChangeProperty(note, P_FRET, nNewFret);
            if (nString != nNewString)
                  note->score()->undoChangeProperty(note, P_STRING, nNewString);

            bUsed[nNewString] = true;           // string is used
            }
      bFretting = false;
      }

//---------------------------------------------------------
//   MusicXMLStepAltOct2Pitch
//---------------------------------------------------------

/**
 Convert MusicXML \a step / \a alter / \a octave to midi pitch.
 Note: similar to (part of) xmlSetPitch in mscore/importxml.cpp.
 TODO: combine ?
 */

static int MusicXMLStepAltOct2Pitch(char step, int alter, int octave)
      {
      int istep = step - 'A';
      //                       a  b   c  d  e  f  g
      static int table[7]  = { 9, 11, 0, 2, 4, 5, 7 };
      if (istep < 0 || istep > 6) {
            qDebug("MusicXMLStepAltOct2Pitch: illegal step %d, <%c>", istep, step);
            return -1;
            }
      int pitch = table[istep] + alter + (octave+1) * 12;

      if (pitch < 0)
            pitch = -1;
      if (pitch > 127)
            pitch = -1;

      return pitch;
      }

//---------------------------------------------------------
//   Read MusicXML
//
// Set the FretDiagram state based on the MusicXML <figure> node de.
//---------------------------------------------------------

void Tablature::readMusicXML(const QDomElement& de)
      {
      qDebug("Tablature::readMusicXML");
      _frets = 25;

      for (QDomElement e = de.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            const QString& tag(e.tagName());
            int val = e.text().toInt();
            if (tag == "staff-lines") {
                  if (val > 0) {
                        // resize the string table and init with zeroes
                        stringTable = QVector<int>(val).toList();
                        for (int i = 0; i < stringTable.size(); ++i)
                              qDebug("Tablature::readMusicXML stringTable[%d] = %d", i, stringTable.at(i));
                        }
                  else
                        qDebug("Tablature::readMusicXML: illegal staff-lines %d", val);
                  }
            else if (tag == "staff-tuning") {
                  int     line   = e.attribute("line").toInt();
                  QString step;
                  int     alter  = 0;
                  int     octave = 0;
                  for (QDomElement ee = e.firstChildElement(); !ee.isNull(); ee = ee.nextSiblingElement()) {
                        const QString& tag(ee.tagName());
                        int val = ee.text().toInt();
                        if (tag == "tuning-alter")
                              alter = val;
                        else if (tag == "tuning-octave")
                              octave = val;
                        else if (tag == "tuning-step")
                              step = ee.text();
                        else
                              domError(ee);
                        }
                  qDebug("Tablature::readMusicXML string %d step/alter/oct %s/%d/%d",
                         line, qPrintable(step), alter, octave);
                  if (0 < line && line <= stringTable.size()) {
                        int pitch = MusicXMLStepAltOct2Pitch(step[0].toLatin1(), alter, octave);
                        if (pitch >= 0)
                              stringTable[line - 1] = pitch;
                        else
                              qDebug("Tablature::readMusicXML invalid string %d tuning step/alter/oct %s/%d/%d",
                                     line, qPrintable(step), alter, octave);
                        }
                  }
            else if (tag == "capo") {
                  ; // not supported: silently ignored
                  }
            else {
                  ; // others silently ignored
                  }
            }
      for (int i = 0; i < stringTable.size(); ++i)
            qDebug("Tablature::readMusicXML stringTable[%d] = %d", i, stringTable.at(i));
      }

//---------------------------------------------------------
//   Write MusicXML
//---------------------------------------------------------

void Tablature::writeMusicXML(Xml& /*xml*/) const
      {
      }
