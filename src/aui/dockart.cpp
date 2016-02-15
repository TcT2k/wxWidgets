///////////////////////////////////////////////////////////////////////////////
// Name:        src/aui/dockart.cpp
// Purpose:     wxaui: wx advanced user interface - docking window manager
// Author:      Benjamin I. Williams
// Modified by:
// Created:     2005-05-17
// Copyright:   (C) Copyright 2005-2006, Kirix Corporation, All Rights Reserved
// Licence:     wxWindows Library Licence, Version 3.1
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_AUI

#include "wx/aui/framemanager.h"
#include "wx/aui/dockart.h"
#include "wx/aui/auibook.h"
#include "wx/aui/tabart.h"
#include "wx/utils.h"

#ifndef WX_PRECOMP
    #include "wx/settings.h"
    #include "wx/dcclient.h"
    #include "wx/image.h"
#endif

#ifdef __WXMAC__
#include "wx/osx/private.h"
#include "wx/graphics.h"
#include "wx/dcgraph.h"
#endif

#ifdef __WXGTK__
#include <gtk/gtk.h>
#include "wx/renderer.h"
#ifdef __WXGTK20__
   #include "wx/gtk/private/gtk2-compat.h"
#else
   #define gtk_widget_is_drawable GTK_WIDGET_DRAWABLE
#endif
#ifdef __WXGTK3__
    #include "wx/graphics.h"
    #include "wx/gtk/private.h"
#endif
#endif


// -- wxAuiDefaultDockArt class implementation --

// wxAuiDefaultDockArt is an art provider class which does all of the drawing for
// wxAuiManager.  This allows the library caller to customize the dock art
// (probably by deriving from this class), or to completely replace all drawing
// with custom dock art (probably by writing a new stand-alone class derived
// from the wxAuiDockArt base class). The active dock art class can be set via
// wxAuiManager::SetDockArt()
wxColor wxAuiLightContrastColour(const wxColour& c)
{
    int amount = 120;

    // if the color is especially dark, then
    // make the contrast even lighter
    if (c.Red() < 128 && c.Green() < 128 && c.Blue() < 128)
        amount = 160;

    return c.ChangeLightness(amount);
}

// wxAuiBitmapFromBits() is a utility function that creates a
// masked bitmap from raw bits (XBM format)
wxBitmap wxAuiBitmapFromBits(const unsigned char bits[], int w, int h,
                             const wxColour& color)
{
    wxImage img = wxBitmap((const char*)bits, w, h).ConvertToImage();
    img.Replace(0,0,0,123,123,123);
    img.Replace(255,255,255,color.Red(),color.Green(),color.Blue());
    img.SetMaskColour(123,123,123);
    return wxBitmap(img);
}


static void DrawGradientRectangle(wxDC& dc,
                                  const wxRect& rect,
                                  const wxColour& start_color,
                                  const wxColour& end_color,
                                  int direction)
{
    dc.GradientFillLinear(rect, start_color, end_color, (direction == wxAUI_GRADIENT_VERTICAL) ? wxSOUTH : wxEAST);
}

wxString wxAuiChopText(wxDC& dc, const wxString& text, int max_size)
{
    wxCoord x,y;

    // first check if the text fits with no problems
    dc.GetTextExtent(text, &x, &y);
    if (x <= max_size)
        return text;

    size_t i, len = text.Length();
    size_t last_good_length = 0;
    for (i = 0; i < len; ++i)
    {
        wxString s = text.Left(i);
        s += wxT("...");

        dc.GetTextExtent(s, &x, &y);
        if (x > max_size)
            break;

        last_good_length = i;
    }

    wxString ret = text.Left(last_good_length);
    ret += wxT("...");
    return ret;
}

wxAuiDefaultDockArt::wxAuiDefaultDockArt()
{
#if defined( __WXMAC__ ) && wxOSX_USE_COCOA_OR_CARBON
    wxColor baseColour = wxColour( wxMacCreateCGColorFromHITheme(kThemeBrushToolbarBackground));
#else
    wxColor baseColour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
#endif

    // the baseColour is too pale to use as our base colour,
    // so darken it a bit --
    if ((255-baseColour.Red()) +
        (255-baseColour.Green()) +
        (255-baseColour.Blue()) < 60)
    {
        baseColour = baseColour.ChangeLightness(92);
    }

    m_baseColour = baseColour;

    m_activeCaptionColour = wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION);
    m_activeCaptionGradientColour = wxSystemSettings::GetColour(wxSYS_COLOUR_GRADIENTACTIVECAPTION);
    m_activeCaptionTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_CAPTIONTEXT);
    m_inactiveCaptionColour = wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVECAPTION);
    m_inactiveCaptionGradientColour = wxSystemSettings::GetColour(wxSYS_COLOUR_GRADIENTINACTIVECAPTION);
    m_inactiveCaptionTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_INACTIVECAPTIONTEXT);

    m_sashBrush = wxBrush(baseColour);
    m_backgroundBrush = wxBrush(baseColour);
    m_gripperBrush = wxBrush(baseColour);

    m_borderPen = wxPen(baseColour.ChangeLightness(75));
    m_gripperPen1 = wxPen(baseColour.ChangeLightness(40));
    m_gripperPen2 = wxPen(baseColour.ChangeLightness(60));
    m_gripperPen3 = *wxWHITE_PEN;

#ifdef __WXMAC__
    m_captionFont = *wxSMALL_FONT;
#else
    m_captionFont = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
#endif

    // default metric values
#if defined( __WXMAC__ ) && wxOSX_USE_COCOA_OR_CARBON
    SInt32 height;
    GetThemeMetric( kThemeMetricSmallPaneSplitterHeight , &height );
    m_sashSize = height;
#elif defined(__WXGTK__)
    m_sashSize = wxRendererNative::Get().GetSplitterParams(NULL).widthSash;
#else
    m_sashSize = 4;
#endif
    wxSize captionFontSize = m_captionFont.GetPixelSize();
    if (!captionFontSize.IsFullySpecified())
        captionFontSize.Set(0, 11);
    m_captionSize = captionFontSize.GetHeight() + 6;
    m_borderSize = 1;
    m_buttonSize = m_captionSize - 2;
    m_gripperSize = 9;
#ifdef __WXMSW__
    if (wxCheckOsVersion(6, 2))
        m_gradientType = wxAUI_GRADIENT_NONE;
    else
#endif
        m_gradientType = wxAUI_GRADIENT_VERTICAL;
}

int wxAuiDefaultDockArt::GetMetric(int id)
{
    switch (id)
    {
        case wxAUI_DOCKART_SASH_SIZE:          return m_sashSize;
        case wxAUI_DOCKART_CAPTION_SIZE:       return m_captionSize;
        case wxAUI_DOCKART_GRIPPER_SIZE:       return m_gripperSize;
        case wxAUI_DOCKART_PANE_BORDER_SIZE:   return m_borderSize;
        case wxAUI_DOCKART_PANE_BUTTON_SIZE:   return m_buttonSize;
        case wxAUI_DOCKART_GRADIENT_TYPE:      return m_gradientType;
        default: wxFAIL_MSG(wxT("Invalid Metric Ordinal")); break;
    }

    return 0;
}

void wxAuiDefaultDockArt::SetMetric(int id, int new_val)
{
    switch (id)
    {
        case wxAUI_DOCKART_SASH_SIZE:          m_sashSize = new_val; break;
        case wxAUI_DOCKART_CAPTION_SIZE:       m_captionSize = new_val; break;
        case wxAUI_DOCKART_GRIPPER_SIZE:       m_gripperSize = new_val; break;
        case wxAUI_DOCKART_PANE_BORDER_SIZE:   m_borderSize = new_val; break;
        case wxAUI_DOCKART_PANE_BUTTON_SIZE:   m_buttonSize = new_val; break;
        case wxAUI_DOCKART_GRADIENT_TYPE:      m_gradientType = new_val; break;
        default: wxFAIL_MSG(wxT("Invalid Metric Ordinal")); break;
    }
}

wxColour wxAuiDefaultDockArt::GetColour(int id)
{
    switch (id)
    {
        case wxAUI_DOCKART_BACKGROUND_COLOUR:                return m_backgroundBrush.GetColour();
        case wxAUI_DOCKART_SASH_COLOUR:                      return m_sashBrush.GetColour();
        case wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR:          return m_inactiveCaptionColour;
        case wxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR: return m_inactiveCaptionGradientColour;
        case wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR:     return m_inactiveCaptionTextColour;
        case wxAUI_DOCKART_ACTIVE_CAPTION_COLOUR:            return m_activeCaptionColour;
        case wxAUI_DOCKART_ACTIVE_CAPTION_GRADIENT_COLOUR:   return m_activeCaptionGradientColour;
        case wxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR:       return m_activeCaptionTextColour;
        case wxAUI_DOCKART_BORDER_COLOUR:                    return m_borderPen.GetColour();
        case wxAUI_DOCKART_GRIPPER_COLOUR:                   return m_gripperBrush.GetColour();
        default: wxFAIL_MSG(wxT("Invalid Metric Ordinal")); break;
    }

    return wxColour();
}

void wxAuiDefaultDockArt::SetColour(int id, const wxColor& colour)
{
    switch (id)
    {
        case wxAUI_DOCKART_BACKGROUND_COLOUR:                m_backgroundBrush.SetColour(colour); break;
        case wxAUI_DOCKART_SASH_COLOUR:                      m_sashBrush.SetColour(colour); break;
        case wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR:          m_inactiveCaptionColour = colour; break;
        case wxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR: m_inactiveCaptionGradientColour = colour; break;
        case wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR:     m_inactiveCaptionTextColour = colour; break;
        case wxAUI_DOCKART_ACTIVE_CAPTION_COLOUR:            m_activeCaptionColour = colour; break;
        case wxAUI_DOCKART_ACTIVE_CAPTION_GRADIENT_COLOUR:   m_activeCaptionGradientColour = colour; break;
        case wxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR:       m_activeCaptionTextColour = colour; break;
        case wxAUI_DOCKART_BORDER_COLOUR:                    m_borderPen.SetColour(colour); break;
        case wxAUI_DOCKART_GRIPPER_COLOUR:
            m_gripperBrush.SetColour(colour);
            m_gripperPen1.SetColour(colour.ChangeLightness(40));
            m_gripperPen2.SetColour(colour.ChangeLightness(60));
            break;
        default: wxFAIL_MSG(wxT("Invalid Metric Ordinal")); break;
    }
}

void wxAuiDefaultDockArt::SetFont(int id, const wxFont& font)
{
    if (id == wxAUI_DOCKART_CAPTION_FONT)
        m_captionFont = font;
}

wxFont wxAuiDefaultDockArt::GetFont(int id)
{
    if (id == wxAUI_DOCKART_CAPTION_FONT)
        return m_captionFont;
    return wxNullFont;
}

void wxAuiDefaultDockArt::DrawSash(wxDC& dc, wxWindow *window, int orientation, const wxRect& rect)
{
#if defined( __WXMAC__ ) && wxOSX_USE_COCOA_OR_CARBON
    wxUnusedVar(window);
    wxUnusedVar(orientation);

    HIRect splitterRect = CGRectMake( rect.x , rect.y , rect.width , rect.height );
    CGContextRef cgContext ;
    wxGCDCImpl *impl = (wxGCDCImpl*) dc.GetImpl();
    cgContext = (CGContextRef) impl->GetGraphicsContext()->GetNativeContext() ;

    HIThemeSplitterDrawInfo drawInfo ;
    drawInfo.version = 0 ;
    drawInfo.state = kThemeStateActive ;
    drawInfo.adornment = kHIThemeSplitterAdornmentNone ;
    HIThemeDrawPaneSplitter( &splitterRect , &drawInfo , cgContext , kHIThemeOrientationNormal ) ;

#elif defined(__WXGTK__)
    // clear out the rectangle first
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(m_sashBrush);
    dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);

#if 0
    GdkRectangle gdk_rect;
    if (orientation == wxVERTICAL )
    {
        gdk_rect.x = rect.x;
        gdk_rect.y = rect.y;
        gdk_rect.width = m_sashSize;
        gdk_rect.height = rect.height;
    }
    else
    {
        gdk_rect.x = rect.x;
        gdk_rect.y = rect.y;
        gdk_rect.width = rect.width;
        gdk_rect.height = m_sashSize;
    }
#endif

    if (!window) return;
    if (!window->m_wxwindow) return;
    if (!gtk_widget_is_drawable(window->m_wxwindow)) return;

#ifdef __WXGTK3__
    cairo_t* cr = static_cast<cairo_t*>(dc.GetGraphicsContext()->GetNativeContext());
    // invert orientation for widget (horizontal GtkPaned has a vertical splitter)
    wxOrientation orient = orientation == wxVERTICAL ? wxHORIZONTAL : wxVERTICAL;
    GtkWidget* widget = wxGTKPrivate::GetSplitterWidget(orient);
    GtkStyleContext* sc = gtk_widget_get_style_context(widget);
    gtk_style_context_save(sc);

    gtk_style_context_add_class(sc, GTK_STYLE_CLASS_PANE_SEPARATOR);
    gtk_render_handle(sc, cr, rect.x, rect.y, rect.width, rect.height);

    gtk_style_context_restore(sc);
#else
    gtk_paint_handle
    (
        gtk_widget_get_style(window->m_wxwindow),
        window->GTKGetDrawingWindow(),
        // flags & wxCONTROL_CURRENT ? GTK_STATE_PRELIGHT : GTK_STATE_NORMAL,
        GTK_STATE_NORMAL,
        GTK_SHADOW_NONE,
        NULL /* no clipping */,
        window->m_wxwindow,
        "paned",
        rect.x,
        rect.y,
        rect.width,
        rect.height,
        (orientation == wxVERTICAL) ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL
    );
#endif // !__WXGTK3__

#else
    wxUnusedVar(window);
    wxUnusedVar(orientation);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(m_sashBrush);
    dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);
#endif
}


void wxAuiDefaultDockArt::DrawBackground(wxDC& dc, wxWindow *WXUNUSED(window), int, const wxRect& rect)
{
    dc.SetPen(*wxTRANSPARENT_PEN);
#ifdef __WXMAC__
    // we have to clear first, otherwise we are drawing a light striped pattern
    // over an already darker striped background
    dc.SetBrush(*wxWHITE_BRUSH) ;
    dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);
#endif
    dc.SetBrush(m_backgroundBrush);
    dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);
}

void wxAuiDefaultDockArt::DrawBorder(wxDC& dc, wxWindow* window, const wxRect& _rect,
                                  wxAuiPaneInfo& pane)
{
    dc.SetPen(m_borderPen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    wxRect rect = _rect;
    int i, border_width = GetMetric(wxAUI_DOCKART_PANE_BORDER_SIZE);

    if (pane.IsToolbar())
    {
        for (i = 0; i < border_width; ++i)
        {
            dc.SetPen(*wxWHITE_PEN);
            dc.DrawLine(rect.x, rect.y, rect.x+rect.width, rect.y);
            dc.DrawLine(rect.x, rect.y, rect.x, rect.y+rect.height);
            dc.SetPen(m_borderPen);
            dc.DrawLine(rect.x, rect.y+rect.height-1,
                        rect.x+rect.width, rect.y+rect.height-1);
            dc.DrawLine(rect.x+rect.width-1, rect.y,
                        rect.x+rect.width-1, rect.y+rect.height);
            rect.Deflate(1);
        }
    }
    else
    {
        // notebooks draw the border themselves, so they can use native rendering (e.g. tabartgtk)
        wxAuiTabArt* art = 0;
        wxAuiNotebook* nb = wxDynamicCast(window, wxAuiNotebook);
        if (nb)
            art = nb->GetArtProvider();

        if (art)
            art->DrawBorder(dc, window, rect);
        else
        {
            for (i = 0; i < border_width; ++i)
            {
                dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);
                rect.Deflate(1);
            }
        }
    }
}


void wxAuiDefaultDockArt::DrawCaptionBackground(wxDC& dc, const wxRect& rect, bool active)
{
    if (m_gradientType == wxAUI_GRADIENT_NONE)
    {
        if (active)
            dc.SetBrush(wxBrush(m_activeCaptionColour));
        else
            dc.SetBrush(wxBrush(m_inactiveCaptionColour));

        dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);
    }
    else
    {
        if (active)
        {
            // on mac the gradients are expected to become darker from the top
#ifdef __WXMAC__
            DrawGradientRectangle(dc, rect,
                                 m_activeCaptionColour,
                                 m_activeCaptionGradientColour,
                                 m_gradientType);
#else
            // on other platforms, active gradients become lighter at the top
            DrawGradientRectangle(dc, rect,
                                 m_activeCaptionGradientColour,
                                 m_activeCaptionColour,
                                 m_gradientType);
#endif
        }
        else
        {
#ifdef __WXMAC__
            // on mac the gradients are expected to become darker from the top
            DrawGradientRectangle(dc, rect,
                                 m_inactiveCaptionGradientColour,
                                 m_inactiveCaptionColour,
                                 m_gradientType);
#else
            // on other platforms, inactive gradients become lighter at the bottom
            DrawGradientRectangle(dc, rect,
                                 m_inactiveCaptionColour,
                                 m_inactiveCaptionGradientColour,
                                 m_gradientType);
#endif
        }
    }
}


void wxAuiDefaultDockArt::DrawCaption(wxDC& dc, wxWindow *WXUNUSED(window),
                                   const wxString& text,
                                   const wxRect& rect,
                                   wxAuiPaneInfo& pane)
{
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetFont(m_captionFont);

    DrawCaptionBackground(dc, rect,
                          (pane.state & wxAuiPaneInfo::optionActive)?true:false);

    int caption_offset = 0;
    if ( pane.icon.IsOk() )
    {
        DrawIcon(dc, rect, pane);

        caption_offset += pane.icon.GetWidth() + 3;
    }

    if (pane.state & wxAuiPaneInfo::optionActive)
        dc.SetTextForeground(m_activeCaptionTextColour);
    else
        dc.SetTextForeground(m_inactiveCaptionTextColour);


    wxCoord w,h;
    dc.GetTextExtent(wxT("ABCDEFHXfgkj"), &w, &h);

    wxRect clip_rect = rect;
    clip_rect.width -= 3; // text offset
    clip_rect.width -= 2; // button padding
    if (pane.HasCloseButton())
        clip_rect.width -= m_buttonSize;
    if (pane.HasPinButton())
        clip_rect.width -= m_buttonSize;
    if (pane.HasMaximizeButton())
        clip_rect.width -= m_buttonSize;

    wxString draw_text = wxAuiChopText(dc, text, clip_rect.width);

    dc.SetClippingRegion(clip_rect);
    dc.DrawText(draw_text, rect.x+3 + caption_offset, rect.y+(rect.height/2)-(h/2)-1);
    dc.DestroyClippingRegion();
}

void
wxAuiDefaultDockArt::DrawIcon(wxDC& dc, const wxRect& rect, wxAuiPaneInfo& pane)
{
   // Draw the icon centered vertically
   dc.DrawBitmap(pane.icon,
                 rect.x+2, rect.y+(rect.height-pane.icon.GetHeight())/2,
                 true);
}

void wxAuiDefaultDockArt::DrawGripper(wxDC& dc, wxWindow *WXUNUSED(window),
                                   const wxRect& rect,
                                   wxAuiPaneInfo& pane)
{
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(m_gripperBrush);

    dc.DrawRectangle(rect.x, rect.y, rect.width,rect.height);

    if (!pane.HasGripperTop())
    {
        int y = 5;
        while (1)
        {
            dc.SetPen(m_gripperPen1);
            dc.DrawPoint(rect.x+3, rect.y+y);
            dc.SetPen(m_gripperPen2);
            dc.DrawPoint(rect.x+3, rect.y+y+1);
            dc.DrawPoint(rect.x+4, rect.y+y);
            dc.SetPen(m_gripperPen3);
            dc.DrawPoint(rect.x+5, rect.y+y+1);
            dc.DrawPoint(rect.x+5, rect.y+y+2);
            dc.DrawPoint(rect.x+4, rect.y+y+2);

            y += 4;
            if (y > rect.GetHeight()-5)
                break;
        }
    }
    else
    {
        int x = 5;
        while (1)
        {
            dc.SetPen(m_gripperPen1);
            dc.DrawPoint(rect.x+x, rect.y+3);
            dc.SetPen(m_gripperPen2);
            dc.DrawPoint(rect.x+x+1, rect.y+3);
            dc.DrawPoint(rect.x+x, rect.y+4);
            dc.SetPen(m_gripperPen3);
            dc.DrawPoint(rect.x+x+1, rect.y+5);
            dc.DrawPoint(rect.x+x+2, rect.y+5);
            dc.DrawPoint(rect.x+x+2, rect.y+4);

            x += 4;
            if (x > rect.GetWidth()-5)
                break;
        }
    }
}

void wxAuiDefaultDockArt::DrawPaneButton(wxDC& dc, wxWindow *window,
                                      int button,
                                      int button_state,
                                      const wxRect& _rect,
                                      wxAuiPaneInfo& pane)
{
    // Create square rect and center it
    wxRect rect(0, 0, _rect.width, _rect.height);
    if (rect.width > rect.height)
        rect.width = rect.height;
    else
        rect.height = rect.width;
    rect = rect.CenterIn(_rect);

    wxColour glyphColour;
    if (pane.state & wxAuiPaneInfo::optionActive)
        glyphColour = m_activeCaptionTextColour;
    else
        glyphColour = m_inactiveCaptionTextColour;

    if (button_state == wxAUI_BUTTON_STATE_HOVER ||
        button_state == wxAUI_BUTTON_STATE_PRESSED)
    {
        // Draw background
        wxColour bgColour = *wxWHITE;
        if (button_state == wxAUI_BUTTON_STATE_PRESSED)
            bgColour = bgColour.ChangeLightness(90);

        dc.SetBrush(wxBrush(bgColour));
        dc.SetPen(wxPen(bgColour.ChangeLightness(70)));

#ifdef __WXMAC__
        dc.DrawEllipse(rect);
#else
        dc.DrawRectangle(rect);
#endif // __WXMAC__

        dc.SetPen(wxPen(glyphColour.ChangeLightness(70)));
    }
    else
    {
        dc.SetPen(wxPen(glyphColour));
    }

    // Draw glyph
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    wxRect gr = rect;
    gr.Deflate(window->FromDIP(3));

    switch (button)
    {
        default:
        case wxAUI_BUTTON_CLOSE:
            dc.DrawLine(gr.x + 1, gr.y + 1, gr.GetRight() + 1, gr.GetBottom() + 1);
            dc.DrawLine(gr.GetRight(), gr.y + 1, gr.x, gr.GetBottom() + 1);
            break;
        case wxAUI_BUTTON_MAXIMIZE_RESTORE:
#ifdef __WXMAC__
            dc.DrawLine(gr.x + gr.width / 2, gr.y, gr.x + gr.width / 2, gr.GetBottom() + 2);
            dc.DrawLine(gr.x, gr.y + gr.height / 2, gr.GetRight() + 2, gr.y + gr.height / 2);
#else
            if (pane.IsMaximized())
            {
                gr.Deflate(1);
                gr.Offset(0, 2);

                wxPoint points[] =
                {
                    wxPoint(2, 0),
                    wxPoint(2, -2),
                    wxPoint(gr.width + 1, -2),
                    wxPoint(gr.width + 1, gr.height - 2),
                    wxPoint(gr.width - 2, gr.height - 2)
                };
                dc.DrawLines(WXSIZEOF(points), points, gr.x, gr.y);
            }

            dc.DrawRectangle(gr);
#endif // __WXMAC__
            break;
        case wxAUI_BUTTON_PIN:
            {
                gr.Inflate(-(gr.width/5), 1);

                dc.DrawLine(gr.x, gr.y + gr.height / 2, gr.GetRight() + 2, gr.y + gr.height / 2);
                dc.DrawLine(gr.x + gr.width / 2, gr.GetBottom(), gr.x + gr.width / 2, gr.y + gr.height / 2);

                wxPoint points[] =
                {
                    wxPoint(gr.x + 1, gr.y + gr.height / 2),
                    wxPoint(gr.x + 1, gr.y),
                    wxPoint(gr.GetRight(), gr.y)
                };
                dc.DrawLines(WXSIZEOF(points), points);

                wxPen pen = dc.GetPen();
                pen.SetWidth(2);
                dc.SetPen(pen);
                dc.DrawLine(gr.GetRight(), gr.y, gr.GetRight(), gr.y + gr.height / 2);
                break;
            }
    }
}


#endif // wxUSE_AUI
