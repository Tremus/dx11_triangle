#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define CINTERFACE
#define COBJMACROS
#include <d3d11_1.h>
#include <stdbool.h>
#include <windows.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")
#define xassert(cond) (cond) ? (void)0 : __debugbreak()
#define ARRLEN(a)     (sizeof(a) / sizeof(a[0]))

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
            DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        result = DefWindowProcA(hwnd, msg, wparam, lparam);
    }
    return result;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    HWND hwnd;
    {
        WNDCLASSEXA wc   = {0};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = &WndProc;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIconA(NULL, IDI_APPLICATION);
        wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
        wc.lpszClassName = "Triangle";
        wc.hIconSm       = LoadIconA(NULL, IDI_APPLICATION);

        if (! RegisterClassExA(&wc))
        {
            MessageBoxA(0, "RegisterClassEx failed", "Fatal Error", MB_OK);
            return GetLastError();
        }

        RECT rect = {0, 0, 1280, 720};
        AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
        LONG initialWidth  = rect.right - rect.left;
        LONG initialHeight = rect.bottom - rect.top;

        hwnd = CreateWindowExA(
            WS_EX_OVERLAPPEDWINDOW,
            wc.lpszClassName,
            "Triangle",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            initialWidth,
            initialHeight,
            NULL,
            NULL,
            hInstance,
            NULL);

        if (! hwnd)
        {
            MessageBoxA(0, "CreateWindowExA failed", "Fatal Error", MB_OK);
            return GetLastError();
        }
    }

    // Create D3D11 Device and Context
    ID3D11Device1*        d3d11Device;
    ID3D11DeviceContext1* d3d11DeviceContext;
    {
        ID3D11Device*        baseDevice;
        ID3D11DeviceContext* baseDeviceContext;
        D3D_FEATURE_LEVEL    featureLevels[] = {D3D_FEATURE_LEVEL_11_0};
        UINT                 creationFlags   = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifndef NDEBUG
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        HRESULT hResult = D3D11CreateDevice(
            0,
            D3D_DRIVER_TYPE_HARDWARE,
            0,
            creationFlags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &baseDevice,
            0,
            &baseDeviceContext);
        if (FAILED(hResult))
        {
            MessageBoxA(0, "D3D11CreateDevice() failed", "Fatal Error", MB_OK);
            return GetLastError();
        }

        // Get 1.1 interface of D3D11 Device and Context
        hResult = baseDevice->lpVtbl->QueryInterface(baseDevice, &IID_ID3D11Device1, (void**)&d3d11Device);
        xassert(SUCCEEDED(hResult));
        baseDevice->lpVtbl->Release(baseDevice);

        hResult = baseDeviceContext->lpVtbl->QueryInterface(
            baseDeviceContext,
            &IID_ID3D11DeviceContext1,
            (void**)&d3d11DeviceContext);
        xassert(SUCCEEDED(hResult));
        baseDeviceContext->lpVtbl->Release(baseDeviceContext);
    }

#ifndef NDEBUG
    // Set up debug layer to break on D3D11 errors
    ID3D11Debug* d3dDebug = NULL;
    d3d11Device->lpVtbl->QueryInterface(d3d11Device, &IID_ID3D11Debug, (void**)&d3dDebug);
    if (d3dDebug)
    {
        ID3D11InfoQueue* d3dInfoQueue = NULL;
        if (SUCCEEDED(d3dDebug->lpVtbl->QueryInterface(d3dDebug, &IID_ID3D11InfoQueue, (void**)&d3dInfoQueue)))
        {
            d3dInfoQueue->lpVtbl->SetBreakOnSeverity(d3dInfoQueue, D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->lpVtbl->SetBreakOnSeverity(d3dInfoQueue, D3D11_MESSAGE_SEVERITY_ERROR, true);
            d3dInfoQueue->lpVtbl->Release(d3dInfoQueue);
        }
        d3dDebug->lpVtbl->Release(d3dDebug);
    }
#endif

    // Create Swap Chain
    IDXGISwapChain1* swapchain;
    {
        // Get DXGI Factory (needed to create Swap Chain)
        IDXGIFactory2* dxgi_factory;
        {
            IDXGIDevice1* dxgiDevice;
            HRESULT hResult = d3d11Device->lpVtbl->QueryInterface(d3d11Device, &IID_IDXGIDevice1, (void**)&dxgiDevice);
            xassert(SUCCEEDED(hResult));

            IDXGIAdapter* dxgiAdapter;
            hResult = dxgiDevice->lpVtbl->GetAdapter(dxgiDevice, &dxgiAdapter);
            xassert(SUCCEEDED(hResult));
            dxgiDevice->lpVtbl->Release(dxgiDevice);

            DXGI_ADAPTER_DESC adapterDesc;
            dxgiAdapter->lpVtbl->GetDesc(dxgiAdapter, &adapterDesc);

            OutputDebugStringA("Graphics Device: ");
            OutputDebugStringW(adapterDesc.Description);

            hResult = dxgiAdapter->lpVtbl->GetParent(dxgiAdapter, &IID_IDXGIFactory2, (void**)&dxgi_factory);
            xassert(SUCCEEDED(hResult));
            dxgiAdapter->lpVtbl->Release(dxgiAdapter);
        }

        DXGI_SWAP_CHAIN_DESC1 desc = {0};
        desc.Width                 = 0; // use window width
        desc.Height                = 0; // use window height
        desc.Format                = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count      = 1;
        desc.SampleDesc.Quality    = 0;
        desc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount           = 2;
        desc.Scaling               = DXGI_SCALING_STRETCH;
        desc.SwapEffect            = DXGI_SWAP_EFFECT_DISCARD;
        desc.AlphaMode             = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.Flags                 = 0;

        const DXGI_SWAP_EFFECT swap_effect_types[] = {
            DXGI_SWAP_EFFECT_DISCARD,
            DXGI_SWAP_EFFECT_SEQUENTIAL,
            DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
            DXGI_SWAP_EFFECT_FLIP_DISCARD,
        };
        HRESULT hResult = 0;
        for (int i = ARRLEN(swap_effect_types); i-- != 0;)
        {
            desc.SwapEffect = swap_effect_types[i];
            hResult         = dxgi_factory->lpVtbl
                          ->CreateSwapChainForHwnd(dxgi_factory, (IUnknown*)d3d11Device, hwnd, &desc, 0, 0, &swapchain);
            if (SUCCEEDED(hResult))
                break;
        }
        xassert(SUCCEEDED(hResult));

        dxgi_factory->lpVtbl->Release(dxgi_factory);
    }

    // Create Framebuffer Render Target
    ID3D11RenderTargetView* rendertarget;
    {
        ID3D11Texture2D* framebuffer;
        HRESULT hResult = swapchain->lpVtbl->GetBuffer(swapchain, 0, &IID_ID3D11Texture2D, (void**)&framebuffer);
        xassert(SUCCEEDED(hResult));

        hResult =
            d3d11Device->lpVtbl->CreateRenderTargetView(d3d11Device, (ID3D11Resource*)framebuffer, NULL, &rendertarget);
        xassert(SUCCEEDED(hResult));
        framebuffer->lpVtbl->Release(framebuffer);
    }

    MSG  msg;
    BOOL running = TRUE;
    while (running)
    {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = FALSE;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        FLOAT backgroundColor[4] = {1.0f, 0.5f, 1.0f, 1.0f};
        d3d11DeviceContext->lpVtbl->ClearRenderTargetView(d3d11DeviceContext, rendertarget, backgroundColor);

        swapchain->lpVtbl->Present(swapchain, 1, 0);
    }

    return 0;
}