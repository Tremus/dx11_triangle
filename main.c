#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define CINTERFACE
#define COBJMACROS
#define UNICODE
#include <windows.h>

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <stdbool.h>
#include <stdio.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")
#define xassert(cond) (cond) ? (void)0 : __debugbreak()
#define ARRLEN(a)     (sizeof(a) / sizeof((a)[0]))

static bool g_window_resized = false;

HRESULT
craete_vertex_shader(ID3D11Device1* device, ID3D11VertexShader** vertexShader, ID3DBlob** vsBlob, WCHAR* pFileName)
{
    HRESULT   hResult  = S_OK;
    ID3DBlob* err_blob = NULL;

    hResult = D3DCompileFromFile(pFileName, NULL, NULL, "vs_main", "vs_5_0", 0, 0, vsBlob, &err_blob);
    xassert(SUCCEEDED(hResult));
    if (FAILED(hResult))
    {
        const char* errorString = NULL;
        if (hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            errorString = "Could not compile shader; file not found";
        else if (err_blob)
        {
            errorString = (const char*)err_blob->lpVtbl->GetBufferPointer(err_blob);
            err_blob->lpVtbl->Release(err_blob);
        }
        MessageBoxA(0, errorString, "Shader Compiler Error", MB_ICONERROR | MB_OK);
        return hResult;
    }

    hResult = device->lpVtbl->CreateVertexShader(
        device,
        (*vsBlob)->lpVtbl->GetBufferPointer(*vsBlob),
        (*vsBlob)->lpVtbl->GetBufferSize(*vsBlob),
        NULL,
        vertexShader);
    xassert(SUCCEEDED(hResult));

    return hResult;
}

HRESULT
create_pixel_shader(ID3D11Device1* device, ID3D11PixelShader** pixelShader, ID3DBlob** ps_blob, WCHAR* pFileName)
{
    HRESULT hResult = S_OK;

    ID3DBlob* err_blob;
    hResult = D3DCompileFromFile(pFileName, NULL, NULL, "ps_main", "ps_5_0", 0, 0, ps_blob, &err_blob);
    if (FAILED(hResult))
    {
        const char* errorString = NULL;
        if (hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            errorString = "Could not compile shader; file not found";
        else if (err_blob)
        {
            errorString = (const char*)err_blob->lpVtbl->GetBufferPointer(err_blob);
            err_blob->lpVtbl->Release(err_blob);
        }
        MessageBoxA(0, errorString, "Shader Compiler Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    hResult = device->lpVtbl->CreatePixelShader(
        device,
        (*ps_blob)->lpVtbl->GetBufferPointer(*ps_blob),
        (*ps_blob)->lpVtbl->GetBufferSize(*ps_blob),
        NULL,
        pixelShader);
    xassert(SUCCEEDED(hResult));
    (*ps_blob)->lpVtbl->Release(*ps_blob);

    return hResult;
}

HRESULT
create_shaders_triangle(
    ID3D11Device1*       device,
    ID3D11VertexShader** vertexShader,
    ID3D11PixelShader**  pixelShader,
    ID3D11InputLayout**  inputLayout,
    ID3D11Buffer**       vertexBuffer,
    UINT*                numVerts,
    UINT*                stride,
    UINT*                offset)
{
    HRESULT   hResult = S_OK;
    ID3DBlob* vs_blob = NULL;
    ID3DBlob* ps_blob = NULL;

    hResult = craete_vertex_shader(device, vertexShader, &vs_blob, SHADER_PATH L"triangle.hlsl");
    if (FAILED(hResult))
        return hResult;

    hResult = create_pixel_shader(device, pixelShader, &ps_blob, SHADER_PATH L"triangle.hlsl");
    if (FAILED(hResult))
        return hResult;

    // Create Input Layout
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
        {"POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};

    hResult = device->lpVtbl->CreateInputLayout(
        device,
        inputElementDesc,
        ARRLEN(inputElementDesc),
        vs_blob->lpVtbl->GetBufferPointer(vs_blob),
        vs_blob->lpVtbl->GetBufferSize(vs_blob),
        inputLayout);
    xassert(SUCCEEDED(hResult));
    vs_blob->lpVtbl->Release(vs_blob);

    // Create Vertex Buffer
    struct Vert
    {
        float x, y;
        float r, g, b, a;
    };
    // clang-format off
    struct Vert vertexData[] = {
        // x,    y,    r,   g,   b,   a
        { 0.0f,  0.5f, 0.f, 1.f, 0.f, 1.f},
        { 0.5f, -0.5f, 1.f, 0.f, 0.f, 1.f},
        {-0.5f, -0.5f, 0.f, 0.f, 1.f, 1.f},
    };
    // clang-format on
    *stride   = sizeof(vertexData[0]);
    *numVerts = ARRLEN(vertexData);
    *offset   = 0;

    D3D11_BUFFER_DESC vertexBufferDesc = {0};
    vertexBufferDesc.ByteWidth         = sizeof(vertexData);
    vertexBufferDesc.Usage             = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexSubresourceData = {vertexData};

    hResult = device->lpVtbl->CreateBuffer(device, &vertexBufferDesc, &vertexSubresourceData, vertexBuffer);
    xassert(SUCCEEDED(hResult));

    return hResult;
}

HRESULT
create_shaders_rectangle(
    ID3D11Device1*       device,
    ID3D11VertexShader** vertexShader,
    ID3D11PixelShader**  pixelShader,
    ID3D11InputLayout**  inputLayout,
    ID3D11Buffer**       vertexBuffer,
    UINT*                numVerts,
    UINT*                stride,
    UINT*                offset,
    ID3D11Buffer**       cbuffer)
{
    HRESULT   hResult = S_OK;
    ID3DBlob* vs_blob = NULL;
    ID3DBlob* ps_blob = NULL;

    hResult = craete_vertex_shader(device, vertexShader, &vs_blob, SHADER_PATH L"rectangle.hlsl");
    if (FAILED(hResult))
        return hResult;

    hResult = create_pixel_shader(device, pixelShader, &ps_blob, SHADER_PATH L"rectangle.hlsl");
    if (FAILED(hResult))
        return hResult;

    // Create Input Layout
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
        {"POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};

    hResult = device->lpVtbl->CreateInputLayout(
        device,
        inputElementDesc,
        ARRLEN(inputElementDesc),
        vs_blob->lpVtbl->GetBufferPointer(vs_blob),
        vs_blob->lpVtbl->GetBufferSize(vs_blob),
        inputLayout);
    xassert(SUCCEEDED(hResult));
    vs_blob->lpVtbl->Release(vs_blob);

    // Create Vertex Buffer
    struct Vert
    {
        float x, y;
    };
    // clang-format off
    struct Vert vertexData[] = {
        // x,    y,
        {-0.5f,  0.5f},
        { 0.5f, -0.5f},
        {-0.5f, -0.5f},

        {-0.5f,  0.5f},
        { 0.5f,  0.5f},
        { 0.5f, -0.5f},
    };
    // clang-format on
    *stride   = sizeof(vertexData[0]);
    *numVerts = ARRLEN(vertexData);
    *offset   = 0;

    D3D11_BUFFER_DESC vertexBufferDesc = {0};
    vertexBufferDesc.ByteWidth         = sizeof(vertexData);
    vertexBufferDesc.Usage             = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexSubresourceData = {vertexData};

    hResult = device->lpVtbl->CreateBuffer(device, &vertexBufferDesc, &vertexSubresourceData, vertexBuffer);
    xassert(SUCCEEDED(hResult));

    // Create constant buffer
    struct CONSTANT_BUFFER
    {
        float colour1[4];
        float colour2[4];
    } data;

    data.colour1[0] = 1;
    data.colour1[1] = 0;
    data.colour1[2] = 1;
    data.colour1[3] = 1;

    data.colour2[0] = 0.8;
    data.colour2[1] = 0;
    data.colour2[2] = 1;
    data.colour2[3] = 1;

    // https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-buffers-constant-how-to
    D3D11_BUFFER_DESC constantBufferDesc = {0};
    constantBufferDesc.ByteWidth         = sizeof(data);
    constantBufferDesc.Usage             = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem          = &data;
    InitData.SysMemPitch      = 0;
    InitData.SysMemSlicePitch = 0;

    hResult = device->lpVtbl->CreateBuffer(device, &constantBufferDesc, &InitData, cbuffer);
    xassert(hResult == S_OK);

    return hResult;
}

HRESULT
create_shaders_rectangle_image(
    ID3D11Device1*       device,
    ID3D11VertexShader** vertexShader,
    ID3D11PixelShader**  pixelShader,
    ID3D11InputLayout**  inputLayout,
    ID3D11Buffer**       vertexBuffer,
    UINT*                numVerts,
    UINT*                stride,
    UINT*                offset,
    ID3D11Buffer**       cbuffer)
{
    HRESULT hResult = S_OK;
    // Create Vertex Shader
    ID3DBlob* vsBlob;
    {
        ID3DBlob* err_blob;
        hResult = D3DCompileFromFile(
            SHADER_PATH L"rectangle.hlsl",
            NULL,
            NULL,
            "vs_main",
            "vs_5_0",
            0,
            0,
            &vsBlob,
            &err_blob);
        if (FAILED(hResult))
        {
            const char* errorString = NULL;
            if (hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                errorString = "Could not compile shader; file not found";
            else if (err_blob)
            {
                errorString = (const char*)err_blob->lpVtbl->GetBufferPointer(err_blob);
                err_blob->lpVtbl->Release(err_blob);
            }
            MessageBoxA(0, errorString, "Shader Compiler Error", MB_ICONERROR | MB_OK);
            return hResult;
        }

        hResult = device->lpVtbl->CreateVertexShader(
            device,
            vsBlob->lpVtbl->GetBufferPointer(vsBlob),
            vsBlob->lpVtbl->GetBufferSize(vsBlob),
            NULL,
            vertexShader);
        xassert(SUCCEEDED(hResult));
    }

    // Create Pixel Shader
    {
        ID3DBlob* ps_blob;
        ID3DBlob* err_blob;
        hResult = D3DCompileFromFile(
            SHADER_PATH L"rectangle.hlsl",
            NULL,
            NULL,
            "ps_main",
            "ps_5_0",
            0,
            0,
            &ps_blob,
            &err_blob);
        if (FAILED(hResult))
        {
            const char* errorString = NULL;
            if (hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                errorString = "Could not compile shader; file not found";
            else if (err_blob)
            {
                errorString = (const char*)err_blob->lpVtbl->GetBufferPointer(err_blob);
                err_blob->lpVtbl->Release(err_blob);
            }
            MessageBoxA(0, errorString, "Shader Compiler Error", MB_ICONERROR | MB_OK);
            return 1;
        }

        hResult = device->lpVtbl->CreatePixelShader(
            device,
            ps_blob->lpVtbl->GetBufferPointer(ps_blob),
            ps_blob->lpVtbl->GetBufferSize(ps_blob),
            NULL,
            pixelShader);
        xassert(SUCCEEDED(hResult));
        ps_blob->lpVtbl->Release(ps_blob);
    }

    // Create Input Layout
    {
        D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
            {"POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};

        hResult = device->lpVtbl->CreateInputLayout(
            device,
            inputElementDesc,
            ARRLEN(inputElementDesc),
            vsBlob->lpVtbl->GetBufferPointer(vsBlob),
            vsBlob->lpVtbl->GetBufferSize(vsBlob),
            inputLayout);
        xassert(SUCCEEDED(hResult));
        vsBlob->lpVtbl->Release(vsBlob);
    }

    // Create Vertex Buffer
    {
        struct Vert
        {
            float x, y;
        };
        // clang-format off
        struct Vert vertexData[] = {
            // x,    y,
            {-0.5f,  0.5f},
            { 0.5f, -0.5f},
            {-0.5f, -0.5f},

            {-0.5f,  0.5f},
            { 0.5f,  0.5f},
            { 0.5f, -0.5f},
        };
        // clang-format on
        *stride   = sizeof(vertexData[0]);
        *numVerts = ARRLEN(vertexData);
        *offset   = 0;

        D3D11_BUFFER_DESC vertexBufferDesc = {0};
        vertexBufferDesc.ByteWidth         = sizeof(vertexData);
        vertexBufferDesc.Usage             = D3D11_USAGE_IMMUTABLE;
        vertexBufferDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexSubresourceData = {vertexData};

        hResult = device->lpVtbl->CreateBuffer(device, &vertexBufferDesc, &vertexSubresourceData, vertexBuffer);
        xassert(SUCCEEDED(hResult));
    }

    // Create constant buffer
    {
        float data[4] = {0.0f, 1.0f, 1.0f, 1.0f};
        // https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-buffers-constant-how-to
        D3D11_BUFFER_DESC constantBufferDesc = {0};
        constantBufferDesc.ByteWidth         = sizeof(float) * 4;
        constantBufferDesc.Usage             = D3D11_USAGE_DYNAMIC;
        constantBufferDesc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        constantBufferDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem          = &data;
        InitData.SysMemPitch      = 0;
        InitData.SysMemSlicePitch = 0;

        hResult = device->lpVtbl->CreateBuffer(device, &constantBufferDesc, &InitData, cbuffer);
        xassert(hResult == S_OK);
    }

    return hResult;
}

HRESULT
create_shaders_circle(
    ID3D11Device1*       device,
    ID3D11VertexShader** vertexShader,
    ID3D11PixelShader**  pixelShader,
    ID3D11InputLayout**  inputLayout,
    ID3D11Buffer**       vertexBuffer,
    UINT*                numVerts,
    UINT*                stride,
    UINT*                offset,
    ID3D11Buffer**       cbuffer)
{
    HRESULT   hResult = S_OK;
    ID3DBlob* vs_blob = NULL;
    ID3DBlob* ps_blob = NULL;

    hResult = craete_vertex_shader(device, vertexShader, &vs_blob, SHADER_PATH L"circle.hlsl");
    if (FAILED(hResult))
        return hResult;

    hResult = create_pixel_shader(device, pixelShader, &ps_blob, SHADER_PATH L"circle.hlsl");
    if (FAILED(hResult))
        return hResult;

    // Create Input Layout
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
        {"POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};

    hResult = device->lpVtbl->CreateInputLayout(
        device,
        inputElementDesc,
        ARRLEN(inputElementDesc),
        vs_blob->lpVtbl->GetBufferPointer(vs_blob),
        vs_blob->lpVtbl->GetBufferSize(vs_blob),
        inputLayout);
    xassert(SUCCEEDED(hResult));
    vs_blob->lpVtbl->Release(vs_blob);

    // Create Vertex Buffer
    struct Vert
    {
        float x, y;
    };
    // clang-format off
    struct Vert vertexData[] = {
        // x,    y,
        {-1.0f,  1.0f},
        { 1.0f, -1.0f},
        {-1.0f, -1.0f},

        {-1.0f,  1.0f},
        { 1.0f,  1.0f},
        { 1.0f, -1.0f},
    };
    // clang-format on
    *stride   = sizeof(vertexData[0]);
    *numVerts = ARRLEN(vertexData);
    *offset   = 0;

    D3D11_BUFFER_DESC vertexBufferDesc = {0};
    vertexBufferDesc.ByteWidth         = sizeof(vertexData);
    vertexBufferDesc.Usage             = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexSubresourceData = {vertexData};

    hResult = device->lpVtbl->CreateBuffer(device, &vertexBufferDesc, &vertexSubresourceData, vertexBuffer);
    xassert(SUCCEEDED(hResult));

    // Create constant buffer
    float data[4] = {0.0f, 1.0f, 1.0f, 1.0f};
    // https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-buffers-constant-how-to
    D3D11_BUFFER_DESC constantBufferDesc = {0};
    constantBufferDesc.ByteWidth         = sizeof(float) * 4;
    constantBufferDesc.Usage             = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem          = &data;
    InitData.SysMemPitch      = 0;
    InitData.SysMemSlicePitch = 0;

    hResult = device->lpVtbl->CreateBuffer(device, &constantBufferDesc, &InitData, cbuffer);
    xassert(hResult == S_OK);

    return hResult;
}

HRESULT
create_shaders_line_primitives(
    ID3D11Device1*          device,
    ID3D11VertexShader**    vertexShader,
    ID3D11PixelShader**     pixelShader,
    ID3D11InputLayout**     inputLayout,
    ID3D11Buffer**          vertexBuffer,
    UINT*                   numVerts,
    UINT*                   stride,
    UINT*                   offset,
    D3D_PRIMITIVE_TOPOLOGY* topology)
{
    HRESULT   hResult = S_OK;
    ID3DBlob* vs_blob = NULL;
    ID3DBlob* ps_blob = NULL;

    hResult = craete_vertex_shader(device, vertexShader, &vs_blob, SHADER_PATH L"linelist_primitive.hlsl");
    if (FAILED(hResult))
        return hResult;

    hResult = create_pixel_shader(device, pixelShader, &ps_blob, SHADER_PATH L"linelist_primitive.hlsl");
    if (FAILED(hResult))
        return hResult;

    // Create Input Layout
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
        {"POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};

    hResult = device->lpVtbl->CreateInputLayout(
        device,
        inputElementDesc,
        ARRLEN(inputElementDesc),
        vs_blob->lpVtbl->GetBufferPointer(vs_blob),
        vs_blob->lpVtbl->GetBufferSize(vs_blob),
        inputLayout);
    xassert(SUCCEEDED(hResult));
    vs_blob->lpVtbl->Release(vs_blob);

    // Create Vertex Buffer
    struct Vert
    {
        float x, y;
    };
    // clang-format off
    struct Vert vertexData[] = {
        // x,    y,
        { -0.6f,  0.6f},
        { 0.7f, 0.7f},
        {-0.6f, -0.7f},
        {0.2f, -0.5f},
    };
    // clang-format on
    *stride   = sizeof(vertexData[0]);
    *numVerts = ARRLEN(vertexData);
    *offset   = 0;

    D3D11_BUFFER_DESC vertexBufferDesc = {0};
    vertexBufferDesc.ByteWidth         = sizeof(vertexData);
    vertexBufferDesc.Usage             = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexSubresourceData = {vertexData};

    hResult = device->lpVtbl->CreateBuffer(device, &vertexBufferDesc, &vertexSubresourceData, vertexBuffer);
    xassert(SUCCEEDED(hResult));
    *topology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;

    return hResult;
}

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
    case WM_SIZE:
        g_window_resized = true;
        break;
    default:
        result = DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    return result;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    HRESULT hResult;
    HWND    hwnd;
    {
        WNDCLASSEXW wc   = {0};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = &WndProc;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIconW(NULL, IDI_APPLICATION);
        wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
        wc.lpszClassName = L"Triangle";
        wc.hIconSm       = LoadIconW(NULL, IDI_APPLICATION);

        if (! RegisterClassExW(&wc))
        {
            MessageBoxW(0, L"RegisterClassEx failed", L"Fatal Error", MB_OK);
            return GetLastError();
        }

        RECT rect = {0, 0, 1280, 720};
        AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
        LONG initialWidth  = rect.right - rect.left;
        LONG initialHeight = rect.bottom - rect.top;

        hwnd = CreateWindowExW(
            WS_EX_OVERLAPPEDWINDOW,
            wc.lpszClassName,
            L"Triangle",
            WS_OVERLAPPEDWINDOW,
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
            MessageBoxW(0, L"CreateWindowExA failed", L"Fatal Error", MB_OK);
            return GetLastError();
        }
    }

    // Create D3D11 Device and Context
    ID3D11Device1*        device;
    ID3D11DeviceContext1* device_ctx;
    {
        ID3D11Device*        baseDevice;
        ID3D11DeviceContext* baseDeviceContext;

        D3D_DRIVER_TYPE driver_types[] = {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_SOFTWARE,
            D3D_DRIVER_TYPE_REFERENCE,
        };

        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1,
        };
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifndef NDEBUG
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        hResult = S_OK;
        for (int i = 0; i < ARRLEN(driver_types); i++)
        {
            hResult = D3D11CreateDevice(
                0,
                driver_types[i],
                0,
                creationFlags,
                featureLevels,
                ARRLEN(featureLevels),
                D3D11_SDK_VERSION,
                &baseDevice,
                0,
                &baseDeviceContext);
            if (hResult == S_OK)
                break;
        }

        if (FAILED(hResult))
        {
            MessageBoxW(0, L"D3D11CreateDevice() failed", L"Fatal Error", MB_OK);
            return GetLastError();
        }

        // Get 1.1 interface of D3D11 Device and Context
        hResult = baseDevice->lpVtbl->QueryInterface(baseDevice, &IID_ID3D11Device1, (void**)&device);
        xassert(SUCCEEDED(hResult));
        baseDevice->lpVtbl->Release(baseDevice);

        hResult = baseDeviceContext->lpVtbl->QueryInterface(
            baseDeviceContext,
            &IID_ID3D11DeviceContext1,
            (void**)&device_ctx);
        xassert(SUCCEEDED(hResult));
        baseDeviceContext->lpVtbl->Release(baseDeviceContext);
    }

#ifndef NDEBUG
    // Set up debug layer to break on D3D11 errors
    ID3D11Debug* d3dDebug = NULL;
    device->lpVtbl->QueryInterface(device, &IID_ID3D11Debug, (void**)&d3dDebug);
    if (d3dDebug)
    {
        ID3D11InfoQueue* d3dInfoQueue = NULL;
        d3dDebug->lpVtbl->QueryInterface(d3dDebug, &IID_ID3D11InfoQueue, (void**)&d3dInfoQueue);
        xassert(d3dInfoQueue);
        if (d3dInfoQueue)
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
        // Factory2 required to create SwapChain1
        // https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nn-dxgi1_2-idxgifactory2
        IDXGIFactory2* factory;
        {
            IDXGIDevice1* dxgiDevice;
            hResult = device->lpVtbl->QueryInterface(device, &IID_IDXGIDevice1, (void**)&dxgiDevice);
            xassert(SUCCEEDED(hResult));

            IDXGIAdapter* dxgiAdapter;
            hResult = dxgiDevice->lpVtbl->GetAdapter(dxgiDevice, &dxgiAdapter);
            xassert(SUCCEEDED(hResult));
            dxgiDevice->lpVtbl->Release(dxgiDevice);

            DXGI_ADAPTER_DESC adapterDesc;
            dxgiAdapter->lpVtbl->GetDesc(dxgiAdapter, &adapterDesc);

            OutputDebugStringW(L"Graphics Device: ");
            OutputDebugStringW(adapterDesc.Description);

            hResult = dxgiAdapter->lpVtbl->GetParent(dxgiAdapter, &IID_IDXGIFactory2, (void**)&factory);
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
            DXGI_SWAP_EFFECT_FLIP_DISCARD,
            DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
            DXGI_SWAP_EFFECT_SEQUENTIAL,
            DXGI_SWAP_EFFECT_DISCARD,
        };
        hResult = 0;
        for (int i = 0; i < ARRLEN(swap_effect_types); i++)
        {
            desc.SwapEffect = swap_effect_types[i];
            hResult =
                factory->lpVtbl->CreateSwapChainForHwnd(factory, (IUnknown*)device, hwnd, &desc, 0, 0, &swapchain);
            if (SUCCEEDED(hResult))
                break;
        }
        xassert(SUCCEEDED(hResult));

        factory->lpVtbl->Release(factory);
    }

    // Create Framebuffer Render Target
    ID3D11RenderTargetView* rendertarget;
    {
        ID3D11Texture2D* framebuffer;
        hResult = swapchain->lpVtbl->GetBuffer(swapchain, 0, &IID_ID3D11Texture2D, (void**)&framebuffer);
        xassert(SUCCEEDED(hResult));

        hResult = device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)framebuffer, NULL, &rendertarget);
        xassert(SUCCEEDED(hResult));
        framebuffer->lpVtbl->Release(framebuffer);
    }

    // Put these into a struct?
    ID3D11VertexShader* vertexShader = NULL;
    ID3D11PixelShader*  pixelShader  = NULL;
    ID3D11InputLayout*  inputLayout  = NULL;

    ID3D11Buffer* vertexBuffer = NULL;
    UINT          numVerts     = 0;
    UINT          stride       = 0;
    UINT          offset       = 0;
    ID3D11Buffer* cbuffer      = NULL;

    D3D_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    hResult = create_shaders_triangle(
        device,
        &vertexShader,
        &pixelShader,
        &inputLayout,
        &vertexBuffer,
        &numVerts,
        &stride,
        &offset);
    // hResult = create_shaders_rectangle(
    //     device,
    //     &vertexShader,
    //     &pixelShader,
    //     &inputLayout,
    //     &vertexBuffer,
    //     &numVerts,
    //     &stride,
    //     &offset,
    //     &cbuffer);
    // hResult = create_shaders_circle(
    //     device,
    //     &vertexShader,
    //     &pixelShader,
    //     &inputLayout,
    //     &vertexBuffer,
    //     &numVerts,
    //     &stride,
    //     &offset,
    //     &cbuffer);
    // hResult = create_shaders_line_primitives(
    //     device,
    //     &vertexShader,
    //     &pixelShader,
    //     &inputLayout,
    //     &vertexBuffer,
    //     &numVerts,
    //     &stride,
    //     &offset,
    //     &topology);
    if (hResult)
        return hResult;

    ShowWindow(hwnd, SW_SHOW);

    MSG  msg;
    bool running = true;
    while (running)
    {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = false;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (g_window_resized)
        {
            device_ctx->lpVtbl->OMSetRenderTargets(device_ctx, 0, 0, 0);
            rendertarget->lpVtbl->Release(rendertarget);

            hResult = swapchain->lpVtbl->ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            xassert(SUCCEEDED(hResult));

            ID3D11Texture2D* framebuffer;
            hResult = swapchain->lpVtbl->GetBuffer(swapchain, 0, &IID_ID3D11Texture2D, (void**)&framebuffer);
            xassert(SUCCEEDED(hResult));

            hResult = device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)framebuffer, NULL, &rendertarget);
            xassert(SUCCEEDED(hResult));
            framebuffer->lpVtbl->Release(framebuffer);

            g_window_resized = false;
        }

        FLOAT colour[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        device_ctx->lpVtbl->ClearRenderTargetView(device_ctx, rendertarget, colour);

        RECT winRect;
        GetClientRect(hwnd, &winRect);
        D3D11_VIEWPORT viewport =
            {0.0f, 0.0f, (FLOAT)(winRect.right - winRect.left), (FLOAT)(winRect.bottom - winRect.top), 0.0f, 1.0f};
        device_ctx->lpVtbl->RSSetViewports(device_ctx, 1, &viewport);
        device_ctx->lpVtbl->OMSetRenderTargets(device_ctx, 1, &rendertarget, NULL);

        device_ctx->lpVtbl->IASetPrimitiveTopology(device_ctx, topology);
        device_ctx->lpVtbl->IASetInputLayout(device_ctx, inputLayout);

        device_ctx->lpVtbl->VSSetShader(device_ctx, vertexShader, NULL, 0);
        device_ctx->lpVtbl->PSSetShader(device_ctx, pixelShader, NULL, 0);

        if (cbuffer)
            device_ctx->lpVtbl->PSSetConstantBuffers(device_ctx, 0, 1, &cbuffer);

        device_ctx->lpVtbl->IASetVertexBuffers(device_ctx, 0, 1, &vertexBuffer, &stride, &offset);
        device_ctx->lpVtbl->Draw(device_ctx, numVerts, 0);

        swapchain->lpVtbl->Present(swapchain, 1, 0);
    }

    if (swapchain)
        swapchain->lpVtbl->Release(swapchain);
    if (rendertarget)
        rendertarget->lpVtbl->Release(rendertarget);
    if (vertexBuffer)
        vertexBuffer->lpVtbl->Release(vertexBuffer);
    if (cbuffer)
        cbuffer->lpVtbl->Release(cbuffer);

    vertexShader->lpVtbl->Release(vertexShader);
    pixelShader->lpVtbl->Release(pixelShader);
    inputLayout->lpVtbl->Release(inputLayout);

    device_ctx->lpVtbl->Release(device_ctx);
    device->lpVtbl->Release(device);
    // d3dDebug->lpVtbl->Release(d3dDebug);

    return 0;
}