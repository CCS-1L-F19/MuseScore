//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2011-2014 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "text.h"
#include "xml.h"
#include "score.h"

namespace Ms {

//---------------------------------------------------------
//   defaultStyle
//---------------------------------------------------------

static const ElementStyle defaultStyle {
      { Sid::defaultSystemFlag, Pid::SYSTEM_FLAG },
      };

//---------------------------------------------------------
//   Text
//---------------------------------------------------------

Text::Text(Score* s, Tid tid) : TextBase(s, tid)
      {
      initElementStyle(&defaultStyle);
      }


Text::Text(const TextBase& tb) : TextBase(tb)
      {
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Text::read(XmlReader& e)
      {
      while (e.readNextStartElement()) {
            const QStringRef& tag(e.name());
            if (tag == "style") {
                  QString sn = e.readElementText();
                  if (sn == "Tuplet")          // ugly hack for compatibility
                        continue;
                  Tid s = textStyleFromName(sn);
                  initTid(s);
                  }
            else if (!readProperties(e))
                  e.unknown();
            }
      }

//---------------------------------------------------------
//   propertyDefault
//---------------------------------------------------------

QVariant Text::propertyDefault(Pid id) const
      {
      switch(id) {
            case Pid::SUB_STYLE:
                  return int(Tid::DEFAULT);
            default:
                  return TextBase::propertyDefault(id);
            }
      }

//---------------------------------------------------------
//   WrappedText
//---------------------------------------------------------

WrappedText::WrappedText(const TextBase& tex, qreal w)
   : _original(tex), _text(tex), _width(w)
      {
      _original.textBlockList() = const_cast<TextBase&>(tex).textBlockList();
      _text.textBlockList().clear();
      QFontMetricsF fm(_text.font());
      qreal lineLen = 0;
      for (const TextBlock& t : _original.textBlockList()) {
            _text.appendTextBlock();
            _text.textBlockList().last().setEol(true);
            for (TextFragment frag : t.fragments()) {
                   QFont fon = frag.font(&_text);
                   fon.setPointSizeF(fon.pointSizeF() * MScore::pixelRatio);
                   fm = QFontMetricsF(fon);
                   lineLen += fm.horizontalAdvance(frag.text);
                   if (lineLen <= w) {
                         _text.appendFragment(frag);
                         }
                   else {
                         TextFragment back;
                         lineLen -= fm.horizontalAdvance(frag.text);
                         int col = 0, idx = 0;
                         for (const QChar& c : frag.text) {
                               lineLen += fm.horizontalAdvance(c);
                               if (lineLen > w) {
                                     break;
                                     }
                               ++idx;
                               if (c.isHighSurrogate())
                                     continue;
                               ++col;
                               }
                         back = frag.split(col);
                         _text.appendFragment(frag);
                         _text.appendTextBlock();
                         _text.textBlockList().last().setEol(true);
                         _text.appendFragment(back);
                         lineLen = fm.horizontalAdvance(frag.text);
                         }
                   }
            }
            _text.layout();
      }
}


