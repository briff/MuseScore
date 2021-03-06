//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id: instrument.cpp 5149 2011-12-29 08:38:43Z wschweer $
//
//  Copyright (C) 2008-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "instrument_p.h"
#include "xml.h"
#include "drumset.h"
#include "articulation.h"
#include "utils.h"
#include "tablature.h"
#include "instrtemplate.h"

Instrument InstrumentList::defaultInstrument;

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void NamedEventList::write(Xml& xml, const QString& n) const
      {
      xml.stag(QString("%1 name=\"%2\"").arg(n).arg(name));
      if (!descr.isEmpty())
            xml.tag("descr", descr);
      foreach(const Event& e, events)
            e.write(xml);
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void NamedEventList::read(const QDomElement& de)
      {
      name = de.attribute("name");
      for (QDomElement e = de.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            const QString& tag(e.tagName());
            if (tag == "program") {
                  Event ev(ME_CONTROLLER);
                  ev.setController(CTRL_PROGRAM);
                  ev.setValue(e.attribute("value", "0").toInt());
                  events.append(ev);
                  }
            else if (tag == "controller") {
                  Event ev(ME_CONTROLLER);
                  ev.setController(e.attribute("ctrl", "0").toInt());
                  ev.setValue(e.attribute("value", "0").toInt());
                  events.append(ev);
                  }
            else if (tag == "descr")
                  descr = e.text();
            else
                  domError(e);
            }
      }

//---------------------------------------------------------
//   operator
//---------------------------------------------------------

bool MidiArticulation::operator==(const MidiArticulation& i) const
      {
      return (i.name == name) && (i.velocity == velocity) && (i.gateTime == gateTime);
      }

//---------------------------------------------------------
//   Instrument
//---------------------------------------------------------

InstrumentData::InstrumentData()
      {
      Channel a;
      a.name  = "normal";
      _channel.append(a);

      _minPitchA   = 0;
      _maxPitchA   = 127;
      _minPitchP   = 0;
      _maxPitchP   = 127;
      _useDrumset  = false;
      _drumset     = 0;
      _tablature   = 0;
      }

InstrumentData::InstrumentData(const InstrumentData& i)
   : QSharedData(i)
      {
      _longNames    = i._longNames;
      _shortNames   = i._shortNames;
      _trackName    = i._trackName;
      _minPitchA    = i._minPitchA;
      _maxPitchA    = i._maxPitchA;
      _minPitchP    = i._minPitchP;
      _maxPitchP    = i._maxPitchP;
      _transpose    = i._transpose;
      _useDrumset   = i._useDrumset;
      _drumset      = 0;
      _tablature    = 0;
      setDrumset(i._drumset);
      setTablature(i._tablature);
      _midiActions  = i._midiActions;
      _articulation = i._articulation;
      _channel      = i._channel;
      }

//---------------------------------------------------------
//   ~InstrumentData
//---------------------------------------------------------

InstrumentData::~InstrumentData()
      {
      delete _tablature;
      delete _drumset;
      }

//---------------------------------------------------------
//   tablature
//    If instrument has no tablature, return default
//    (guitar) tablature
//---------------------------------------------------------

Tablature* InstrumentData::tablature() const
      {
      return _tablature ? _tablature : &guitarTablature;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void InstrumentData::write(Xml& xml) const
      {
      xml.stag("Instrument");
      foreach(const StaffNameDoc& doc, _longNames) {
            xml.stag(QString("longName pos=\"%1\"").arg(doc.pos));
            xml.writeHtml(doc.name.toHtml());
            xml.etag();
            }
      foreach(const StaffNameDoc& doc, _shortNames) {
            xml.stag(QString("shortName pos=\"%1\"").arg(doc.pos));
            xml.writeHtml(doc.name.toHtml());
            xml.etag();
            }
      xml.tag("trackName", _trackName);
      if (_minPitchP > 0)
            xml.tag("minPitchP", _minPitchP);
      if (_maxPitchP < 127)
            xml.tag("maxPitchP", _maxPitchP);
      if (_minPitchA > 0)
            xml.tag("minPitchA", _minPitchA);
      if (_maxPitchA < 127)
            xml.tag("maxPitchA", _maxPitchA);
      if (_transpose.diatonic)
            xml.tag("transposeDiatonic", _transpose.diatonic);
      if (_transpose.chromatic)
            xml.tag("transposeChromatic", _transpose.chromatic);
      if (_useDrumset) {
            xml.tag("useDrumset", _useDrumset);
            _drumset->save(xml);
            }
      if (_tablature)
            _tablature->write(xml);
      foreach(const NamedEventList& a, _midiActions)
            a.write(xml, "MidiAction");
      foreach(const MidiArticulation& a, _articulation)
            a.write(xml);
      foreach(const Channel& a, _channel)
            a.write(xml);
      xml.etag();
      }

//---------------------------------------------------------
//   InstrumentData::read
//---------------------------------------------------------

void InstrumentData::read(const QDomElement& de)
      {
      int program = -1;
      int bank    = 0;
      int chorus  = 30;
      int reverb  = 30;
      int volume  = 100;
      int pan     = 60;
      bool customDrumset = false;

      _channel.clear();
      for (QDomElement e = de.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            const QString& tag(e.tagName());
            const QString& val(e.text());

            if (tag == "longName") {
                  int pos = e.attribute("pos", "0").toInt();
                  QTextDocumentFragment longName = QTextDocumentFragment::fromHtml(Xml::htmlToString(e));
                  _longNames.append(StaffNameDoc(longName, pos));
                  }
            else if (tag == "shortName") {
                  int pos = e.attribute("pos", "0").toInt();
                  QTextDocumentFragment shortName = QTextDocumentFragment::fromHtml(Xml::htmlToString(e));
                  _shortNames.append(StaffNameDoc(shortName, pos));
                  }
            else if (tag == "trackName")
                  _trackName = val;
            else if (tag == "minPitch") {      // obsolete
                  _minPitchP = _minPitchA = val.toInt();
                  }
            else if (tag == "maxPitch") {       // obsolete
                  _maxPitchP = _maxPitchA = val.toInt();
                  }
            else if (tag == "minPitchA")
                  _minPitchA = val.toInt();
            else if (tag == "minPitchP")
                  _minPitchP = val.toInt();
            else if (tag == "maxPitchA")
                  _maxPitchA = val.toInt();
            else if (tag == "maxPitchP")
                  _maxPitchP = val.toInt();
            else if (tag == "transposition") {    // obsolete
                  _transpose.chromatic = val.toInt();
                  _transpose.diatonic = chromatic2diatonic(val.toInt());
                  }
            else if (tag == "transposeChromatic")
                  _transpose.chromatic = val.toInt();
            else if (tag == "transposeDiatonic")
                  _transpose.diatonic = val.toInt();
            else if (tag == "useDrumset") {
                  _useDrumset = val.toInt();
                  if (_useDrumset)
                        _drumset = new Drumset(*smDrumset);
                  }
            else if (tag == "Drum") {
                  // if we see on of this tags, a custom drumset will
                  // be created
                  if (_drumset == 0)
                        _drumset = new Drumset(*smDrumset);
                  if (!customDrumset) {
                        _drumset->clear();
                        customDrumset = true;
                        }
                  _drumset->load(e);
                  }
            else if (tag == "Tablature") {
                  _tablature = new Tablature();
                  _tablature->read(e);
                  }
            else if (tag == "MidiAction") {
                  NamedEventList a;
                  a.read(e);
                  _midiActions.append(a);
                  }
            else if (tag == "Articulation") {
                  MidiArticulation a;
                  a.read(e);
                  _articulation.append(a);
                  }
            else if (tag == "Channel" || tag == "channel") {
                  Channel a;
                  a.read(e);
                  _channel.append(a);
                  }
            else if (tag == "chorus")     // obsolete
                  chorus = val.toInt();
            else if (tag == "reverb")     // obsolete
                  reverb = val.toInt();
            else if (tag == "midiProgram")  // obsolete
                  program = val.toInt();
            else if (tag == "volume")     // obsolete
                  volume = val.toInt();
            else if (tag == "pan")        // obsolete
                  pan = val.toInt();
            else if (tag == "midiChannel")      // obsolete
                  ;
            else
                  domError(e);
            }
      if (_channel.isEmpty()) {      // for backward compatibility
            Channel a;
            a.chorus  = chorus;
            a.reverb  = reverb;
            a.name    = "normal";
            a.program = program;
            a.bank    = bank;
            a.volume  = volume;
            a.pan     = pan;
            _channel.append(a);
            }
      if (_useDrumset) {
            if (_channel[0].bank == 0)
                  _channel[0].bank = 128;
            _channel[0].updateInitList();
            }
      }

//---------------------------------------------------------
//   action
//---------------------------------------------------------

NamedEventList* InstrumentData::midiAction(const QString& s, int channelIdx) const
      {
      // first look in channel list

      foreach(const NamedEventList& a, _channel[channelIdx].midiActions) {
            if (s == a.name)
                  return const_cast<NamedEventList*>(&a);
            }

      foreach(const NamedEventList& a, _midiActions) {
            if (s == a.name)
                  return const_cast<NamedEventList*>(&a);
            }
      return 0;
      }

//---------------------------------------------------------
//   Channel
//---------------------------------------------------------

Channel::Channel()
      {
      for(int i = 0; i < A_INIT_COUNT; ++i)
            init.append(0);
      synti    = 0;     // -1;
      channel  = -1;
      program  = -1;
      bank     = 0;
      volume   = 100;
      pan      = 64;
      chorus   = 0;
      reverb   = 0;

      mute     = false;
      solo     = false;
      soloMute = false;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Channel::write(Xml& xml) const
      {
      if (name.isEmpty())
            xml.stag("Channel");
      else
            xml.stag(QString("Channel name=\"%1\"").arg(name));
      if (!descr.isEmpty())
            xml.tag("descr", descr);
      updateInitList();
      foreach(const Event& e, init) {
            if (e.type() == ME_INVALID)
                  continue;
            if (e.type() == ME_CONTROLLER) {
                  if (e.controller() == CTRL_HBANK && e.value() == 0)
                        continue;
                  if (e.controller() == CTRL_LBANK && e.value() == 0)
                        continue;
                  if (e.controller() == CTRL_VOLUME && e.value() == 100)
                        continue;
                  if (e.controller() == CTRL_PANPOT && e.value() == 64)
                        continue;
                  if (e.controller() == CTRL_REVERB_SEND && e.value() == 0)
                        continue;
                  if (e.controller() == CTRL_CHORUS_SEND && e.value() == 0)
                        continue;
                  }

            e.write(xml);
            }
      if (synti)                    // HACK
            xml.tag("synti", "Aeolus");
      if (mute)
            xml.tag("mute", mute);
      if (solo)
            xml.tag("solo", solo);
      foreach(const NamedEventList& a, midiActions)
            a.write(xml, "MidiAction");
      foreach(const MidiArticulation& a, articulation)
            a.write(xml);
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Channel::read(const QDomElement& de)
      {
      synti = 0;
      name = de.attribute("name");
      for (QDomElement e = de.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            const QString& tag(e.tagName());
            const QString& val(e.text());
            if (tag == "program") {
                  program = e.attribute("value", "-1").toInt();
                  if (program == -1)
                        program = val.toInt();
                  }
            else if (tag == "controller") {
                  int value = e.attribute("value", 0).toInt();
                  int ctrl  = e.attribute("ctrl", 0).toInt();
                  switch (ctrl) {
                        case CTRL_HBANK:
                              bank = (value << 7) + (bank & 0x7f);
                              break;
                        case CTRL_LBANK:
                              bank = (bank & ~0x7f) + (value & 0x7f);
                              break;
                        case CTRL_VOLUME:
                              volume = value;
                              break;
                        case CTRL_PANPOT:
                              pan = value;
                              break;
                        case CTRL_CHORUS_SEND:
                              chorus = value;
                              break;
                        case CTRL_REVERB_SEND:
                              reverb = value;
                              break;
                        default:
                              {
                              Event e(ME_CONTROLLER);
                              e.setOntime(-1);
                              e.setChannel(0);
                              e.setController(ctrl);
                              e.setValue(value);
                              init.append(e);
                              }
                              break;
                        }
                  }
            else if (tag == "Articulation") {
                  MidiArticulation a;
                  a.read(e);
                  articulation.append(a);
                  }
            else if (tag == "MidiAction") {
                  NamedEventList a;
                  a.read(e);
                  midiActions.append(a);
                  }
            else if (tag == "synti") {
                  if (val == "Aeolus")
                        synti = 1;
                  else
                        synti = 0;
                  }
            else if (tag == "descr") {
                  descr = e.text();
                  }
            else if (tag == "mute") {
                  mute = val.toInt();
                  }
            else if (tag == "solo") {
                  solo = val.toInt();
                  }
            else
                  domError(e);
            }
      updateInitList();
      }

//---------------------------------------------------------
//   updateInitList
//---------------------------------------------------------

void Channel::updateInitList() const
      {
      for (int i = 0; i < A_INIT_COUNT; ++i) {
            // delete init[i];      memory leak
            init[i] = 0;
            }
      Event e;
      if (program != -1) {
            e.setType(ME_CONTROLLER);
            e.setController(CTRL_PROGRAM);
            e.setValue(program);
            init[A_PROGRAM] = e;
            }

      e.setType(ME_CONTROLLER);
      e.setController(CTRL_HBANK);
      e.setValue((bank >> 7) & 0x7f);
      init[A_HBANK] = e;

      e.setType(ME_CONTROLLER);
      e.setController(CTRL_LBANK);
      e.setValue(bank & 0x7f);
      init[A_LBANK] = e;

      e.setType(ME_CONTROLLER);
      e.setController(CTRL_VOLUME);
      e.setValue(volume);
      init[A_VOLUME] = e;

      e.setType(ME_CONTROLLER);
      e.setController(CTRL_PANPOT);
      e.setValue(pan);
      init[A_PAN] = e;

      e.setType(ME_CONTROLLER);
      e.setController(CTRL_CHORUS_SEND);
      e.setValue(chorus);
      init[A_CHORUS] = e;

      e.setType(ME_CONTROLLER);
      e.setController(CTRL_REVERB_SEND);
      e.setValue(reverb);
      init[A_REVERB] = e;
      }

//---------------------------------------------------------
//   channelIdx
//---------------------------------------------------------

int InstrumentData::channelIdx(const QString& s) const
      {
      int idx = 0;
      foreach(const Channel& a, _channel) {
            if (a.name.isEmpty() && s == "normal")
                  return idx;
            if (s == a.name)
                  return idx;
            ++idx;
            }
      return -1;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MidiArticulation::write(Xml& xml) const
      {
      if (name.isEmpty())
            xml.stag("Articulation");
      else
            xml.stag(QString("Articulation name=\"%1\"").arg(name));
      if (!descr.isEmpty())
            xml.tag("descr", descr);
      xml.tag("velocity", velocity);
      xml.tag("gateTime", gateTime);
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MidiArticulation::read(const QDomElement& de)
      {
      name = de.attribute("name");
      for (QDomElement e = de.firstChildElement(); !e.isNull(); e = e.nextSiblingElement()) {
            const QString& tag(e.tagName());
            QString text(e.text());
            if (tag == "velocity") {
                  if (text.endsWith("%"))
                        text = text.left(text.size()-1);
                  velocity = text.toInt();
                  }
            else if (tag == "gateTime") {
                  if (text.endsWith("%"))
                        text = text.left(text.size()-1);
                  gateTime = text.toInt();
                  }
            else if (tag == "descr")
                  descr = e.text();
            else
                  domError(e);
            }
      }

//---------------------------------------------------------
//   updateVelocity
//---------------------------------------------------------

void InstrumentData::updateVelocity(int* velocity, int /*channelIdx*/, const QString& name)
      {
//      const Channel& c = _channel[channelIdx];
//      foreach(const MidiArticulation& a, c.articulation) {
      foreach(const MidiArticulation& a, _articulation) {
            if (a.name == name) {
                  *velocity = *velocity * a.velocity / 100;
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   updateGateTime
//---------------------------------------------------------

void InstrumentData::updateGateTime(int* gateTime, int /*channelIdx*/, const QString& name)
      {
//      const Channel& c = _channel[channelIdx];
//      foreach(const MidiArticulation& a, c.articulation) {

      foreach(const MidiArticulation& a, _articulation) {
            if (a.name == name) {
                  *gateTime = *gateTime * a.gateTime / 100;
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   operator==
//---------------------------------------------------------

bool InstrumentData::operator==(const InstrumentData& i) const
      {
      int n = _longNames.size();
      if (i._longNames.size() != n)
            return false;
      for (int k = 0; k < n; ++k) {
            if (!(i._longNames[k] == _longNames[k]))
                  return false;
            }
      n = _shortNames.size();
      if (i._shortNames.size() != n)
            return false;
      for (int k = 0; k < n; ++k) {
            if (!(i._shortNames[k] == _shortNames[k].name))
                  return false;
            }
      return i._minPitchA == _minPitchA
         &&  i._maxPitchA == _maxPitchA
         &&  i._minPitchP == _minPitchP
         &&  i._maxPitchP == _maxPitchP
         &&  i._useDrumset == _useDrumset
         &&  i._midiActions == _midiActions
         &&  i._channel == _channel
         &&  i._articulation == _articulation
         &&  i._transpose.diatonic == _transpose.diatonic
         &&  i._transpose.chromatic == _transpose.chromatic
         &&  i._trackName == _trackName
         &&  i.tablature() == tablature();
         ;
      }

//---------------------------------------------------------
//   operator==
//---------------------------------------------------------

bool StaffNameDoc::operator==(const StaffNameDoc& i) const
      {
      return (i.pos == pos) && (i.name.toHtml() == name.toHtml());
      }

//---------------------------------------------------------
//   setUseDrumset
//---------------------------------------------------------

void InstrumentData::setUseDrumset(bool val)
      {
      _useDrumset = val;
      if (val && _drumset == 0) {
            _drumset = new Drumset(*smDrumset);
            }
      }

//---------------------------------------------------------
//   setDrumset
//---------------------------------------------------------

void InstrumentData::setDrumset(Drumset* ds)
      {
      delete _drumset;
      if (ds)
            _drumset = new Drumset(*ds);
      else
            _drumset = 0;
      }

//---------------------------------------------------------
//   setTablature
//---------------------------------------------------------

void InstrumentData::setTablature(Tablature* t)
      {
      delete _tablature;
      if (t)
            _tablature = new Tablature(*t);
      else
            _tablature = 0;
      }

//---------------------------------------------------------
//   setLongName
//---------------------------------------------------------

void InstrumentData::setLongName(const QTextDocumentFragment& f)
      {
      _longNames.clear();
      _longNames.append(StaffNameDoc(f, 0));
      }

//---------------------------------------------------------
//   setShortName
//---------------------------------------------------------

void InstrumentData::setShortName(const QTextDocumentFragment& f)
      {
      _shortNames.clear();
      _shortNames.append(StaffNameDoc(f, 0));
      }

//---------------------------------------------------------
//   addLongName
//---------------------------------------------------------

void InstrumentData::addLongName(const StaffNameDoc& f)
      {
      _longNames.append(f);
      }

//---------------------------------------------------------
//   addShortName
//---------------------------------------------------------

void InstrumentData::addShortName(const StaffNameDoc& f)
      {
      _shortNames.append(f);
      }

//---------------------------------------------------------
//   Instrument
//---------------------------------------------------------

Instrument::Instrument()
      {
      d = new InstrumentData;
      }

Instrument::Instrument(const Instrument& s)
   : d(s.d)
      {
      }

Instrument::~Instrument()
      {
      }

//---------------------------------------------------------
//   operator=
//---------------------------------------------------------

Instrument& Instrument::operator=(const Instrument& s)
      {
      d = s.d;
      return *this;
      }

//---------------------------------------------------------
//   operator==
//---------------------------------------------------------

bool Instrument::operator==(const Instrument& s) const
      {
      return d->operator==(*s.d);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Instrument::read(const QDomElement& e)
      {
      d->read(e);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Instrument::write(Xml& xml) const
      {
      d->write(xml);
      }

//---------------------------------------------------------
//   midiAction
//---------------------------------------------------------

NamedEventList* Instrument::midiAction(const QString& s, int channel) const
      {
      return d->midiAction(s, channel);
      }

//---------------------------------------------------------
//   channelIdx
//---------------------------------------------------------

int Instrument::channelIdx(const QString& s) const
      {
      return d->channelIdx(s);
      }

//---------------------------------------------------------
//   updateVelocity
//---------------------------------------------------------

void Instrument::updateVelocity(int* velocity, int channel, const QString& name)
      {
      d->updateVelocity(velocity, channel, name);
      }

//---------------------------------------------------------
//   updateGateTime
//---------------------------------------------------------

void Instrument::updateGateTime(int* gateTime, int channel, const QString& name)
      {
      d->updateGateTime(gateTime, channel, name);
      }

//---------------------------------------------------------
//   minPitchP
//---------------------------------------------------------

int Instrument::minPitchP() const
      {
      return d->_minPitchP;
      }

//---------------------------------------------------------
//   maxPitchP
//---------------------------------------------------------

int Instrument::maxPitchP() const
      {
      return d->_maxPitchP;
      }

//---------------------------------------------------------
//   minPitchA
//---------------------------------------------------------

int Instrument::minPitchA() const
      {
      return d->_minPitchA;
      }

//---------------------------------------------------------
//   maxPitchA
//---------------------------------------------------------

int Instrument::maxPitchA() const
      {
      return d->_maxPitchA;
      }

//---------------------------------------------------------
//   setMinPitchP
//---------------------------------------------------------

void Instrument::setMinPitchP(int v)
      {
      d->setMinPitchP(v);
      }

//---------------------------------------------------------
//   setMaxPitchP
//---------------------------------------------------------

void Instrument::setMaxPitchP(int v)
      {
      d->setMaxPitchP(v);
      }

//---------------------------------------------------------
//   setMinPitchA
//---------------------------------------------------------

void Instrument::setMinPitchA(int v)
      {
      d->setMinPitchA(v);
      }

//---------------------------------------------------------
//   setMaxPitchA
//---------------------------------------------------------

void Instrument::setMaxPitchA(int v)
      {
      d->setMaxPitchA(v);
      }

//---------------------------------------------------------
//   transpose
//---------------------------------------------------------

Interval Instrument::transpose() const
      {
      return d->transpose();
      }

//---------------------------------------------------------
//   setTranspose
//---------------------------------------------------------

void Instrument::setTranspose(const Interval& v)
      {
      d->setTranspose(v);
      }

//---------------------------------------------------------
//   setDrumset
//---------------------------------------------------------

void Instrument::setDrumset(Drumset* ds)
      {
      d->setDrumset(ds);
      }

//---------------------------------------------------------
//   drumset
//---------------------------------------------------------

Drumset* Instrument::drumset() const
      {
      return d->drumset();
      }

//---------------------------------------------------------
//   useDrumset
//---------------------------------------------------------

bool Instrument::useDrumset() const
      {
      return d->useDrumset();
      }

//---------------------------------------------------------
//   setUseDrumset
//---------------------------------------------------------

void Instrument::setUseDrumset(bool val)
      {
      d->setUseDrumset(val);
      }

//---------------------------------------------------------
//   setAmateurPitchRange
//---------------------------------------------------------

void Instrument::setAmateurPitchRange(int a, int b)
      {
      d->setAmateurPitchRange(a, b);
      }

//---------------------------------------------------------
//   setProfessionalPitchRange
//---------------------------------------------------------

void Instrument::setProfessionalPitchRange(int a, int b)
      {
      d->setProfessionalPitchRange(a, b);
      }

//---------------------------------------------------------
//   channel
//---------------------------------------------------------

Channel& Instrument::channel(int idx)
      {
      return d->channel(idx);
      }

//---------------------------------------------------------
//   channel
//---------------------------------------------------------

const Channel& Instrument::channel(int idx) const
      {
      return d->channel(idx);
      }

//---------------------------------------------------------
//   midiActions
//---------------------------------------------------------

const QList<NamedEventList>& Instrument::midiActions() const
      {
      return d->midiActions();
      }

//---------------------------------------------------------
//   articulation
//---------------------------------------------------------

const QList<MidiArticulation>& Instrument::articulation() const
      {
      return d->articulation();
      }

//---------------------------------------------------------
//   channel
//---------------------------------------------------------

const QList<Channel>& Instrument::channel() const
      {
      return d->channel();
      }

//---------------------------------------------------------
//   setMidiActions
//---------------------------------------------------------

void Instrument::setMidiActions(const QList<NamedEventList>& l)
      {
      d->setMidiActions(l);
      }

//---------------------------------------------------------
//   setArticulation
//---------------------------------------------------------

void Instrument::setArticulation(const QList<MidiArticulation>& l)
      {
      d->setArticulation(l);
      }

//---------------------------------------------------------
//   setChannel
//---------------------------------------------------------

void Instrument::setChannel(const QList<Channel>& l)
      {
      d->setChannel(l);
      }

//---------------------------------------------------------
//   setChannel
//---------------------------------------------------------

void Instrument::setChannel(int i, const Channel& c)
      {
      d->setChannel(i, c);
      }

//---------------------------------------------------------
//   tablature
//---------------------------------------------------------

Tablature* Instrument::tablature() const
      {
      return d->tablature();
      }

//---------------------------------------------------------
//   setTablature
//---------------------------------------------------------

void Instrument::setTablature(Tablature* t)
      {
      d->setTablature(t);
      }

//---------------------------------------------------------
//   instrument
//---------------------------------------------------------

const Instrument& InstrumentList::instrument(int tick) const
      {
      if (empty())
            return defaultInstrument;
      ciInstrument i = upper_bound(tick);
      if (i == begin())
            return defaultInstrument;
      --i;
      return i->second;
      }

//---------------------------------------------------------
//   instrument
//---------------------------------------------------------

Instrument& InstrumentList::instrument(int tick)
      {
      if (empty())
            return defaultInstrument;
      iInstrument i = upper_bound(tick);
      if (i == begin())
            return defaultInstrument;
      --i;
      return i->second;
      }

//---------------------------------------------------------
//   setInstrument
//---------------------------------------------------------

void InstrumentList::setInstrument(const Instrument& instr, int tick)
      {
      std::pair<int, Instrument> instrument(tick, instr);
      std::pair<iInstrument,bool> p = insert(instrument);
      if (!p.second)
            (*this)[tick] = instr;
      }

//---------------------------------------------------------
//   longName
//---------------------------------------------------------

const QList<StaffNameDoc>& Instrument::longNames() const
      {
      return d->_longNames;
      }

//---------------------------------------------------------
//   shortName
//---------------------------------------------------------

const QList<StaffNameDoc>& Instrument::shortNames() const
      {
      return d->_shortNames;
      }

//---------------------------------------------------------
//   longName
//---------------------------------------------------------

QList<StaffNameDoc>& Instrument::longNames()
      {
      return d->_longNames;
      }

//---------------------------------------------------------
//   setLongName
//---------------------------------------------------------

void Instrument::setLongName(const QTextDocumentFragment& f)
      {
      d->setLongName(f);
      }

//---------------------------------------------------------
//   addLongName
//---------------------------------------------------------

void Instrument::addLongName(const StaffNameDoc& f)
      {
      d->addLongName(f);
      }

//---------------------------------------------------------
//   addShortName
//---------------------------------------------------------

void Instrument::addShortName(const StaffNameDoc& f)
      {
      d->addShortName(f);
      }

//---------------------------------------------------------
//   setShortName
//---------------------------------------------------------

void Instrument::setShortName(const QTextDocumentFragment& f)
      {
      d->setShortName(f);
      }

//---------------------------------------------------------
//   shortName
//---------------------------------------------------------

QList<StaffNameDoc>& Instrument::shortNames()
      {
      return d->_shortNames;
      }

//---------------------------------------------------------
//   trackName
//---------------------------------------------------------

QString Instrument::trackName() const
      {
      return d->_trackName;
      }

void Instrument::setTrackName(const QString& s)
      {
      d->_trackName = s;
      }

//---------------------------------------------------------
//   fromTemplate
//---------------------------------------------------------

Instrument Instrument::fromTemplate(const InstrumentTemplate* t)
      {
      Instrument instr;
      instr.setAmateurPitchRange(t->minPitchA, t->maxPitchA);
      instr.setProfessionalPitchRange(t->minPitchP, t->maxPitchP);
      foreach(StaffName sn, t->longNames)
            instr.addLongName(StaffNameDoc(sn.name, sn.pos));
      foreach(StaffName sn, t->shortNames)
            instr.addShortName(StaffNameDoc(sn.name, sn.pos));
      instr.setTrackName(t->trackName);
      instr.setTranspose(t->transpose);
      if (t->useDrumset) {
            instr.setUseDrumset(true);
            instr.setDrumset(new Drumset(*((t->drumset) ? t->drumset : smDrumset)));
            }
      instr.setMidiActions(t->midiActions);
      instr.setArticulation(t->articulation);
      instr.setChannel(t->channel);
      instr.setTablature(t->tablature ? new Tablature(*t->tablature) : 0);
      return instr;
      }

