/////////////////////////////////////////////////////////////////
//
//      Task:
//
//      Create a separate program that contains all the functionality of part 1. Additionally, make each of the
//      visible faces of the object a solid, opaque blue color. Make the color smoothly vary between #00005F
//      (when the surface is viewed on edge, i.e., the normal of the surface makes a 90-degree angle to the Z-
//      axis) and #0000FF (when the surface is viewed flat, i.e., orthogonal to the Z-axis) based on the angle with
//      the Z-axis, such that the face is displayed similarly to how a shader would display it.
//
//      Kevin Li, 3/23/2025
//
/////////////////////////////////////////////////////////////////

#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <fstream>

// Structure representing a vertex in 3D space
struct Vertex {
    int id;     // Vertex identifier
    float x;    // X coordinate
    float y;    // Y coordinate
    float z;    // Z coordinate
};

// Structure representing a triangular face using 1-based vertex indices
struct Face {
    int v1;     // Index of first vertex
    int v2;     // Index of second vertex
    int v3;     // Index of third vertex
};

// Window dimensions
extern const int WIDTH;
extern const int HEIGHT;

// Global state used throughout the program
extern std::vector<Vertex> vertices;      // List of original vertices loaded from file
extern std::vector<Vertex> transformed;   // Transformed (normalized + rotated) vertices
extern std::vector<Face> faces;           // List of triangular faces

extern bool dragging;         // True if mouse is dragging 
extern POINT lastMouse;       // Last mouse position recorded
extern float angleX, angleY;  // Rotation angles in degrees for X and Y axes

// Projects a 3D vertex to 2D screen coordinates
POINT project(const Vertex& v);

// Rotates a vertex around the X-axis by a specified angle
Vertex rotateX(const Vertex& v, float angle);

// Rotates a vertex around the Y-axis by a specified angle
Vertex rotateY(const Vertex& v, float angle);

// Applies centering, normalization, and rotation transformations to all vertices
void applyTransform();

// Loads vertex data from file and returns a list of Vertex structs
std::vector<Vertex> loadVertices(std::ifstream& file, int vertexCount);

// Loads face data (triangles) from file and returns a list of Face structs
std::vector<Face> loadFaces(std::ifstream& file, int faceCount);

// Renders the shaded 3D model (with smooth shading and edge overlay) to the provided HDC
void drawShadedModel(HDC hdc);

// Handles Win32 events: input, painting, and cleanup
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Application entry point (main function for Win32 GUI apps)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow);
