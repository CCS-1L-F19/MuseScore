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
      qreal lineLen;

      for (const TextBlock& t : _original.textBlockList()) {
            lineLen = 0;
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
                         QString s = frag.text;
                         for (const QChar& c : s) {
                               lineLen += fm.horizontalAdvance(c);
                               if (lineLen > w) {
                                     back = frag.split(col);
                                     _text.appendFragment(frag);
                                     _text.appendTextBlock();
                                     _text.textBlockList().last().setEol(true);
                                     lineLen = col = idx = 0;
                                     frag = back;
                                     }
                               ++idx;
                               if (c.isHighSurrogate())
                                     continue;
                               ++col;
                               }
                         lineLen = fm.horizontalAdvance(frag.text);
                         _text.appendFragment(frag);
                         }
                   }
            }
            _text.layout();
      }

//---------------------------------------------------------
//   translatedToWrappedRowColPair
//---------------------------------------------------------

std::pair<int, int> WrappedText::translatedToWrappedRowColPair(int r, int c) {
      int wrappedRow = 0, tempCol = 0;
      for (int tempOrigRow = 0; tempOrigRow < r; ++tempOrigRow) {
            int origRowLen = _original.textBlock(tempOrigRow).columns();
            int tempRowLen = 0;
            for (; tempRowLen < origRowLen; ++wrappedRow)
                  tempRowLen += _text.textBlock(wrappedRow).columns();
            }
      // Now _text.textBlock(wrappedRow)'s fragments will be a subset of _original.textBlock(r)'s fragments.

      for (;tempCol <= c && wrappedRow != _text.textBlockList().length(); ++wrappedRow)
            tempCol += _text.textBlock(wrappedRow).columns();

      --wrappedRow;
      tempCol -= _text.textBlock(wrappedRow).columns();

      // Now wrappedRow will contain (r, c)'s match. tempCol will be the col that points to
      // the start of wrappedRow in the original row.

      return {wrappedRow, c-tempCol};


      }

//---------------------------------------------------------
//   translatedToWrapped
//---------------------------------------------------------

TextCursor WrappedText::translatedToWrapped(const TextCursor& cur) {
      TextCursor result = TextCursor(static_cast<TextBase*>(&_text));

      auto pos = translatedToWrappedRowColPair(cur.row(), cur.column());
      auto selectPos = translatedToWrappedRowColPair(cur.selectLine(), cur.selectColumn());

      result.setRow(pos.first);
      result.setColumn(pos.second);
      result.setSelectLine(selectPos.first);
      result.setSelectColumn(selectPos.second);

      return result;
      }
}


