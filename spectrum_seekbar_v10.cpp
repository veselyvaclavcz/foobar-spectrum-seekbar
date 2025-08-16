// Spectrum Seekbar V10 - 4 styles (lines, bars, blocks, dots) with mono/stereo modes
#define FOOBAR2000_TARGET_VERSION 80

#include "backup/foo_seekbar/SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include <windows.h>
#include <windowsx.h>
#include <math.h>

DECLARE_COMPONENT_VERSION(
    "Spectrum Seekbar V10",
    "10.0.0",
    "4 visualization styles with mono/stereo modes"
);

VALIDATE_COMPONENT_FILENAME("foo_spectrum_seekbar_v10.dll");

class spectrum_seekbar_v10 : public ui_element_instance, private play_callback_impl_base {
private:
    HWND m_hwnd;
    ui_element_instance_callback::ptr m_callback;
    
    // Visualization
    visualisation_stream::ptr m_vis_stream;
    
    // Spectrum data
    static const int NUM_BARS = 32;
    float m_bars[NUM_BARS];
    float m_peaks[NUM_BARS];
    float m_bars_left[NUM_BARS];
    float m_bars_right[NUM_BARS];
    
    // Timer
    UINT_PTR m_timer;
    
    // Colors
    COLORREF m_clr_background;
    COLORREF m_clr_bar;
    COLORREF m_clr_played;
    COLORREF m_clr_position;
    
    // Playback state
    bool m_is_playing;
    double m_track_length;
    double m_playback_position;
    metadb_handle_ptr m_current_track;
    
    // Seeking
    bool m_seeking;
    
    // Cleanup tracking
    bool m_callbacks_registered;
    
    // Display modes
    enum VisualizationStyle {
        STYLE_LINES = 0,
        STYLE_BARS = 1,
        STYLE_BLOCKS = 2,
        STYLE_DOTS = 3,
        STYLE_COUNT = 4
    };
    
    enum ChannelMode {
        CHANNEL_MONO = 0,
        CHANNEL_STEREO = 1,
        CHANNEL_COUNT = 2
    };
    
    int m_visualization_style;
    int m_channel_mode;
    
public:
    spectrum_seekbar_v10(ui_element_config::ptr config, ui_element_instance_callback::ptr callback) 
        : m_callback(callback), m_hwnd(NULL), m_timer(0), m_is_playing(false),
          m_track_length(0), m_playback_position(0), m_seeking(false), 
          m_callbacks_registered(false),
          m_visualization_style(STYLE_BARS), m_channel_mode(CHANNEL_MONO) {
        
        memset(m_bars, 0, sizeof(m_bars));
        memset(m_peaks, 0, sizeof(m_peaks));
        memset(m_bars_left, 0, sizeof(m_bars_left));
        memset(m_bars_right, 0, sizeof(m_bars_right));
        
        // Load configuration if available
        if (config.is_valid() && config->get_data_size() >= 8) {
            const t_uint8* data = (const t_uint8*)config->get_data();
            m_visualization_style = *(int*)data;
            m_channel_mode = *(int*)(data + 4);
            
            // Validate loaded values
            if (m_visualization_style < 0 || m_visualization_style >= STYLE_COUNT)
                m_visualization_style = STYLE_BARS;
            if (m_channel_mode < 0 || m_channel_mode >= CHANNEL_COUNT)
                m_channel_mode = CHANNEL_MONO;
        }
    }
    
    ~spectrum_seekbar_v10() {
        // Clean up timer if window still exists
        if (m_timer && m_hwnd && IsWindow(m_hwnd)) {
            KillTimer(m_hwnd, m_timer);
            m_timer = 0;
        }
        
        // Unregister playback callback
        if (m_callbacks_registered) {
            try {
                static_api_ptr_t<play_callback_manager>()->unregister_callback(this);
                m_callbacks_registered = false;
            } catch(...) {}
        }
        
        // Release visualization stream
        if (m_vis_stream.is_valid()) {
            m_vis_stream.release();
        }
        
        // Destroy window if it still exists
        if (m_hwnd && IsWindow(m_hwnd)) {
            DestroyWindow(m_hwnd);
            m_hwnd = NULL;
        }
    }
    
    void initialize_window(HWND parent) {
        // Create window
        m_hwnd = CreateWindowEx(
            0, L"Static", L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            0, 0, 100, 100,
            parent, NULL, core_api::get_my_instance(), NULL
        );
        
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
        SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);
        
        // Get colors
        m_clr_background = m_callback->query_std_color(ui_color_background);
        m_clr_bar = m_callback->query_std_color(ui_color_text);
        m_clr_played = m_callback->query_std_color(ui_color_selection);
        m_clr_position = m_callback->query_std_color(ui_color_highlight);
        
        // Create visualization stream
        try {
            static_api_ptr_t<visualisation_manager> vis_manager;
            vis_manager->create_stream(m_vis_stream, visualisation_manager::KStreamFlagNewFFT);
        } catch(...) {}
        
        // Register for playback callbacks
        static_api_ptr_t<play_callback_manager>()->register_callback(
            this,
            play_callback::flag_on_playback_all | play_callback::flag_on_playback_time,
            false
        );
        m_callbacks_registered = true;
        
        // Get initial state
        update_playback_state();
        
        // Start timer
        m_timer = SetTimer(m_hwnd, 1, 16, TimerProc);
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
    }
    
    void update_playback_state() {
        static_api_ptr_t<playback_control> pc;
        m_is_playing = pc->is_playing() && !pc->is_paused();
        
        if (m_is_playing) {
            // Method 1: Try to get length from playback control
            m_track_length = pc->playback_get_length();
            
            // Method 2: If that fails, get from current track metadata
            if (m_track_length <= 0) {
                metadb_handle_ptr track;
                if (pc->get_now_playing(track)) {
                    m_current_track = track;
                    
                    file_info_impl info;
                    if (track->get_info(info)) {
                        m_track_length = info.get_length();
                    }
                    
                    // Method 3: Try alternate method
                    if (m_track_length <= 0) {
                        m_track_length = track->get_length();
                    }
                }
            }
            
            // Get current position
            m_playback_position = pc->playback_get_position();
        } else {
            m_track_length = 0;
            m_playback_position = 0;
            m_current_track.release();
        }
    }
    
    HWND get_wnd() { return m_hwnd; }
    
    void set_configuration(ui_element_config::ptr config) {
        // Load configuration if available
        if (config.is_valid() && config->get_data_size() >= 8) {
            const t_uint8* data = (const t_uint8*)config->get_data();
            m_visualization_style = *(int*)data;
            m_channel_mode = *(int*)(data + 4);
            
            // Validate loaded values
            if (m_visualization_style < 0 || m_visualization_style >= STYLE_COUNT)
                m_visualization_style = STYLE_BARS;
            if (m_channel_mode < 0 || m_channel_mode >= CHANNEL_COUNT)
                m_channel_mode = CHANNEL_MONO;
                
            // Refresh display
            if (m_hwnd) InvalidateRect(m_hwnd, NULL, FALSE);
        }
    }
    
    ui_element_config::ptr get_configuration() { 
        // Save current settings
        ui_element_config_builder builder;
        builder << m_visualization_style;
        builder << m_channel_mode;
        return builder.finish(g_get_guid());
    }
    
    GUID get_guid() { return g_get_guid(); }
    GUID get_subclass() { return g_get_subclass(); }
    
    static GUID g_get_guid() {
        // {C1B2A3D4-E5F6-7890-ABCD-EF1234567890}
        static const GUID guid = 
        { 0xc1b2a3d4, 0xe5f6, 0x7890, { 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90 } };
        return guid;
    }
    
    static void g_get_name(pfc::string_base & out) { out = "Spectrum Seekbar V10"; }
    static ui_element_config::ptr g_get_default_configuration() { return ui_element_config::g_create_empty(g_get_guid()); }
    static const char * g_get_description() { return "4 visualization styles with mono/stereo modes"; }
    static GUID g_get_subclass() { return ui_element_subclass_playback_visualisation; }
    
    void show_menu(POINT pt) {
        HMENU menu = CreatePopupMenu();
        HMENU styleMenu = CreatePopupMenu();
        HMENU channelMenu = CreatePopupMenu();
        
        // Style submenu
        AppendMenu(styleMenu, MF_STRING | (m_visualization_style == STYLE_LINES ? MF_CHECKED : 0), 1001, L"Lines");
        AppendMenu(styleMenu, MF_STRING | (m_visualization_style == STYLE_BARS ? MF_CHECKED : 0), 1002, L"Bars");
        AppendMenu(styleMenu, MF_STRING | (m_visualization_style == STYLE_BLOCKS ? MF_CHECKED : 0), 1003, L"Blocks");
        AppendMenu(styleMenu, MF_STRING | (m_visualization_style == STYLE_DOTS ? MF_CHECKED : 0), 1004, L"Dots");
        
        // Channel submenu
        AppendMenu(channelMenu, MF_STRING | (m_channel_mode == CHANNEL_MONO ? MF_CHECKED : 0), 2001, L"Mixed (Mono)");
        AppendMenu(channelMenu, MF_STRING | (m_channel_mode == CHANNEL_STEREO ? MF_CHECKED : 0), 2002, L"Stereo (Mirrored)");
        
        // Main menu
        AppendMenu(menu, MF_POPUP, (UINT_PTR)styleMenu, L"Visualization Style");
        AppendMenu(menu, MF_POPUP, (UINT_PTR)channelMenu, L"Channel Mode");
        
        int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTBUTTON, pt.x, pt.y, 0, m_hwnd, NULL);
        
        if (cmd >= 1001 && cmd <= 1004) {
            m_visualization_style = cmd - 1001;
            InvalidateRect(m_hwnd, NULL, FALSE);
            // Save configuration
            m_callback->on_min_max_info_change();
        } else if (cmd >= 2001 && cmd <= 2002) {
            m_channel_mode = cmd - 2001;
            InvalidateRect(m_hwnd, NULL, FALSE);
            // Save configuration
            m_callback->on_min_max_info_change();
        }
        
        DestroyMenu(channelMenu);
        DestroyMenu(styleMenu);
        DestroyMenu(menu);
    }
    
    static VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
        spectrum_seekbar_v10* p_this = reinterpret_cast<spectrum_seekbar_v10*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (p_this) {
            p_this->update_spectrum();
            InvalidateRect(hwnd, NULL, FALSE);
        }
    }
    
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        spectrum_seekbar_v10* p_this = reinterpret_cast<spectrum_seekbar_v10*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (!p_this) return DefWindowProc(hwnd, msg, wParam, lParam);
        
        switch(msg) {
            case WM_PAINT:
                p_this->on_paint();
                return 0;
                
            case WM_ERASEBKGND:
                return 1;
                
            case WM_LBUTTONDOWN:
                p_this->on_lbutton_down(GET_X_LPARAM(lParam));
                return 0;
                
            case WM_LBUTTONUP:
                if (p_this->m_seeking) {
                    p_this->m_seeking = false;
                    ReleaseCapture();
                }
                return 0;
                
            case WM_MOUSEMOVE:
                if (p_this->m_seeking) {
                    p_this->on_lbutton_down(GET_X_LPARAM(lParam));
                }
                return 0;
                
            case WM_RBUTTONUP:
                {
                    // Check if layout editing mode is active
                    if (p_this->m_callback->is_edit_mode_enabled()) {
                        // In edit mode, don't handle - let parent window handle it
                        break;  // Fall through to DefWindowProc
                    } else {
                        // Show custom visualization menu
                        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                        ClientToScreen(hwnd, &pt);
                        p_this->show_menu(pt);
                        return 0;
                    }
                }
                
            case WM_DESTROY:
                if (p_this->m_timer) {
                    KillTimer(hwnd, p_this->m_timer);
                    p_this->m_timer = 0;
                }
                if (p_this->m_callbacks_registered) {
                    try {
                        static_api_ptr_t<play_callback_manager>()->unregister_callback(p_this);
                        p_this->m_callbacks_registered = false;
                    } catch(...) {}
                }
                // Clear the window pointer to prevent double-cleanup
                p_this->m_hwnd = NULL;
                return 0;
        }
        
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    void on_lbutton_down(int x) {
        if (!m_is_playing || m_track_length <= 0) return;
        
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        m_seeking = true;
        SetCapture(m_hwnd);
        
        // Calculate seek position
        double ratio = (double)x / rc.right;
        if (ratio < 0) ratio = 0;
        if (ratio > 1) ratio = 1;
        
        double seek_position = ratio * m_track_length;
        
        // Perform seek
        static_api_ptr_t<playback_control> pc;
        pc->playback_seek(seek_position);
        
        m_playback_position = seek_position;
    }
    
    void update_spectrum() {
        if (!m_vis_stream.is_valid()) {
            for (int i = 0; i < NUM_BARS; i++) {
                m_bars[i] *= 0.9f;
                if (m_bars[i] < 0.01f) m_bars[i] = 0;
            }
            return;
        }
        
        double time = 0;
        if (!m_vis_stream->get_absolute_time(time)) {
            for (int i = 0; i < NUM_BARS; i++) {
                m_bars[i] *= 0.9f;
                if (m_bars[i] < 0.01f) m_bars[i] = 0;
            }
            return;
        }
        
        audio_chunk_impl chunk;
        if (m_vis_stream->get_spectrum_absolute(chunk, time, 1024)) {
            process_spectrum(chunk);
        } else {
            m_vis_stream->make_fake_spectrum_absolute(chunk, time, 1024);
            if (chunk.get_sample_count() > 0) {
                process_spectrum(chunk);
            }
        }
    }
    
    void process_spectrum(const audio_chunk & chunk) {
        unsigned samples = chunk.get_sample_count();
        unsigned channels = chunk.get_channels();
        
        if (samples == 0 || channels == 0) return;
        
        const audio_sample * data = chunk.get_data();
        
        for (int bar = 0; bar < NUM_BARS; bar++) {
            float freq_start = powf(2.0f, (float)bar / 4.0f);
            float freq_end = powf(2.0f, (float)(bar + 1) / 4.0f);
            
            int bin_start = (int)(freq_start * samples / 512.0f);
            int bin_end = (int)(freq_end * samples / 512.0f);
            
            if (bin_start >= (int)samples) bin_start = samples - 1;
            if (bin_end > (int)samples) bin_end = samples;
            if (bin_start >= bin_end) bin_end = bin_start + 1;
            
            float sum = 0;
            int count = 0;
            
            float sum_left = 0, sum_right = 0;
            
            for (int i = bin_start; i < bin_end && i < (int)samples; i++) {
                if (channels == 1) {
                    float val = data[i];
                    sum += val * val;
                    sum_left += val * val;
                    sum_right += val * val;
                } else if (channels >= 2) {
                    float val_left = data[i * channels];
                    float val_right = data[i * channels + 1];
                    sum += (val_left * val_left + val_right * val_right) / 2;
                    sum_left += val_left * val_left;
                    sum_right += val_right * val_right;
                }
                count++;
            }
            
            if (count > 0) {
                // Combined channel
                float magnitude = sqrtf(sum / count);
                float db = 20.0f * log10f(magnitude + 1e-10f);
                float normalized = (db + 60.0f) / 60.0f;
                if (normalized < 0) normalized = 0;
                if (normalized > 1) normalized = 1;
                
                float target = normalized;
                if (target > m_bars[bar]) {
                    m_bars[bar] = m_bars[bar] + (target - m_bars[bar]) * 0.5f;
                } else {
                    m_bars[bar] = m_bars[bar] * 0.9f;
                }
                
                // Left channel
                float magnitude_left = sqrtf(sum_left / count);
                float db_left = 20.0f * log10f(magnitude_left + 1e-10f);
                float normalized_left = (db_left + 60.0f) / 60.0f;
                if (normalized_left < 0) normalized_left = 0;
                if (normalized_left > 1) normalized_left = 1;
                
                if (normalized_left > m_bars_left[bar]) {
                    m_bars_left[bar] = m_bars_left[bar] + (normalized_left - m_bars_left[bar]) * 0.5f;
                } else {
                    m_bars_left[bar] = m_bars_left[bar] * 0.9f;
                }
                
                // Right channel
                float magnitude_right = sqrtf(sum_right / count);
                float db_right = 20.0f * log10f(magnitude_right + 1e-10f);
                float normalized_right = (db_right + 60.0f) / 60.0f;
                if (normalized_right < 0) normalized_right = 0;
                if (normalized_right > 1) normalized_right = 1;
                
                if (normalized_right > m_bars_right[bar]) {
                    m_bars_right[bar] = m_bars_right[bar] + (normalized_right - m_bars_right[bar]) * 0.5f;
                } else {
                    m_bars_right[bar] = m_bars_right[bar] * 0.9f;
                }
                
                if (m_bars[bar] > m_peaks[bar]) {
                    m_peaks[bar] = m_bars[bar];
                } else {
                    m_peaks[bar] *= 0.98f;
                }
            }
        }
    }
    
    void draw_lines(HDC hdc, const RECT& rc, float* bars, COLORREF color) {
        if (NUM_BARS < 2) return;
        
        HPEN linePen = CreatePen(PS_SOLID, 2, color);
        HPEN oldPen = (HPEN)SelectObject(hdc, linePen);
        
        int bar_width = rc.right / NUM_BARS;
        int first_x = bar_width / 2;
        int first_y = rc.bottom - (int)(bars[0] * rc.bottom * 0.9f);
        MoveToEx(hdc, first_x, first_y, NULL);
        
        for (int i = 1; i < NUM_BARS; i++) {
            int x = i * bar_width + bar_width / 2;
            int y = rc.bottom - (int)(bars[i] * rc.bottom * 0.9f);
            LineTo(hdc, x, y);
        }
        
        SelectObject(hdc, oldPen);
        DeleteObject(linePen);
    }
    
    void draw_bars(HDC hdc, const RECT& rc, float* bars, COLORREF color) {
        int bar_width = rc.right / NUM_BARS;
        HBRUSH barBrush = CreateSolidBrush(color);
        
        for (int i = 0; i < NUM_BARS; i++) {
            int x = i * bar_width;
            int bar_height = (int)(bars[i] * rc.bottom * 0.9f);
            int y = rc.bottom - bar_height;
            
            RECT barRect = {x + 1, y, x + bar_width - 1, rc.bottom};
            FillRect(hdc, &barRect, barBrush);
        }
        
        DeleteObject(barBrush);
    }
    
    void draw_blocks(HDC hdc, const RECT& rc, float* bars, COLORREF color) {
        int bar_width = rc.right / NUM_BARS;
        HBRUSH blockBrush = CreateSolidBrush(color);
        
        for (int i = 0; i < NUM_BARS; i++) {
            int x = i * bar_width;
            int bar_height = (int)(bars[i] * rc.bottom * 0.9f);
            int num_blocks = (bar_height / 8) + 1;
            
            for (int j = 0; j < num_blocks; j++) {
                int block_y = rc.bottom - (j + 1) * 8;
                if (block_y < rc.bottom - bar_height) break;
                
                RECT blockRect = {x + 2, block_y - 6, x + bar_width - 2, block_y - 2};
                FillRect(hdc, &blockRect, blockBrush);
            }
        }
        
        DeleteObject(blockBrush);
    }
    
    void draw_dots(HDC hdc, const RECT& rc, float* bars, COLORREF color) {
        int bar_width = rc.right / NUM_BARS;
        HBRUSH dotBrush = CreateSolidBrush(color);
        
        for (int i = 0; i < NUM_BARS; i++) {
            int x = i * bar_width + bar_width / 2;
            int y = rc.bottom - (int)(bars[i] * rc.bottom * 0.9f);
            
            RECT dotRect = {x - 3, y - 3, x + 3, y + 3};
            FillRect(hdc, &dotRect, dotBrush);
        }
        
        DeleteObject(dotBrush);
    }
    
    void draw_stereo_mirrored(HDC hdc, const RECT& rc) {
        int center_y = rc.bottom / 2;
        RECT top_rc = {rc.left, rc.top, rc.right, center_y};
        RECT bottom_rc = {rc.left, center_y, rc.right, rc.bottom};
        
        // Draw top half (left channel)
        switch(m_visualization_style) {
            case STYLE_LINES:
                draw_lines(hdc, top_rc, m_bars_left, m_clr_bar);
                break;
            case STYLE_BARS:
                draw_bars(hdc, top_rc, m_bars_left, m_clr_bar);
                break;
            case STYLE_BLOCKS:
                draw_blocks(hdc, top_rc, m_bars_left, m_clr_bar);
                break;
            case STYLE_DOTS:
                draw_dots(hdc, top_rc, m_bars_left, m_clr_bar);
                break;
        }
        
        // Draw bottom half (right channel) - flip the rect coordinate system
        COLORREF right_color = RGB(
            GetRValue(m_clr_bar) * 3 / 4,
            GetGValue(m_clr_bar) * 3 / 4,
            GetBValue(m_clr_bar) * 3 / 4
        );
        
        // For bottom half, we need to draw from center downward
        switch(m_visualization_style) {
            case STYLE_LINES:
                {
                    if (NUM_BARS >= 2) {
                        HPEN linePen = CreatePen(PS_SOLID, 2, right_color);
                        HPEN oldPen = (HPEN)SelectObject(hdc, linePen);
                        
                        int bar_width = rc.right / NUM_BARS;
                        int first_x = bar_width / 2;
                        int first_y = center_y + (int)(m_bars_right[0] * center_y * 0.8f);
                        MoveToEx(hdc, first_x, first_y, NULL);
                        
                        for (int i = 1; i < NUM_BARS; i++) {
                            int x = i * bar_width + bar_width / 2;
                            int y = center_y + (int)(m_bars_right[i] * center_y * 0.8f);
                            LineTo(hdc, x, y);
                        }
                        
                        SelectObject(hdc, oldPen);
                        DeleteObject(linePen);
                    }
                }
                break;
            case STYLE_BARS:
                {
                    int bar_width = rc.right / NUM_BARS;
                    HBRUSH barBrush = CreateSolidBrush(right_color);
                    
                    for (int i = 0; i < NUM_BARS; i++) {
                        int x = i * bar_width;
                        int bar_height = (int)(m_bars_right[i] * center_y * 0.8f);
                        
                        RECT barRect = {x + 1, center_y, x + bar_width - 1, center_y + bar_height};
                        FillRect(hdc, &barRect, barBrush);
                    }
                    
                    DeleteObject(barBrush);
                }
                break;
            case STYLE_BLOCKS:
                {
                    int bar_width = rc.right / NUM_BARS;
                    HBRUSH blockBrush = CreateSolidBrush(right_color);
                    
                    for (int i = 0; i < NUM_BARS; i++) {
                        int x = i * bar_width;
                        int bar_height = (int)(m_bars_right[i] * center_y * 0.8f);
                        int num_blocks = (bar_height / 8) + 1;
                        
                        for (int j = 0; j < num_blocks; j++) {
                            int block_y = center_y + j * 8;
                            if (block_y > center_y + bar_height) break;
                            
                            RECT blockRect = {x + 2, block_y + 2, x + bar_width - 2, block_y + 6};
                            FillRect(hdc, &blockRect, blockBrush);
                        }
                    }
                    
                    DeleteObject(blockBrush);
                }
                break;
            case STYLE_DOTS:
                {
                    int bar_width = rc.right / NUM_BARS;
                    HBRUSH dotBrush = CreateSolidBrush(right_color);
                    
                    for (int i = 0; i < NUM_BARS; i++) {
                        int x = i * bar_width + bar_width / 2;
                        int y = center_y + (int)(m_bars_right[i] * center_y * 0.8f);
                        
                        RECT dotRect = {x - 3, y - 3, x + 3, y + 3};
                        FillRect(hdc, &dotRect, dotBrush);
                    }
                    
                    DeleteObject(dotBrush);
                }
                break;
        }
        
        // Draw center line
        HPEN centerPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HPEN oldPen = (HPEN)SelectObject(hdc, centerPen);
        MoveToEx(hdc, 0, center_y, NULL);
        LineTo(hdc, rc.right, center_y);
        SelectObject(hdc, oldPen);
        DeleteObject(centerPen);
    }
    
    void on_paint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);
        
        // Background
        HBRUSH bgBrush = CreateSolidBrush(m_clr_background);
        FillRect(memDC, &rc, bgBrush);
        DeleteObject(bgBrush);
        
        // Draw spectrum based on mode
        if (m_channel_mode == CHANNEL_MONO) {
            // Mono visualization
            switch(m_visualization_style) {
                case STYLE_LINES:
                    draw_lines(memDC, rc, m_bars, m_clr_bar);
                    break;
                case STYLE_BARS:
                    draw_bars(memDC, rc, m_bars, m_clr_bar);
                    break;
                case STYLE_BLOCKS:
                    draw_blocks(memDC, rc, m_bars, m_clr_bar);
                    break;
                case STYLE_DOTS:
                    draw_dots(memDC, rc, m_bars, m_clr_bar);
                    break;
            }
        } else {
            // Stereo mirrored
            draw_stereo_mirrored(memDC, rc);
        }
        
        // Draw seekbar overlay
        if (m_is_playing && m_track_length > 0) {
            int pos_x = (int)((m_playback_position / m_track_length) * rc.right);
            
            // Vertical position line
            HPEN posPen = CreatePen(PS_SOLID, 3, m_clr_position);
            HPEN oldPen = (HPEN)SelectObject(memDC, posPen);
            MoveToEx(memDC, pos_x, 0, NULL);
            LineTo(memDC, pos_x, rc.bottom);
            SelectObject(memDC, oldPen);
            DeleteObject(posPen);
            
            // Bottom progress bar
            HPEN progPen = CreatePen(PS_SOLID, 4, m_clr_played);
            oldPen = (HPEN)SelectObject(memDC, progPen);
            MoveToEx(memDC, 0, rc.bottom - 2, NULL);
            LineTo(memDC, pos_x, rc.bottom - 2);
            SelectObject(memDC, oldPen);
            DeleteObject(progPen);
            
            // Time text
            WCHAR time_str[64];
            int cur_min = (int)(m_playback_position / 60);
            int cur_sec = (int)m_playback_position % 60;
            int tot_min = (int)(m_track_length / 60);
            int tot_sec = (int)m_track_length % 60;
            swprintf_s(time_str, L"%d:%02d / %d:%02d", cur_min, cur_sec, tot_min, tot_sec);
            
            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, m_clr_position);
            
            RECT text_rc = {10, 10, 200, 30};
            DrawText(memDC, time_str, -1, &text_rc, DT_LEFT);
            
            // Mode and style indicator
            const WCHAR* style_names[] = {L"Lines", L"Bars", L"Blocks", L"Dots"};
            const WCHAR* channel_names[] = {L"Mono", L"Stereo"};
            
            WCHAR mode_str[64];
            swprintf_s(mode_str, L"%s | %s", style_names[m_visualization_style], channel_names[m_channel_mode]);
            
            RECT mode_rc = {rc.right - 150, 10, rc.right - 10, 30};
            DrawText(memDC, mode_str, -1, &mode_rc, DT_RIGHT);
        }
        
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
        
        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        
        EndPaint(m_hwnd, &ps);
    }
    
    // Playback callbacks
    void on_playback_starting(play_control::t_track_command p_command, bool p_paused) override {
        update_playback_state();
    }
    
    void on_playback_new_track(metadb_handle_ptr p_track) override {
        m_current_track = p_track;
        update_playback_state();
    }
    
    void on_playback_stop(play_control::t_stop_reason p_reason) override {
        m_is_playing = false;
        m_track_length = 0;
        m_playback_position = 0;
        m_current_track.release();
    }
    
    void on_playback_pause(bool p_state) override {
        m_is_playing = !p_state;
    }
    
    void on_playback_time(double p_time) override {
        m_playback_position = p_time;
        
        // If we still don't have track length, try to get it again
        if (m_track_length <= 0 && m_is_playing) {
            update_playback_state();
        }
    }
    
    void on_playback_seek(double p_time) override {
        m_playback_position = p_time;
    }
};

// UI element factory
class ui_element_spectrum_seekbar_v10 : public ui_element {
public:
    GUID get_guid() { return spectrum_seekbar_v10::g_get_guid(); }
    void get_name(pfc::string_base & out) { spectrum_seekbar_v10::g_get_name(out); }
    ui_element_config::ptr get_default_configuration() { return spectrum_seekbar_v10::g_get_default_configuration(); }
    const char * get_description() { return spectrum_seekbar_v10::g_get_description(); }
    GUID get_subclass() { return spectrum_seekbar_v10::g_get_subclass(); }
    
    ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr) {
        return NULL;
    }
    
    ui_element_instance::ptr instantiate(HWND parent, ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback) {
        PFC_ASSERT(cfg->get_guid() == get_guid());
        service_nnptr_t<spectrum_seekbar_v10> instance = new service_impl_t<spectrum_seekbar_v10>(cfg, callback);
        instance->initialize_window(parent);
        return instance;
    }
};

static service_factory_single_t<ui_element_spectrum_seekbar_v10> g_spectrum_seekbar_v10_factory;