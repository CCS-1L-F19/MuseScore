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

#include <algorithm>
#include <vector>
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
      int wrappedRow = 0, wrappedCol = 0, origRow = 0, origCol = 0;

      for (const TextBlock& t : _original.textBlockList()) {
            _text.appendTextBlock();
            _text.textBlockList().last().setEol(true);
            lineLen = origCol = wrappedCol = 0;
            for (TextFragment frag : t.fragments()) {
                   QFont fon = frag.font(&_text);
                   fon.setPointSizeF(fon.pointSizeF() * MScore::pixelRatio);
                   fm = QFontMetricsF(fon);
                   lineLen += fm.horizontalAdvance(frag.text);
                   if (lineLen <= w) {
                         _text.appendFragment(frag);
                         for (int i = 0; i < frag.columns(); ++i) {
                               posMap.push_back({origRow, origCol, wrappedRow, wrappedCol});
                               ++wrappedCol;
                               ++origCol;
                               }

                         }
                   else {
                         TextFragment back;
                         lineLen -= fm.horizontalAdvance(frag.text);
                         int col = 0, idx = 0, lastWordCol = 0, lastWordColCount = 0;
                         qreal lastWordLen = 0;
                         bool prevCharIsSpace = false;
                         QString s = frag.text;
                         for (const QChar& c : s) {
                               if (c == ' ') {
                                     prevCharIsSpace = true;
                                     lastWordLen += fm.horizontalAdvance(c);
                                     ++lastWordColCount;
                                     }
                               else if (prevCharIsSpace) {
                                     lastWordCol = col;
                                     lastWordLen = 0;
                                     lastWordColCount = 0;
                                     prevCharIsSpace = false;
                                     }
                               else {
                                     if (c.isLowSurrogate())
                                           ++lastWordColCount;
                                     lastWordLen += fm.horizontalAdvance(c);
                                     }

                               lineLen += fm.horizontalAdvance(c);
                               if (lineLen > w) {
                                     back = frag.split(lastWordCol != 0 ? lastWordCol : col);
                                     for (int i = 0; i < frag.columns(); ++i) {
                                           posMap.push_back({origRow, origCol, wrappedRow, wrappedCol});
                                           ++wrappedCol;
                                           ++origCol;
                                           }
                                     posMap.push_back({origRow, origCol, wrappedRow, wrappedCol});
                                     _text.appendFragment(frag);
                                     _text.appendTextBlock();
                                     _text.textBlockList().last().setEol(true);
                                     ++wrappedRow;
                                     col = idx = wrappedCol = 0;
                                     lineLen = fm.horizontalAdvance(c);
                                     if (lastWordCol != 0) {
                                           col = idx = lastWordColCount;
                                           lineLen += lastWordLen;
                                           }
                                     lastWordCol = lastWordColCount = lastWordLen = 0;
                                     frag = back;
                                     }
                               ++idx;
                               if (c.isHighSurrogate())
                                     continue;
                               ++col;
                               }
                         for (int i = 0; i < frag.columns(); ++i) {
                               posMap.push_back({origRow, origCol, wrappedRow, wrappedCol});
                               ++wrappedCol;
                               ++origCol;
                               }
                         posMap.push_back({origRow, origCol, wrappedRow, wrappedCol});
                         lineLen = fm.horizontalAdvance(frag.text);
                         _text.appendFragment(frag);
                         }
                   ++wrappedRow;
                   ++origRow;
                   }
            }
            posMap.push_back({origRow - 1, origCol, wrappedRow - 1, wrappedCol});
            posMap.push_back({origRow, 0, wrappedRow, 0});
            _text.layout();
      }

//---------------------------------------------------------
//   translatedToWrapped
//---------------------------------------------------------

TextCursor WrappedText::translatedToWrapped(const TextCursor& cur) {
      TextCursor result = TextCursor(static_cast<TextBase*>(&_text));

      auto posIt = std::find_if(posMap.begin(), posMap.end(), [cur](const PositionPair& p) {
            return p.originalCol == cur.column() && p.originalRow == cur.row();
      });
      auto selectPosIt = std::find_if(posMap.begin(), posMap.end(), [cur](const PositionPair& p) {
            return p.originalCol == cur.selectColumn() && p.originalRow == cur.selectLine();
      });

      Q_ASSERT(posIt != posMap.end() && selectPosIt != posMap.end());

      result.setRow(posIt->wrappedRow);
      result.setColumn(posIt->wrappedCol);
      result.setSelectLine(selectPosIt->wrappedRow);
      result.setSelectColumn(selectPosIt->wrappedCol);

      return result;
      }

//---------------------------------------------------------
//   translatedToOriginal
//---------------------------------------------------------

TextCursor WrappedText::translatedToOriginal(const TextCursor& cur) {
      TextCursor result = TextCursor(static_cast<TextBase*>(&_original));

      auto posIt = std::find_if(posMap.begin(), posMap.end(), [cur](const PositionPair& p) {
            return p.wrappedCol == cur.column() && p.wrappedRow == cur.row();
      });
      auto selectPosIt = std::find_if(posMap.begin(), posMap.end(), [cur](const PositionPair& p) {
            return p.wrappedCol == cur.selectColumn() && p.wrappedRow == cur.selectLine();
      });

      Q_ASSERT(posIt != posMap.end() && selectPosIt != posMap.end());

      result.setRow(posIt->originalRow);
      result.setColumn(posIt->originalCol);
      result.setSelectLine(selectPosIt->originalRow);
      result.setSelectColumn(selectPosIt->originalCol);

      return result;

      }
}
