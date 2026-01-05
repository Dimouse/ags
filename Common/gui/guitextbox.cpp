//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/keycode.h"
#include "font/fonts.h"
#include "gui/guimain.h"
#include "gui/guitextbox.h"
#include "util/stream.h"
#include "util/string_utils.h"
#include "util/utf8.h"

#define GUITEXTBOX_LEGACY_TEXTLEN 200

namespace AGS
{
namespace Common
{

/* static */ String GUITextBox::EventNames[GUITextBox::EventCount] =
    { "Activate" };
/* static */ String GUITextBox::EventArgs[GUITextBox::EventCount] =
    { "GUIControl *control" };

GUITextBox::GUITextBox()
{
}

void GUITextBox::SetFont(int font)
{
    if (_font != font)
    {
        _font = font;
        MarkChanged();
    }
}

void GUITextBox::SetTextColor(int color)
{
    if (_textColor != color)
    {
        _textColor = color;
        MarkChanged();
    }
}

void GUITextBox::SetText(const String &text)
{
    if (_text != text)
    {
        _text = text;
        MarkChanged();
    }
}

bool GUITextBox::HasAlphaChannel() const
{
    return is_font_antialiased(_font);
}

bool GUITextBox::IsBorderShown() const
{
    return (_textBoxFlags & kTextBox_ShowBorder) != 0;
}

uint32_t GUITextBox::GetEventCount() const
{
    return EventCount;
}

String GUITextBox::GetEventName(uint32_t event) const
{
    if (event >= EventCount)
        return "";
    return EventNames[event];
}

String GUITextBox::GetEventArgs(uint32_t event) const
{
    if (event >= EventCount)
        return "";
    return EventArgs[event];
}

Rect GUITextBox::CalcGraphicRect(bool clipped)
{
    if (clipped)
        return RectWH(0, 0, _width, _height);

    // TODO: need to find a way to cache text position, or there'll be some repetition
    Rect rc = RectWH(0, 0, _width, _height);
    Point text_at(1 + get_fixed_pixel_size(1), 1 + get_fixed_pixel_size(1));
    Rect text_rc = GUI::CalcTextGraphicalRect(_text, _font, text_at);
    if (GUI::IsGUIEnabled(this))
    {
        // add a cursor
        Rect cur_rc = RectWH(
            text_rc.Right + 3,
            1 + get_font_height(_font),
            get_fixed_pixel_size(5),
            get_fixed_pixel_size(1) - 1);
        text_rc = SumRects(text_rc, cur_rc);
    }
    return SumRects(rc, text_rc);
}

void GUITextBox::Draw(Bitmap *ds, int x, int y)
{
    color_t text_color = ds->GetCompatibleColor(_textColor);
    color_t draw_color = ds->GetCompatibleColor(_textColor);
    if (IsBorderShown())
    {
        ds->DrawRect(RectWH(x, y, _width, _height), draw_color);
        if (get_fixed_pixel_size(1) > 1)
        {
            ds->DrawRect(Rect(x + 1, y + 1, x + _width - get_fixed_pixel_size(1), y + _height - get_fixed_pixel_size(1)), draw_color);
        }
    }
    DrawTextBoxContents(ds, x, y, text_color);
}

// TODO: a shared utility function
static void Backspace(String &text)
{
    if (get_uformat() == U_UTF8)
    {// Find where the last utf8 char begins
        const char *ptr_end = text.GetCStr() + text.GetLength();
        const char *ptr_prev = Utf8::BackOneChar(ptr_end, text.GetCStr());
        text.ClipRight(ptr_end - ptr_prev);
    }
    else
    {
        text.ClipRight(1);
    }
}

//code credited to Den Zurin https://gamedev.ru/code/forum/?id=119312&page=2&m=1645259#m21 
int wtable[64] = {
  0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
  0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
  0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x007F, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
  0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
  0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
  0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
  0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457};
  
int utf8_to_win1251(const char* text, char* wtext)
{
  int wc, uc;
  int i, j, k, m;
  if (!wtext)
    return 0;
  i=0;
  j=0;
  while (i<strlen(text))
  {
    /* read Unicode character */
    /* read first UTF-8 byte */
    wc = (unsigned char)text[i++];
    /* 11111xxx - not symbol (BOM etc) */
    if (wc>=0xF8)
    {
      m = -1;
    }
    /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx -> 0x00010000 — 0x001FFFFF */
    else if (wc>=0xF0)
    {
      uc = (wc&0x07); 
      m = 3;
    }
    /* 1110xxxx 10xxxxxx 10xxxxxx -> 0x00000800 — 0x0000FFFF */
    else if (wc>=0xE0)
    {
      uc = (wc&0x0F); 
      m = 2;
    }
    /* 110xxxxx 10xxxxxx -> 0x00000080 — 0x000007FF */
    else if (wc>=0xC0)
    {
      uc = (wc&0x1F); 
      m = 1;
    }
    /* 0xxxxxxx -> 0x00000000 — 0x0000007F */
    else if (wc<=0x7F)
    {
      uc = wc;
      m = 0;
    }
    /* 10xxxxxx - data error! */
    else
    {
      m = -1;
    }
    /* read m UTF-8 bytes (10xxxxxx) */
    k = 1;
    wc = 0;
    while (k<=m && wc<=0xBF)
    {
      wc = (unsigned char)text[i++];
      uc <<= 6;
      uc += (wc&0x3F);
      k++;
    }
    if (wc>0xBF || m<0)
    {
      uc = -1;
    }
    /* Unicode to Windows-1251 */
    if (uc<0)
    {
      wc = -1;
    }
    else if (uc<=0x7F) /* ASCII */
    {
      wc = uc;
    }
    else if (uc>=0x0410 && uc<=0x042F) /* А-Я */
    {
      wc = uc - 0x410 + 0xC0;
    }
    else if (uc>=0x0430 && uc<=0x044F) /* а-я */
    {
      wc = uc - 0x0430 + 0xE0;
    }
    else /* Ђ-ї */
    {
      /* search in wtable */
      k = 0;
      while (k<64 && wtable[k]!=uc)
      {
        k++;
      }
      if (k<64)
      {
        wc = k + 0x80;
      }
      else
      {
        wc = '?';
      }
    }
    /* save Windows-1251 character */
    if (wc>0)
    {
      wtext[j++] = (char)wc;
    }
  }
  wtext[j] = 0x00;
  return 1;
}

bool GUITextBox::OnKeyPress(const KeyInput &ki)
{
    switch (ki.Key)
    {
    case eAGSKeyCodeReturn:
        _isActivated = true;
        return true;
    case eAGSKeyCodeBackspace:
        Backspace(_text);
        MarkChanged();
        return true;
    default: break;
    }

    if (ki.UChar > 256){ //code for cyrillic cp1251 input (Windows)
        char str[500];
        utf8_to_win1251(ki.Text,str);
        _text.Append(str); // proper unicode char
        MarkChanged();
        return true;
    }
    if (ki.UChar == 0)
        return false; // not a textual event, don't handle

    if (get_uformat() == U_UTF8)
        _text.Append(ki.Text); // proper unicode char
    else if (ki.UChar < 256)
        _text.AppendChar(static_cast<uint8_t>(ki.UChar)); // ascii/ansi-range char in ascii mode
    else
        return true; // char from an unsupported range, don't print but still report as handled
    // if the new string is too long, remove the new character
    if (get_text_width(_text.GetCStr(), _font) > (_width - (6 + get_fixed_pixel_size(5))))
        Backspace(_text);
    MarkChanged();
    return true;
}

void GUITextBox::SetShowBorder(bool on)
{
    if (on)
        _textBoxFlags |= kTextBox_ShowBorder;
    else
        _textBoxFlags &= ~kTextBox_ShowBorder;
}

// TODO: replace string serialization with StrUtil::ReadString and WriteString
// methods in the future, to keep this organized.
void GUITextBox::WriteToFile(Stream *out) const
{
    GUIObject::WriteToFile(out);
    StrUtil::WriteString(_text, out);
    out->WriteInt32(_font);
    out->WriteInt32(_textColor);
    out->WriteInt32(_textBoxFlags);
}

void GUITextBox::ReadFromFile(Stream *in, GuiVersion gui_version)
{
    GUIObject::ReadFromFile(in, gui_version);
    if (gui_version < kGuiVersion_350)
        _text.ReadCount(in, GUITEXTBOX_LEGACY_TEXTLEN);
    else
        _text = StrUtil::ReadString(in);
    _font = in->ReadInt32();
    _textColor = in->ReadInt32();
    _textBoxFlags = in->ReadInt32();
    // reverse particular flags from older format
    if (gui_version < kGuiVersion_350)
        _textBoxFlags ^= kTextBox_OldFmtXorMask;

    if (_textColor == 0)
        _textColor = 16;
}

void GUITextBox::ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver)
{
    GUIObject::ReadFromSavegame(in, svg_ver);
    _font = in->ReadInt32();
    _textColor = in->ReadInt32();
    _text = StrUtil::ReadString(in);
    if (svg_ver >= kGuiSvgVersion_350)
        _textBoxFlags = in->ReadInt32();
}

void GUITextBox::WriteToSavegame(Stream *out) const
{
    GUIObject::WriteToSavegame(out);
    out->WriteInt32(_font);
    out->WriteInt32(_textColor);
    StrUtil::WriteString(_text, out);
    out->WriteInt32(_textBoxFlags);
}

} // namespace Common
} // namespace AGS
