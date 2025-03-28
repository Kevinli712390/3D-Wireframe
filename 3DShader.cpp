//////////////////////////////////////////////////////////////////////////
//
//       Software Assessment: Shader Model Viewer
//
//       Kevin Li, 3/22/2025
//
//////////////////////////////////////////////////////////////////////////

#include "3DShaderViewer.hpp"
#include <windows.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cmath>

#undef min
#undef max

// Window dimensions
const int WIDTH = 800;
const int HEIGHT = 600;

// Global variables for storing geometry and transformations
std::vector<Vertex> vertices;
std::vector<Face> faces;
std::vector<Vertex> transformed;

// Mouse dragging state for rotation
bool dragging = false;
POINT lastMouse;    // Previous mouse position 
float angleX = 0.0f, angleY = 0.0f; // Rotation angles

// Project a 3D vertex to 2D screen coordinates
POINT project(const Vertex& v) {
    float scale = std::min(WIDTH, HEIGHT) * 0.4f;
    return {
        static_cast<LONG>(WIDTH / 2 + v.x * scale),
        static_cast<LONG>(HEIGHT / 2 - v.y * scale)
    };
}

// Rotate a vertex around the X-axis
Vertex rotateX(const Vertex& v, float angle) {
    float rad = angle * 3.14159265f / 180.0f;
    float c = cosf(rad), s = sinf(rad);
    return {
        v.id,
        v.x,
        v.y * c - v.z * s,
        v.y * s + v.z * c
    };
}

// Rotate a vertex around the Y-axis 
Vertex rotateY(const Vertex& v, float angle) {
    float rad = angle * 3.14159265f / 180.0f;
    float c = cosf(rad), s = sinf(rad);
    return {
        v.id,
        v.x * c + v.z * s,
        v.y,
        -v.x * s + v.z * c
    };
}

// Normalize, center, and rotate all vertices
void applyTransform() {
    transformed.clear();

    // Compute model centroid
    float cx = 0, cy = 0, cz = 0;
    for (const auto& v : vertices) {
        cx += v.x; cy += v.y; cz += v.z;
    }
    cx /= vertices.size();
    cy /= vertices.size();
    cz /= vertices.size();

    // Calculate max distance from center
    float maxExtent = 0;
    for (const auto& v : vertices) {
        float dx = v.x - cx, dy = v.y - cy, dz = v.z - cz;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);
        if (dist > maxExtent) maxExtent = dist;
    }

    // Normalize, rotate, and store transformed vertices
    for (const auto& v : vertices) {
        Vertex normed = {
            v.id,
            (v.x - cx) / maxExtent,
            (v.y - cy) / maxExtent,
            (v.z - cz) / maxExtent
        };
        Vertex t = rotateX(normed, angleX);
        t = rotateY(t, angleY);
        transformed.push_back(t);
    }
}

// Load vertices from file
std::vector<Vertex> loadVertices(std::ifstream& file, int vertexCount) {
    std::vector<Vertex> vertices;
    std::string line;
    for (int i = 0; i < vertexCount; ++i) {
        std::getline(file, line);
        if (line.empty()) { --i; continue; }
        std::replace(line.begin(), line.end(), ',', ' ');
        std::istringstream iss(line);
        Vertex v;
        iss >> v.id >> v.x >> v.y >> v.z;
        vertices.push_back(v);
    }
    return vertices;
}

// Load faces from file
std::vector<Face> loadFaces(std::ifstream& file, int faceCount) {
    std::vector<Face> faces;
    std::string line;
    for (int i = 0; i < faceCount; ++i) {
        std::getline(file, line);
        if (line.empty()) { --i; continue; }
        std::replace(line.begin(), line.end(), ',', ' ');
        std::istringstream iss(line);
        Face f;
        iss >> f.v1 >> f.v2 >> f.v3;
        faces.push_back(f);
    }
    return faces;
}

// Draw shaded model using face normals to control blue intensity
void drawShadedModel(HDC hdc) {
    RECT rect;
    GetClientRect(WindowFromDC(hdc), &rect);

    // Offscreen buffer for smooth rendering
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
    FillRect(memDC, &rect, (HBRUSH)(COLOR_WINDOW + 1));

    std::vector<bool> vertexVisible(transformed.size(), false);

    // Sort faces by average Z-depth
    struct IndexedFace { Face face; float avgZ; };
    std::vector<IndexedFace> sortedFaces;
    for (const auto& f : faces) {
        float zAvg = (transformed[f.v1 - 1].z + transformed[f.v2 - 1].z + transformed[f.v3 - 1].z) / 3.0f;
        sortedFaces.push_back({ f, zAvg });
    }
    std::sort(sortedFaces.begin(), sortedFaces.end(), [](const IndexedFace& a, const IndexedFace& b) {
        return a.avgZ < b.avgZ;
        });

    for (const auto& entry : sortedFaces) {
        const Face& f = entry.face;
        const Vertex& v1 = transformed[f.v1 - 1];
        const Vertex& v2 = transformed[f.v2 - 1];
        const Vertex& v3 = transformed[f.v3 - 1];

        // Compute face normal
        float ux = v2.x - v1.x;
        float uy = v2.y - v1.y;
		float uz = v2.z - v1.z;
        float vx = v3.x - v1.x;
		float vy = v3.y - v1.y;
		float vz = v3.z - v1.z;
        float nx = uy * vz - uz * vy;
        float ny = uz * vx - ux * vz;
        float nz = ux * vy - uy * vx;
        float length = sqrtf(nx * nx + ny * ny + nz * nz);
        if (length < 1e-6f) continue;
        nx /= length; ny /= length; nz /= length;

        // Blue shading based on angle with Z-axis
        float intensity = fabs(nz);
        int blue = static_cast<int>(0x5F + intensity * (0xFF - 0x5F));
        COLORREF color = RGB(0, 0, blue);

        // Fill triangle
        HBRUSH brush = CreateSolidBrush(color);
        HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, brush);
        HPEN pen = CreatePen(PS_NULL, 0, 0);
        HPEN oldPen = (HPEN)SelectObject(memDC, pen);
        POINT pts[3] = { project(v1), project(v2), project(v3) };
        Polygon(memDC, pts, 3);
        SelectObject(memDC, oldBrush); DeleteObject(brush);
        SelectObject(memDC, oldPen); DeleteObject(pen);

        // Draw wireframe overlay
        HPEN wirePen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        HPEN prevPen = (HPEN)SelectObject(memDC, wirePen);
        MoveToEx(memDC, pts[0].x, pts[0].y, nullptr);
        LineTo(memDC, pts[1].x, pts[1].y);
        LineTo(memDC, pts[2].x, pts[2].y);
        LineTo(memDC, pts[0].x, pts[0].y);
        SelectObject(memDC, prevPen); DeleteObject(wirePen);

        // Mark vertices of front-facing triangles
        if (nz > 0) {
            vertexVisible[f.v1 - 1] = true;
            vertexVisible[f.v2 - 1] = true;
            vertexVisible[f.v3 - 1] = true;
        }
    }

    // Draw visible vertex dots in blue
    HBRUSH blueDot = CreateSolidBrush(RGB(0, 0, 255));
    HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, blueDot);
    for (size_t i = 0; i < transformed.size(); ++i) {
        if (!vertexVisible[i]) continue;
        if (transformed[i].z <= 0) continue;

        POINT p = project(transformed[i]);
        Ellipse(memDC, p.x - 3, p.y - 3, p.x + 3, p.y + 3);
    }
    SelectObject(memDC, oldBrush);
    DeleteObject(blueDot);

    // Blit the final image to screen
    BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

// Handle user input and window messages
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_LBUTTONDOWN:
        dragging = true;
        lastMouse.x = LOWORD(lParam);
        lastMouse.y = HIWORD(lParam);
        break;
    case WM_LBUTTONUP:
        dragging = false;
        break;
    case WM_MOUSEMOVE:
        if (dragging) {
            int x = LOWORD(lParam), y = HIWORD(lParam);
            int dx = x - lastMouse.x;
            int dy = y - lastMouse.y;
            lastMouse.x = x;
            lastMouse.y = y;
            angleY += dx * 0.5f;
            angleX += dy * 0.5f;
            applyTransform();
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        drawShadedModel(hdc);
        EndPaint(hwnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Entry point: load model, create window, start message loop
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    std::ifstream file("object.txt");
    if (!file.is_open()) {
        MessageBoxA(nullptr, "Could not open object.txt", "Error", MB_OK);
        return 1;
    }

    // Parse header: # vertices, # faces
    std::string header;
    std::getline(file, header);
    std::replace(header.begin(), header.end(), ',', ' ');
    std::istringstream hs(header);
    int vertexCount, faceCount;
    hs >> vertexCount >> faceCount;

    vertices = loadVertices(file, vertexCount);
    faces = loadFaces(file, faceCount);
    applyTransform();

    // Register window class
    const wchar_t CLASS_NAME[] = L"3DViewerWin32";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);

    // Create application window
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"3D Wireframe Viewer",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT,
        nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Main message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
