/*
 * FBReader -- electronic book reader
 * Copyright (C) 2005 Nikolay Pultsin <geometer@mawhrin.net>
 * Copyright (C) 2005 Mikhail Sobolev <mss@mawhrin.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __PALMPAINTCONTEXT_H__
#define __PALMQPAINTCONTEXT_H__

#include <PalmOS.h>				

//#include <map>

#include "../common/view/PaintContext.h"
#include "../common/textview/Word.h"

class PalmPaintContext : public PaintContext {

public:
	PalmPaintContext() VIEW_SECTION;
	~PalmPaintContext() VIEW_SECTION;
	void removeCaches() VIEW_SECTION;

	//const QPixmap &pixmap() const { return *myPixmap; }

	void setSize(int w, int h) VIEW_SECTION;

	int width() const VIEW_SECTION;
	int height() const VIEW_SECTION;

	void clear() VIEW_SECTION;

	void fillFamiliesList(std::vector<std::string> &families) const VIEW_SECTION;
	const std::string realFontFamilyName(std::string &fontFamily) const VIEW_SECTION;

	void setFont(const std::string &family, int size, bool bold, bool italic) VIEW_SECTION;
	void setColor(ZLColor color) VIEW_SECTION;
	void setFillColor(ZLColor color) VIEW_SECTION;

	int stringWidth(const std::string &str, int from, int len) const VIEW_SECTION;
	int stringHeight() const VIEW_SECTION;
	void drawString(int x, int y, const std::string &str, int from, int len) VIEW_SECTION;

	int imageWidth(const Image &image) const VIEW_SECTION;
	int imageHeight(const Image &image) const VIEW_SECTION;
	void drawImage(int x, int y, const Image &image) VIEW_SECTION;

	void drawLine(int x0, int y0, int x1, int y1) VIEW_SECTION;
	void fillRectangle(int x0, int y0, int x1, int y1) VIEW_SECTION;

private:
	//QImage &qImage(const Image &image) const;

private:
	//mutable std::map<const Image*,QImage*> myImageCache;
	int myWidth, myHeight;
	RGBColorType myFillColor;
};

#endif /* __PALMPAINTCONTEXT_H__ */
