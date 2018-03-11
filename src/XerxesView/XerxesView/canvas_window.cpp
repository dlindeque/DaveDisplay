#include "stdafx.h"
#include "canvas_window.h"

#include "main_window.h"
#include "Resource.h"
#include <assert.h>
#include <string>

#include "messages.h"
#include "application.h"


#include <Shlwapi.h>

namespace xerxes
{
    ATOM canvas_window::_registration = 0;
    HWND canvas_window::_wnd = NULL;
    IMFPMediaPlayer *canvas_window::_player = NULL;
    bool canvas_window::_is_playing = false;

    class MediaPlayerCallback : public IMFPMediaPlayerCallback
    {
        long m_cRef; // Reference count

    public:

        MediaPlayerCallback() : m_cRef(1)
        {
        }

        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
        {
            static const QITAB qit[] =
            {
                QITABENT(MediaPlayerCallback, IMFPMediaPlayerCallback),
                { 0 },
            };
            return QISearch(this, qit, riid, ppv);
        }

        IFACEMETHODIMP_(ULONG) AddRef()
        {
            return InterlockedIncrement(&m_cRef);
        }

        IFACEMETHODIMP_(ULONG) Release()
        {
            ULONG count = InterlockedDecrement(&m_cRef);
            if (count == 0)
            {
                delete this;
                return 0;
            }
            return count;
        }

        // IMFPMediaPlayerCallback methods
        IFACEMETHODIMP_(void) OnMediaPlayerEvent(MFP_EVENT_HEADER *pEventHeader);
    };

    void MediaPlayerCallback::OnMediaPlayerEvent(MFP_EVENT_HEADER * pEventHeader)
    {
        if (FAILED(pEventHeader->hrEvent))
        {
            //ShowErrorMessage(L"Playback error", pEventHeader->hrEvent);
            return;
        }

        switch (pEventHeader->eEventType)
        {
        case MFP_EVENT_TYPE_MEDIAITEM_CREATED:
            {
                auto pEvent = MFP_GET_MEDIAITEM_CREATED_EVENT(pEventHeader);
                if (canvas_window::_player != NULL) {
                    BOOL    bHasVideo = FALSE;
                    BOOL    bIsSelected = FALSE;

                    // Check if the media item contains video.
                    HRESULT hr = pEvent->pMediaItem->HasVideo(&bHasVideo, &bIsSelected);
                    if (SUCCEEDED(hr))
                    {
                        canvas_window::_is_playing = bHasVideo && bIsSelected;

                        // Set the media item on the player. This method completes
                        // asynchronously.
                        hr = canvas_window::_player->SetMediaItem(pEvent->pMediaItem);
                    }

                    //if (FAILED(hr))
                    //{
                    //    ShowErrorMessage(L"Error playing this file.", hr);
                    //}
                }
            }
            break;

        case MFP_EVENT_TYPE_MEDIAITEM_SET:
            {
                auto pEvent = MFP_GET_MEDIAITEM_SET_EVENT(pEventHeader);
                if (canvas_window::_player != NULL) {
                    canvas_window::_player->Play();
                }
            }
            break;
        }
    }

    template <class T> void SafeRelease(T **ppT)
    {
        if (*ppT)
        {
            (*ppT)->Release();
            *ppT = NULL;
        }
    }

    LRESULT CALLBACK canvas_window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message) {
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);

                if (_player != NULL && _is_playing) {
                    _player->UpdateVideo();
                }
                else {
                    FillRect(hdc, &ps.rcPaint, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
                }

                // TODO: Add any drawing code that uses hdc here...
                SelectObject(hdc, GetStockObject(WHITE_PEN));
                RECT rect;
                GetWindowRect(_wnd, &rect);
                MoveToEx(hdc, 0, 0, NULL);
                LineTo(hdc, rect.right - rect.left, rect.bottom - rect.top);
                MoveToEx(hdc, 0, rect.bottom - rect.top, NULL);
                LineTo(hdc, rect.right-rect.left, 0);
                EndPaint(hWnd, &ps);
            }
            break;
        case WM_DESTROY:
            SafeRelease(&_player);
            PostMessage(main_window::get_wnd(), WM_USER_CANVAS_WINDOW_CLOSED, 0, 0);
            _wnd = NULL;
            break;
        case WM_SIZE:
            if (_player != NULL && wParam == SIZE_RESTORED) {
                _player->UpdateVideo();
            }
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }

    auto canvas_window::try_register_class() -> bool
    {
        assert(_registration == 0);

        WNDCLASSEXW wcex;

        wcex.cbSize = sizeof(WNDCLASSEX);

        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = application::instance();
        wcex.hIcon = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_XERXESVIEW));
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)); // (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = CANVAS_WINDOW_CLASS_NAME;
        wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

        _registration = RegisterClassExW(&wcex);

        return _registration != 0;
    }

    auto canvas_window::try_create_window(int x, int y, int width, int height, bool fullscreen) -> bool
    {
        assert(_wnd == NULL);
        WCHAR initial_title[MAX_INITIAL_TITLE_LENGTH + 1];
        if (LoadStringW(application::instance(), IDS_APP_TITLE, initial_title, MAX_INITIAL_TITLE_LENGTH) == 0) return false;
        std::wstring title(initial_title);
        title.append(L" - Show");
        if (fullscreen) {
            _wnd = CreateWindowW(CANVAS_WINDOW_CLASS_NAME, title.c_str(), WS_POPUP,
                x, y, width, height, nullptr, nullptr, application::instance(), nullptr);
        }
        else {
            _wnd = CreateWindowW(CANVAS_WINDOW_CLASS_NAME, title.c_str(), WS_OVERLAPPEDWINDOW,
                x, y, width, height, nullptr, nullptr, application::instance(), nullptr);
        }
        if (_player == NULL) {
            auto hr = MFPCreateMediaPlayer(NULL, false, 0, new (std::nothrow)MediaPlayerCallback(), _wnd, &_player);
            if (SUCCEEDED(hr)) {
                _player->CreateMediaItemFromURL(L"C:\\Users\\dawie\\Videos\\Ian & Cosmo.mp4", false, 0, NULL);
            }
        }

        return _wnd != 0;
    }

    auto canvas_window::try_show(int x, int y, int width, int height, int nCmdShow, bool fullscreen, bool update_immediately) -> bool
    {
        assert(application::instance() != NULL);
        if (_registration == 0) {
            if (!try_register_class()) return false;
        }
        if (_wnd == NULL) {
            if (!try_create_window(x, y, width, height, fullscreen)) return false;
        }
        if (fullscreen) {
            ShowWindow(_wnd, SW_SHOW);
        }
        else {
            ShowWindow(_wnd, nCmdShow);
        }

        if (update_immediately) {
            UpdateWindow(_wnd);
        }

        return true;
    }

    auto canvas_window::close() -> void
    {
        DestroyWindow(_wnd);
    }

}
