// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved


#include "MainWindow.h"
#include "ManagerEventHandler.h"
#include "UIAnimationSample.h"
#include <string>
#include <sstream>

const DOUBLE COLOR_MIN = 0.0;
const DOUBLE COLOR_MAX = 1.0;

CMainWindow::CMainWindow() :
    m_hwnd(NULL),
    m_pD2DFactory(NULL),
    m_pRenderTarget(NULL),
    m_pBackgroundBrush(NULL),
    m_pAnimationManager(NULL),
    m_pAnimationTimer(NULL),
    m_pTransitionLibrary(NULL),
    m_pAnimationVariableRed(NULL)
{
}

CMainWindow::~CMainWindow()
{
    // Animated Variables

    SafeRelease(&m_pAnimationVariableRed);

    // Animation

    SafeRelease(&m_pAnimationManager);
    SafeRelease(&m_pAnimationTimer);
    SafeRelease(&m_pTransitionLibrary);

    // D2D

    SafeRelease(&m_pD2DFactory);
    SafeRelease(&m_pRenderTarget);
    SafeRelease(&m_pBackgroundBrush);
}

// Creates the CMainWindow window and initializes 
// device-independent resources

HRESULT CMainWindow::Initialize(
    HINSTANCE hInstance
    )
{
    // Client area dimensions, in inches
    const FLOAT CLIENT_WIDTH = 7.0f;
    const FLOAT CLIENT_HEIGHT = 5.0f; 

    HRESULT hr = CreateDeviceIndependentResources();
    if (SUCCEEDED(hr))
    {
        // Register the window class

        WNDCLASSEX wcex = {sizeof(wcex)};
        wcex.style         = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc   = CMainWindow::WndProc;
        wcex.cbWndExtra    = sizeof(LONG_PTR);
        wcex.hInstance     = hInstance;
        wcex.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
        wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wcex.lpszClassName = L"MyWindowClass";

        RegisterClassEx(&wcex);

        // Create the CMainWindow window
        m_hwnd = CreateWindow(
            wcex.lpszClassName,
            L"Windows Animation Manager demo",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            600,
            600,
            NULL,
            NULL,
            hInstance,
            this
            );

        hr = m_hwnd ? S_OK : E_FAIL;
        if (SUCCEEDED(hr))
        {
            // Initialize Animation

            hr = InitializeAnimation();
            if (SUCCEEDED(hr))
            {
                hr = CreateAnimationVariables();
                if (SUCCEEDED(hr))
                {
                    // Display the window
                    ShowWindow(m_hwnd, SW_SHOWNORMAL);
                    UpdateWindow(m_hwnd);

                    // Fade in with Red
                    hr = ChangeColor(COLOR_MAX);
                }
            }
        }
    }

    return hr;
}

// Invalidates the client area for redrawing

HRESULT CMainWindow::Invalidate()
{
    BOOL bResult = InvalidateRect(
        m_hwnd,
        NULL,
        FALSE
        );
    
    HRESULT hr = bResult ? S_OK : E_FAIL;

    return hr;
}

// Creates and initializes the main animation components

HRESULT CMainWindow::InitializeAnimation()
{
    // Create Animation Manager
    HRESULT hr = CoCreateInstance(
        CLSID_UIAnimationManager,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_pAnimationManager)
        );
    if (SUCCEEDED(hr))
    {
        // Create Animation Timer
        hr = CoCreateInstance(
            CLSID_UIAnimationTimer,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_pAnimationTimer)
            );
        if (SUCCEEDED(hr))
        {
            // Create Animation Transition Library
            hr = CoCreateInstance(
                CLSID_UIAnimationTransitionLibrary,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&m_pTransitionLibrary)
                );
            if (SUCCEEDED(hr))
            {
                // Create and set the ManagerEventHandler to start updating when animations are scheduled

                IUIAnimationManagerEventHandler *pManagerEventHandler;
                hr = CManagerEventHandler::CreateInstance(
                    this,
                    &pManagerEventHandler
                    );
                if (SUCCEEDED(hr))
                {
                    hr = m_pAnimationManager->SetManagerEventHandler(
                        pManagerEventHandler
                        );
                    pManagerEventHandler->Release();
                }
            }
        }
    }
    
    return hr;
}

// Creates the RGB animation variables for the background color

HRESULT CMainWindow::CreateAnimationVariables()
{
    const DOUBLE INITIAL_RED = COLOR_MAX; // 1.0f

    HRESULT hr = m_pAnimationManager->CreateAnimationVariable(
        INITIAL_RED,
        &m_pAnimationVariableRed
        );
    if (SUCCEEDED(hr))
    {
        hr = m_pAnimationVariableRed->SetLowerBound(COLOR_MIN); // 0.0f
        if (SUCCEEDED(hr))
        {
            hr = m_pAnimationVariableRed->SetUpperBound(COLOR_MAX); // 1.0f
        }
    }
    
    return hr;
}

// Creates resources which are not bound to any device.
// Their lifetime effectively extends for the duration of the app.
// These resources include the Direct2D factory.

HRESULT CMainWindow::CreateDeviceIndependentResources()
{
    // Create a Direct2D factory
    
    HRESULT hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        &m_pD2DFactory
        );

    return hr;
}

// Creates resources which are bound to a particular Direct3D device.
// It's all centralized here, so the resources can be easily recreated
// in the event of Direct3D device loss (e.g. display change, remoting,
// removal of video card, etc). The resources will only be created
// if necessary.
             
HRESULT CMainWindow::CreateDeviceResources()
{
    HRESULT hr = S_OK;
    
    if (m_pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(
            m_hwnd,
            &rc
            );

        D2D1_SIZE_U size = D2D1::SizeU(
            rc.right - rc.left,
            rc.bottom - rc.top
            );
 
        // Create a Direct2D render target

        hr = m_pD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(
                m_hwnd,
                size
                ),
            &m_pRenderTarget
            );
        if (SUCCEEDED(hr))
        {
            // Create a background brush

            hr = m_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(
                    D2D1::ColorF::Blue
                    ),
                &m_pBackgroundBrush
                );
        }
    }

    return hr;
}
                                                                                             
//  Discards device-specific resources that need to be recreated   
//  when a Direct3D device is lost                                      
                                                              
void CMainWindow::DiscardDeviceResources()
{
    SafeRelease(&m_pRenderTarget);
    SafeRelease(&m_pBackgroundBrush);
}

// The window message handler

LRESULT CALLBACK CMainWindow::WndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    const int MESSAGE_PROCESSED = 0;

    if (uMsg == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        CMainWindow *pMainWindow = (CMainWindow *)pcs->lpCreateParams;

        SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            PtrToUlong(pMainWindow)
            );

        return MESSAGE_PROCESSED;
    }

    CMainWindow *pMainWindow = reinterpret_cast<CMainWindow *>(static_cast<LONG_PTR>(
        GetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA
            )));

    if (pMainWindow != NULL)
    {
        switch (uMsg)
        {
        case WM_SIZE:
            {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                pMainWindow->OnResize(
                    width,
                    height
                    );
            }
            return MESSAGE_PROCESSED;
        
        case WM_LBUTTONDOWN:
            {
                pMainWindow->OnLButtonDown();
            }
            return MESSAGE_PROCESSED;

        case WM_PAINT:
        case WM_DISPLAYCHANGE:
            {
                PAINTSTRUCT ps;
                BeginPaint(
                    hwnd,
                    &ps
                    );

                pMainWindow->OnPaint(
                    ps.hdc,
                    ps.rcPaint
                    );

                EndPaint(
                    hwnd,
                    &ps
                    );
            }
            return MESSAGE_PROCESSED;

        case WM_DESTROY:
            {
                pMainWindow->OnDestroy();
            
                PostQuitMessage(
                    0
                    );
            }
            return MESSAGE_PROCESSED;
        }
    }
    
    return DefWindowProc(
        hwnd,
        uMsg,
        wParam,
        lParam
        );
}
                                                        
//  Called whenever the client area needs to be drawn

HRESULT CMainWindow::OnPaint(
    HDC hdc,
    const RECT &rcPaint
    )
{
    // Update the animation manager with the current time

    UI_ANIMATION_SECONDS secondsNow;
    HRESULT hr = m_pAnimationTimer->GetTime(
        &secondsNow
        );
    if (SUCCEEDED(hr))
    {
        hr = m_pAnimationManager->Update(
            secondsNow
            );
        if (SUCCEEDED(hr))
        {
            // Read the values of the animation variables and draw the client area
        
            hr = DrawClientArea();
            if (SUCCEEDED(hr))
            {
                // Continue redrawing the client area as long as there are animations scheduled
                
                UI_ANIMATION_MANAGER_STATUS status;
                hr = m_pAnimationManager->GetStatus(
                    &status
                    );
                if (SUCCEEDED(hr))
                {
                    if (status == UI_ANIMATION_MANAGER_BUSY)
                    {
                        InvalidateRect(
                            m_hwnd,
                            NULL,
                            FALSE
                            );
                    }
                }
            }
        }
    }

    return hr;
}

// When the left mouse button is clicked on the client area, schedule
// animations to change the background color of the window

HRESULT CMainWindow::OnLButtonDown()
{
    HRESULT hr = ChangeColor(
        RandomFromRange(COLOR_MIN, COLOR_MAX)
        );

    return hr; 
}
                                        
// Resizes the render target in response to a change in client area size                        

HRESULT CMainWindow::OnResize(
    UINT width,
    UINT height
    )
{
    HRESULT hr = S_OK;

    if (m_pRenderTarget)
    {
        D2D1_SIZE_U size;
        size.width = width;
        size.height = height;
        hr = m_pRenderTarget->Resize(
            size
            );
    }

    return hr;
}

// Prepares for window destruction

void CMainWindow::OnDestroy()
{
    if (m_pAnimationManager != NULL)
    {
        // Clear the manager event handler, to ensure that the one that points to this object is released
    
        (void)m_pAnimationManager->SetManagerEventHandler(NULL);
    }
}

// Draws the contents of the client area.
//
// Note that this function will automatically discard device-specific
// resources if the Direct3D device disappears during function
// invocation, and will recreate the resources the next time it's
// invoked.

HRESULT CMainWindow::DrawClientArea()
{
    // Begin drawing the client area

    HRESULT hr = CreateDeviceResources();
    if (SUCCEEDED(hr))
    {
        m_pRenderTarget->BeginDraw();

        // Retrieve the size of the render target

        D2D1_SIZE_F sizeRenderTarget = m_pRenderTarget->GetSize();

        hr = DrawBackground(
            D2D1::RectF(
                0.0f,
                0.0f,
                sizeRenderTarget.width,
                sizeRenderTarget.height
                )
            );

        // Can only return one failure HRESULT
        HRESULT hrEndDraw = m_pRenderTarget->EndDraw();
        if (SUCCEEDED(hr))
        {
            hr = hrEndDraw;
        }

        if (hr == D2DERR_RECREATE_TARGET)
        {
            DiscardDeviceResources();
        }
    }
    
    return hr;
}

// Fills the background with the color specified by the animation variables

HRESULT CMainWindow::DrawBackground(
    const D2D1_RECT_F &rectPaint
    )
{
    // Get the red animation variable value

    DOUBLE red;
    HRESULT hr = m_pAnimationVariableRed->GetValue(
        &red
        );
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr))
        {
            // Set the red value of the background brush to the new animated value

            m_pBackgroundBrush->SetColor(
                D2D1::ColorF(
                    static_cast<FLOAT>(red),
                    static_cast<FLOAT>(0),
                    static_cast<FLOAT>(0),
                    1.0
                    )
                );

            // Paint the background

            m_pRenderTarget->FillRectangle(
                rectPaint,
                m_pBackgroundBrush
                );
        }
    }

    return hr;
}

// Animates the background color to a new value

HRESULT CMainWindow::ChangeColor(
    DOUBLE red
    ) 
{
    std::wstringstream wss;
    wss << red;
    LPCWSTR wideString = L"New animation variable value: ";
    MessageBox(NULL, (std::wstring(wideString) + wss.str()).c_str(), L"Red", MB_OK);

    const UI_ANIMATION_SECONDS DURATION = 1;
    const DOUBLE ACCELERATION_RATIO = 0.5;
    const DOUBLE DECELERATION_RATIO = 0.5;

    // Create a storyboard

    IUIAnimationStoryboard *pStoryboard = NULL;
    HRESULT hr = m_pAnimationManager->CreateStoryboard(
        &pStoryboard
        );
    if (SUCCEEDED(hr))
    {
        // Create transition for the red animation variable

        IUIAnimationTransition *pTransitionRed;
        hr = m_pTransitionLibrary->CreateAccelerateDecelerateTransition(
            DURATION,
            red,
            ACCELERATION_RATIO,
            DECELERATION_RATIO,
            &pTransitionRed
            );
        if (SUCCEEDED(hr))
        {
            // Add transition to the storyboard

            hr = pStoryboard->AddTransition(
                m_pAnimationVariableRed,
                pTransitionRed
                );
            if (SUCCEEDED(hr))
            {
                // Get the current time and schedule the storyboard for play

                UI_ANIMATION_SECONDS secondsNow;
                hr = m_pAnimationTimer->GetTime(
                    &secondsNow
                    );
                if (SUCCEEDED(hr))
                {
                    hr = pStoryboard->Schedule(
                        secondsNow
                        );
                }
            }

            pTransitionRed->Release();
        }

        pStoryboard->Release();
    }

    return hr;
}

// Generate a random value in the given range

DOUBLE CMainWindow::RandomFromRange(
    DOUBLE minimum,
    DOUBLE maximum
    )
{
     return minimum + (maximum - minimum) * rand() / RAND_MAX;
}
