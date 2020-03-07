//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2014 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __TEXT_H__
#define __TEXT_H__

#include "textbase.h"

namespace Ms {

class WrappedText;

//---------------------------------------------------------
//   Text
//---------------------------------------------------------

class Text final: public TextBase {

   public:
      Text(Score* s = 0, Tid tid = Tid::DEFAULT);
      Text(const TextBase&);

      virtual ElementType type() const override    { return ElementType::TEXT; }
      virtual Text* clone() const override         { return new Text(*this); }
      virtual void read(XmlReader&) override;
      virtual QVariant propertyDefault(Pid id) const override;

      friend class WrappedText;
      };

class WrappedText {
      Text  _original;
      Text  _text;
      qreal _width;

      std::set<std::pair<int /*originalRow*/, int/*wrappedRow*/>> rowMap{};

      std::pair<int, int> translatedToWrappedRowColPair(int r, int c);
      std::pair<int, int> translatedToOriginalRowColPair(int r, int c);
   public:
      WrappedText(const TextBase&, qreal);
      QList<TextBlock>& textBlockList() { return _text.textBlockList(); }
      qreal width() const { return _width; }
      TextCursor translatedToWrapped(const TextCursor&);
      TextCursor translatedToOriginal(const TextCursor&);
      Text& text()  { return _text; }
      };

}     // namespace Ms

#endif

