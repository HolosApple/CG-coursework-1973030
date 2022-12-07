#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <RayTriangleIntersection.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>
#include <map>
#include<iostream>
#include<cstring>
#include <ModelTexture.h>

using namespace std;

using namespace glm;

#define WIDTH 640
#define HEIGHT 480

#define WIREFRAME 1
#define RASTERISER 2
#define RAYTRACER 3
#define ANITALIASING 4
#define CULLING 5
#define WULINES 6
#define JUMPANIMATION 7

#define PI 3.14159265
#define SCALETING 0.5
#define SCALETING2 0.1
#define SCALETING3 0.002

void handleEvent(SDL_Event event);

static DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

int mode = WIREFRAME;
mat3 camOrien = mat3();
vec3 camera = vec3(0,0,6);
float angle = 0.1f;
int fileCount = 0;


vector<vec3> interpolation3(vec3 v1, vec3 v2, int n)
{
    vector<vec3> result;
    float x = (v2.x - v1.x)/(n-1);
    float y = (v2.y - v1.y)/(n-1);
    float z = (v2.z - v1.z)/(n-1);
    for(int i = 0; i < n; i++)
    {
        result.push_back(vec3(v1.x + i * x,v1.y + i * y,v1.z + i * z));
    }
    return result;
}

vector<vec3> setlights(vec3 light)
{
    vector<vec3> result;
    result.push_back(light);

    for(u_int i=1; i<7; i++)
    {
        vec3 temp = light;
        float count = 0.02 * i;
        temp.x += count;
        result.push_back(temp);
    }
    for(u_int i=1; i<7; i++)
    {
        vec3 temp = light;
        float count = 0.02 * i;
        temp.x -= count;
        result.push_back(temp);
    }
    return result;
}

vec3 light = vec3(-0.15,1.80463,-1.04199);

vector<vec3> lights = setlights(light);

vector<pair<ModelTriangle,vector<vec3>>> triangleNormals;
vector<pair<ModelTriangle,ModelTexture>> texturePoints;

void savePPM()
{
    string filepath = std::to_string(fileCount) + ".ppm";
    std::ofstream fout(filepath, std::ofstream::out | std::ofstream::trunc);
    fout << "P6\n";
    fout << "640 480\n";
    fout << "255\n";

    for(int i = 0; i<HEIGHT; i++)
    {
        for(int j = 0; j<WIDTH; j++)
        {
            uint32_t colour = window.getPixelColour(j,i);
            int r = (colour >> 16) & 255;
            int g = (colour >> 8) & 255;
            int b = (colour) & 255;
            fout << (u8) r << (u8) g << (u8) b;
        }
    }
    fout.close();
}

double **malloc2dArray(int width, int height)
{
    double **retult = (double **) malloc(width * sizeof(double *));
    for (int i = 0; i < width; i++)
    {
        retult[i] = (double *) malloc(height * sizeof(double));
    }
    return retult;
}

vector<float> interpolation(float source, float dest, int n)
{
    vector<float> result;
    float s = (dest - source)/(n-1);
    for(int i = 0; i < n; i++)
    {
        result.push_back(source + i * s);
    }
    return result;
}

vector<CanvasPoint> interpolation2(CanvasPoint source, CanvasPoint dest, int n)
{
    vector<CanvasPoint> result;
    float stepx = (dest.x - source.x)/n;
    float stepy = (dest.y - source.y)/n;
    double stepDepth = (dest.depth - source.depth)/n;
    float stepxt = (dest.texturePoint.x - source.texturePoint.x)/n;
    float stepyt = (dest.texturePoint.y - source.texturePoint.y)/n;
    for(int i = 0; i <= n; i++)
    {
        CanvasPoint p;
        p.x = source.x + i * stepx;
        p.y = source.y + i * stepy;
        p.depth = source.depth + i * stepDepth;
        p.texturePoint.x =  source.texturePoint.x + i * stepxt;
        p.texturePoint.y =  source.texturePoint.y + i * stepyt;
        result.push_back(p);
    }
    return result;
}

vector<Colour> readmat(string filename)
{
    ifstream fin;
    fin.open(filename);
    vector<string> colors;
    vector<vec3> rgb;
    vector<Colour> result;
    string line;
    while (getline(fin, line))
    {
        if(line[0] == 'n')
        {
            colors.push_back(line.substr(7,line.size()));
        }
        if(line[0] == 'K')
        {
            float r = stof(line.substr(2,10)) * 255;
            float g = stof(line.substr(11,19)) * 255;
            float b = stof(line.substr(20,line.size())) * 255;
            rgb.push_back(vec3(r,g,b));
        }
    }
    for(u_int i = 0; i<colors.size(); i++)
    {
        Colour colour;
        colour.name = colors[i];
        colour.red = rgb[i].x;
        colour.green = rgb[i].y;
        colour.blue = rgb[i].z;
        result.push_back(colour);
    }
    return result;
}

vector<vec3> readVertices(float s, string filename)
{
    ifstream fin;
    fin.open(filename);
    string line;
    vector<vec3> vertices;
    while (getline(fin, line))
    {
        string* strings = split(line,' ');
        if(strings[0] == "v")
        {
            float x = stof(strings[1])*s;
            float y = stof(strings[2])*s;
            float z = stof(strings[3])*s;
            vertices.push_back(vec3(x,y,z));
        }
    }
    return vertices;
}

vector<vec3> readVertices2(float s, string filename)
{
    ifstream fin;
    fin.open(filename);
    std::string line;
    vector<vec3> vertices;
    while (getline(fin, line))
    {
        std::string* strings = split(line,' ');
        if(line[0] == 'v')
        {
            float x = stof(strings[1])*s - 0.5;
            float y = stof(strings[2])*s + 0.9;
            float z = stof(strings[3])*s - 1.3;
            vertices.push_back(vec3(x,y,z));
        }
    }
    return vertices;
}
vector<vec3> readVertices3(float s, string filename)
{
    ifstream fin;
    fin.open(filename);
    std::string line;
    vector<vec3> vertices;
    while (getline(fin, line))
    {
        std::string* strings = split(line,' ');
        if(strings[0] == "v")
        {
            float x = stof(strings[1])*s - 0.2;
            float y = stof(strings[2])*s + 0.7;
            float z = stof(strings[3])*s - 1;
            vertices.push_back(vec3(x,y,z));
        }
    }
    return vertices;
}

vector<ModelTriangle> readtriangles(string filename, vector<Colour> colours)
{
    fstream fin;
    fin.open(filename);
    std::string line;
    vector<ModelTriangle> results;
    vector<vec3> readvertices = readVertices(SCALETING, filename);
    Colour colour;
    while (getline(fin, line))
    {
        string *strings = split(line, ' ');
        if(strings[0] == "usemtl")
        {
            string colourName = strings[1];
            for (u_int i = 0; i < colours.size(); i++)
            {
                if(colourName == colours[i].name) colour = colours[i];
            }
        }
        if(strings[0] == "f")
        {
            int s1 =  strings[1].find('/');
            int s2 =  strings[2].find('/');
            int s3 =  strings[3].find('/');

            float p1 = stof(strings[1].substr(0,s1)) - 1;
            float p2 = stof(strings[2].substr(0,s2)) - 1;
            float p3 = stof(strings[3].substr(0,s3)) - 1;
            ModelTriangle toAdd = ModelTriangle(readvertices[p1], readvertices[p2], readvertices[p3], colour);
            if(toAdd.colour.name == "Mirror")
            {
                toAdd.name = "mirror";
            }
            else if(toAdd.colour.name == "Yellow")
            {
                toAdd.name = "rightwall";
            }
            else if(toAdd.colour.name == "Metal")
            {
                toAdd.name = "metal";
            }
            else
            {
                toAdd.name = "cornell";
            }
            results.push_back(toAdd);
        }
    }
    return results;
}

vector<ModelTriangle> readtriangles2(string filename)
{
    Colour c;
    c.red = 0;
    c.green = 255;
    c.blue = 255;
    ifstream fin;
    fin.open(filename);
    string line;
    vector<ModelTriangle> results;
    vector<vec3> readvertices = readVertices2(SCALETING2, filename);
    while(getline(fin, line))
    {
        string *strings = split(line, ' ');
        if(strings[0] == "f")
        {
            int s1 =  strings[1].find('/');
            int s2 =  strings[2].find('/');
            int s3 =  strings[3].find('/');
            int p1 = stoi(strings[1].substr(0,s1)) - 1;
            int p2 = stoi(strings[2].substr(0,s2)) - 1;
            int p3 = stoi(strings[3].substr(0,s3)) - 1;
            ModelTriangle toAdd = ModelTriangle(readvertices[p1], readvertices[p2], readvertices[p3], c);
            toAdd.name = "sphere";
            results.push_back(toAdd);
        }
    }
    fin.close();
    return results;
}

vector<TexturePoint> readTexturePoints(float scale,string filename)
{
    fstream fin;
    fin.open(filename);
    std::string line;
    vector<TexturePoint> results;
    TexturePoint p;
    while (getline(fin, line))
    {
        string *strings = split(line, ' ');
        if(strings[0] == "vt")
        {
            p.x =  stof(strings[1]) * scale;
            p.y =  stof(strings[2]) * scale;
            results.push_back(p);
        }
    }
    return results;
}

vector<uint32_t> loadImg(string filename)
{
    string line;
    ifstream fin;
    fin.open(filename);
    getline(fin, line);
    getline(fin, line);
    getline(fin, line);
    int w = stoi(line.substr(0,3));
    int h = stoi(line.substr(4,3));
    getline(fin, line);

    vector<uint32_t> results;
    for(int j =0; j<w*h; j++)
    {
        int r,g,b;
        r = fin.get();
        g = fin.get();
        b = fin.get();
        uint32_t colour = (255<<24) + (r<<16) + (g<<8) + b;
        results.push_back(colour);
    }
    return results;
}

vector<vec3> loadBumpMap(string filename)
{
    string line;
    ifstream fin;
    fin.open(filename);
    getline(fin, line);
    getline(fin, line);
    int w = stoi(line.substr(0,3));
    int h = stoi(line.substr(4,3));
    getline(fin, line);

    vector<vec3> results;
    for(int j =0; j<w*h; j++)
    {
        float x,y,z;
        x = (fin.get());
        y = (fin.get());
        z = (fin.get());
        results.push_back(normalize(vec3(x,y,z)));
    }
    return results;
}

vector<ModelTexture> readTextureIndex(string filename)
{
    vector<ModelTexture> results;
    Colour c;
    c.red = 255;
    c.green =c.blue=0;
    TexturePoint p1;
    p1.x =p1.y=-1;

    if(filename == "sphere.obj" || filename == "cornell-box.obj")
    {
        ifstream fin;
        fin.open(filename);
        std::string line;
        vector<TexturePoint> textPoints = readTexturePoints(300.f,filename);
        while (getline(fin, line))
        {
            string *strings = split(line, ' ');
            if(strings[0] == "f")
            {
                ModelTexture toAdd = ModelTexture(p1, p1, p1,c);
                toAdd.name = "noTexture";
                results.push_back(toAdd);
            }
        }
    }
    else
    {
        ifstream fin;
        fin.open(filename);
        std::string line;
        vector<TexturePoint> textPoints = readTexturePoints(300.f,filename);
        while (getline(fin, line))
        {
            string *strings = split(line, ' ');
            if(strings[0] == "f")
            {
                int s1 =  strings[1].find('/');
                int s2 =  strings[2].find('/');
                int s3 =  strings[3].find('/');
                string point1 = strings[1].substr(s1 + 1,strings[1].length());
                string point2 = strings[2].substr(s2 + 1,strings[2].length());
                string point3 = strings[3].substr(s3 + 1,strings[3].length());
                int p1 = stoi(point1) - 1;
                int p2 = stoi(point2) - 1;
                int p3 = stoi(point3) - 1;
                ModelTexture toAdd = ModelTexture(textPoints[p1], textPoints[p2], textPoints[p3],c);
                toAdd.name = "logo";
                results.push_back(toAdd);
            }
        }
    }
    return results;
}

vector<ModelTexture> readTextureIndexForRayTracer(string filename)
{
    Colour c;
    c.red = 255;
    c.green = c.blue = 0;
    vector<ModelTexture> results;
    ifstream fin;
    fin.open(filename);
    std::string line;
    vector<TexturePoint> textPoints = readTexturePoints(1.0f,filename);
    while (getline(fin, line))
    {
        string *strings = split(line, ' ');
        if(strings[0] == "f")
        {
            int s1 =  strings[1].find('/');
            int s2 =  strings[2].find('/');
            int s3 =  strings[3].find('/');
            string point1 = strings[1].substr(s1 + 1,strings[1].length());
            string point2 = strings[2].substr(s2 + 1,strings[2].length());
            string point3 = strings[3].substr(s3 + 1,strings[3].length());
            int p1 = stoi(point1) - 1;
            int p2 = stoi(point2) - 1;
            int p3 = stoi(point3) - 1;
            results.push_back(ModelTexture(textPoints[p1], textPoints[p2], textPoints[p3],c));
        }
    }
    return results;
}

vector<ModelTriangle> readtriangles3(string filename)
{
    Colour c;
    c.red = c.green = c.blue = 255;
    ifstream fin;
    fin.open(filename);
    string line;
    vector<ModelTriangle> results;
    vector<vec3> readvertices = readVertices3(SCALETING3, filename);
    while(getline(fin, line))
    {
        string *strings = split(line, ' ');
        if(strings[0] == "f")
        {
            int s1 =  strings[1].find('/');
            int s2 =  strings[2].find('/');
            int s3 =  strings[3].find('/');
            string point1 = strings[1].substr(0,s1);
            string point2 = strings[2].substr(0,s2);
            string point3 = strings[3].substr(0,s3);
            int p1 = stoi(point1) - 1;
            int p2 = stoi(point2) - 1;
            int p3 = stoi(point3) - 1;
            ModelTriangle toAdd = ModelTriangle(readvertices[p1], readvertices[p2], readvertices[p3], c);
            toAdd.name = "hackspace";
            results.push_back(toAdd);
        }
    }
    fin.close();
    return results;
}
vector<vec3> readVertices4(float s, string filename)
{
    ifstream fin;
    fin.open(filename);
    std::string line;
    vector<vec3> results;
    while (getline(fin, line))
    {
        std::string* strings = split(line,' ');
        if(strings[0] == "v")
        {
            float x = stof(strings[1])*s;
            float y = stof(strings[2])*s;
            float z = stof(strings[3])*s;
            results.push_back(vec3(x,y,z));
        }
    }
    return results;
}
vector<ModelTriangle> readtrianglesBump(string filename)
{
    Colour c;
    c.red = c.green = c.blue = 255;
    ifstream fin;
    fin.open(filename);
    string line;
    vector<ModelTriangle> results;
    vector<vec3> readvertices = readVertices4(SCALETING, filename);
    while(getline(fin, line))
    {
        string *strings = split(line, ' ');
        if(strings[0] == "f")
        {
            int s1 =  strings[1].find('/');
            int s2 =  strings[2].find('/');
            int s3 =  strings[3].find('/');
            string point1 = strings[1].substr(0,s1);
            string point2 = strings[2].substr(0,s2);
            string point3 = strings[3].substr(0,s3);
            int p1 = stoi(point1) - 1;
            int p2 = stoi(point2) - 1;
            int p3 = stoi(point3) - 1;
            ModelTriangle toAdd = ModelTriangle(readvertices[p1], readvertices[p2], readvertices[p3], c);
            toAdd.name = "rightwall";
            results.push_back(toAdd);
        }
    }
    fin.close();
    return results;
}
void drawLine(CanvasPoint source, CanvasPoint dest, uint32_t colour)
{

    float xd = dest.x - source.x;
    float yd = dest.y - source.y;
    float steps = std::max(abs(xd), abs(yd));
    float xs = xd/steps;
    float ys = yd/steps;

    vector<float> depths = interpolation(source.depth, dest.depth, steps);

    for(int i = 0; i<steps; i++)
    {
        float x = source.x + (xs*i);
        float y = source.y + (ys*i);
        window.setPixelColour(round(x), round(y), colour);
    }
}
void swap(int* a, int*b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

float absolute(float f )
{
    if (f < 0)
        return -f;
    else
        return f;
}

int iPartOfNumber(float f)
{
    return (int)f;
}

int roundNumber(float f)
{
    return iPartOfNumber(f + 0.5) ;
}

//returns fractional part of a number
float fPartOfNumber(float f)
{
    if (f>0)
        return f - iPartOfNumber(f);
    else
        return f - (iPartOfNumber(f)+1);

}

//returns 1 - fractional part of number
float rfPartOfNumber(float f)
{
    return 1 - fPartOfNumber(f);
}

void drawPixel( int x, int y,uint32_t colour, float brightness)
{
    int r = (colour >> 16) & 255;
    int g = (colour >> 8) & 255;
    int b = (colour) & 255;
    uint32_t newcolour = (255<<24) + (int(r * brightness)<<16) + (int(g * brightness)<<8) + int(b * brightness);
    if(x<WIDTH && y<HEIGHT && x>=0 && y>=0)
    {
        window.setPixelColour(x,y,newcolour);
    }
}

void drawWuLines(CanvasPoint source, CanvasPoint dest, uint32_t colour)
{
    float x1 = source.x;
    float y1 = source.y;
    float x2 = dest.x;
    float y2 = dest.y;

    int steep = absolute(y2 - y1) > absolute(x2 - x1);

    if (steep)
    {
        swap(x1, y1);
        swap(x2, y2);
    }
    if (x1 > x2)
    {
        swap( x1, x2);
        swap( y1, y2);
    }

    float gradient;
    if (x2 - x1 == 0.0)
        gradient = 1;
    else
        gradient=(y2 - y1)/(x2 - x1);
    int xe1 = round(x1);
    int xe2 = round(x2);
    float ye1 = y1 + gradient*(xe1  - x1);
    float ye2 = y2 + gradient*(xe2  - x2);

    int xp1 = xe1;
    int xp2 = xe2;

    int yp1 = iPartOfNumber(ye1);
    int yp2 = iPartOfNumber(ye2);

    float interY = ye1 + gradient;

    if(steep)
    {
        drawPixel(yp1, xp1, colour, rfPartOfNumber(ye1) * fPartOfNumber(x1+0.5));
        drawPixel(yp1+1, xp1, colour,fPartOfNumber(ye1) * fPartOfNumber(x1+0.5));
        drawPixel(yp2, xp2, colour, rfPartOfNumber(ye2) * fPartOfNumber(x2+0.5));
        drawPixel(yp2+1, xp2, colour,fPartOfNumber(ye2) * fPartOfNumber(x2+0.5));
    }
    else
    {
        drawPixel(xp1,yp1,colour, rfPartOfNumber(ye1) * fPartOfNumber(x1+0.5));
        drawPixel(xp1,yp1+1,colour,fPartOfNumber(ye1) * fPartOfNumber(x1+0.5));
        drawPixel( xp2,yp2, colour, rfPartOfNumber(ye2) * fPartOfNumber(x2+0.5));
        drawPixel(xp2,yp2+1,  colour,fPartOfNumber(ye2) * fPartOfNumber(x2+0.5));
    }
    if (steep)
    {
        int x;
        for (x = xp1+1 ; x <xp2-1 ; x++)
        {
            drawPixel(iPartOfNumber(interY), x, colour, rfPartOfNumber(interY));
            drawPixel(iPartOfNumber(interY)+1, x,colour, fPartOfNumber(interY));
            interY += gradient;
        }
    }
    else
    {
        int x;
        for (x = xp1+1 ; x <xp2-1 ; x++)
        {
            drawPixel(x, iPartOfNumber(interY),colour,
                      rfPartOfNumber(interY));
            drawPixel(x, iPartOfNumber(interY)+1,colour,
                      fPartOfNumber(interY));
            interY += gradient;
        }
    }
}

void drawTriangle( CanvasPoint p1, CanvasPoint p2, CanvasPoint p3, uint32_t colour)
{
    drawLine(p1, p2, colour);
    drawLine(p2, p3, colour);
    drawLine(p1, p3, colour);
}

void drawFillTriangle(CanvasPoint p1, CanvasPoint p2, CanvasPoint p3, uint32_t colour)
{

    for (int i = 0; i < 2; i++)
    {
        if (p3.y < p2.y)
        {
            CanvasPoint temp = p2;
            p2 = p3;
            p3 = temp;
        }
        if (p2.y < p1.y)
        {
            CanvasPoint temp = p1;
            p1 = p2;
            p2 = temp;
        }
        if (p3.y < p1.y)
        {
            CanvasPoint temp = p1;
            p1 = p3;
            p3 = temp;
        }
    }

    float x = ((p3.x - p1.x)*(p2.y - p1.y))/(p3.y - p1.y);
    CanvasPoint p4 = CanvasPoint(round(p1.x + x), p2.y);

    vector<CanvasPoint> p1to4 = interpolation2(p1,p4,(p4.y-p1.y)+1);
    vector<CanvasPoint> p1to2 = interpolation2(p1,p2,(p2.y-p1.y)+1);
    vector<CanvasPoint> p4to3 = interpolation2(p4,p3,(p3.y-p4.y)+1);
    vector<CanvasPoint> p2to3 = interpolation2(p2,p3,(p3.y-p2.y)+1);

    for (u_int i = 0; i < p1to4.size(); i++)
    {
        drawLine(p1to4[i],p1to2[i],colour);
    }

    for (u_int i = 0; i < p4to3.size(); i++)
    {
        drawLine(p4to3[i],p2to3[i],colour);
    }
}

void draw3DWireframes(vector<CanvasTriangle> triangles)
{

    double **buffer = malloc2dArray(WIDTH, HEIGHT);
    for(u_int y = 0; y < HEIGHT; y++)
    {
        for(u_int x = 0; x < WIDTH; x++)
        {
            buffer[x][y] = -INFINITY;
        }
    }

    for (u_int i = 0; i < triangles.size(); i++)
    {
        Colour c = triangles[i].colour;
        uint32_t colour = (255<<24) + (int(c.red)<<16) + (int(c.green)<<8) + int(c.blue);

        float steps = std::max(abs(triangles[i].vertices[1].x - triangles[i].vertices[0].x), abs(triangles[i].vertices[1].y - triangles[i].vertices[0].y));
        vector<CanvasPoint> p1to2 = interpolation2(triangles[i].vertices[0], triangles[i].vertices[1], (int)steps);
        steps = std::max(abs(triangles[i].vertices[2].x - triangles[i].vertices[0].x), abs(triangles[i].vertices[2].y - triangles[i].vertices[0].y));
        vector<CanvasPoint> p1to3 = interpolation2(triangles[i].vertices[0], triangles[i].vertices[2], (int)steps);
        steps = std::max(abs(triangles[i].vertices[2].x - triangles[i].vertices[1].x), abs(triangles[i].vertices[2].y - triangles[i].vertices[1].y));
        vector<CanvasPoint> p2to3 = interpolation2(triangles[i].vertices[1], triangles[i].vertices[2], (int)steps);

        for (u_int i = 0; i < p1to2.size(); i++)
        {
            if ((int)p1to2[i].x >= 0 && (int)p1to2[i].x < WIDTH && (int)p1to2[i].y >= 0 && (int)p1to2[i].y < HEIGHT)
            {
                if (p1to2[i].depth > buffer[(int)p1to2[i].x][(int)p1to2[i].y])
                {
                    buffer[(int)p1to2[i].x][(int)p1to2[i].y] = p1to2[i].depth;
                    window.setPixelColour((int)p1to2[i].x, (int)p1to2[i].y, colour);
                }
            }
        }

        for (u_int i = 0; i < p1to3.size(); i++)
        {
            if ((int)p1to3[i].x >= 0 && (int)p1to3[i].x < WIDTH && (int)p1to3[i].y >= 0 && (int)p1to3[i].y < HEIGHT)
            {
                if (p1to3[i].depth > buffer[(int)p1to3[i].x][(int)p1to3[i].y])
                {
                    buffer[(int)p1to3[i].x][(int)p1to3[i].y] = p1to3[i].depth;
                    window.setPixelColour((int)p1to3[i].x, (int)p1to3[i].y, colour);
                }
            }
        }

        for (u_int i = 0; i < p2to3.size(); i++)
        {
            if ((int)p2to3[i].x >= 0 && (int)p2to3[i].x < WIDTH && (int)p2to3[i].y >= 0 && (int)p2to3[i].y < HEIGHT)
            {
                if (p2to3[i].depth > buffer[(int)p2to3[i].x][(int)p2to3[i].y])
                {
                    buffer[(int)p2to3[i].x][(int)p2to3[i].y] = p2to3[i].depth;
                    window.setPixelColour((int)p2to3[i].x, (int)p2to3[i].y, colour);
                }
            }
        }
    }
}


void draw3DRasterisedTriangles(vector<CanvasTriangle> canvastriangles, vector<uint32_t> colours)
{
    double **buffer = malloc2dArray(WIDTH, HEIGHT);
    for(u_int y = 0; y < HEIGHT; y++)
    {
        for(u_int x = 0; x < WIDTH; x++)
        {
            buffer[x][y] = -INFINITY;
        }
    }
    for(u_int i = 0; i < canvastriangles.size(); i++)
    {

        CanvasPoint p1 = canvastriangles[i].vertices[0];
        CanvasPoint p2 = canvastriangles[i].vertices[1];
        CanvasPoint p3 = canvastriangles[i].vertices[2];
        Colour c = canvastriangles[i].colour;
        float red = c.red;
        float green = c.green;
        float blue = c.blue;
        uint32_t colour = (255<<24) + (int(red)<<16) + (int(green)<<8) + int(blue);

        for (int i = 0; i < 2; i++)
        {
            if (p3.y < p2.y)
            {
                CanvasPoint temp = p2;
                p2 = p3;
                p3 = temp;
            }
            if (p2.y < p1.y)
            {
                CanvasPoint temp = p1;
                p1 = p2;
                p2 = temp;
            }
            if (p3.y < p1.y)
            {
                CanvasPoint temp = p1;
                p1 = p3;
                p3 = temp;
            }
        }

        double ratio = (p3.y - p2.y)/(p3.y - p1.y);
        double point4Depth = p3.depth - ratio * (p3.depth - p1.depth);
        CanvasPoint p4 = CanvasPoint(p3.x - ratio * (p3.x - p1.x), p3.y - ratio * (p3.y - p1.y), point4Depth);

        p4.texturePoint.x = p3.texturePoint.x - ratio * (p3.texturePoint.x - p1.texturePoint.x);
        p4.texturePoint.y = p3.texturePoint.y - ratio * (p3.texturePoint.y - p1.texturePoint.y);

        vector<CanvasPoint> p1to4 = interpolation2(p1,p4,(p2.y-p1.y)+1);
        vector<CanvasPoint> p1to2 = interpolation2(p1,p2,(p2.y-p1.y)+1);
        vector<CanvasPoint> p4to3 = interpolation2(p4,p3,(p3.y-p4.y)+1);
        vector<CanvasPoint> p2to3 = interpolation2(p2,p3,(p3.y-p2.y)+1);

        for (u_int i = 0; i < p1to4.size(); i++)
        {
            vector<CanvasPoint> points = interpolation2(p1to4[i],p1to2[i],abs(p1to4[i].x-p1to2[i].x)+1);
            for(u_int k = 0; k < points.size(); k++)
            {
                CanvasPoint curr = points[k];
                if(((int)curr.x >= 0) && ((int)curr.x < WIDTH) && ((int)curr.y >= 0) && ((int)curr.y < HEIGHT))
                {
                    if(curr.depth > buffer[(int)curr.x][(int)curr.y])
                    {
                        buffer[(int)curr.x][(int)curr.y] = curr.depth;
                        if(curr.texturePoint.x == -1 && curr.texturePoint.y == -1 )
                        {
                            window.setPixelColour((int)curr.x, (int)curr.y, colour);
                        }
                        else
                        {
                            window.setPixelColour((int)curr.x, (int)curr.y, colours[round(curr.texturePoint.x) + round(curr.texturePoint.y) * 300 ]);
                        }
                    }
                }
            }
        }

        for (u_int i = 0; i < p4to3.size(); i++)
        {
            vector<CanvasPoint> points = interpolation2(p4to3[i],p2to3[i],abs(p4to3[i].x-p2to3[i].x)+1);
            for(u_int k = 0; k < points.size(); k++)
            {
                CanvasPoint curr = points[k];
                if((curr.x >= 0) && ((int)curr.x < WIDTH) && ((int)curr.y >= 0) && ((int)curr.y < HEIGHT))
                {
                    if(curr.depth > buffer[(int)curr.x][(int)curr.y])
                    {
                        buffer[(int)curr.x][(int)curr.y] = curr.depth;
                        if(curr.texturePoint.x == -1 && curr.texturePoint.y == -1 )
                        {
                            window.setPixelColour((int)curr.x, (int)curr.y, colour);
                        }
                        else
                        {
                            window.setPixelColour((int)curr.x, (int)curr.y, colours[round(curr.texturePoint.x) + round(curr.texturePoint.y) * 300 ]);
                        }
                    }
                }
            }
        }

    }
    cout<<3<<endl;
}
void drawTextureTriangles(vector<CanvasTriangle> canvastriangles)
{
    vector<uint32_t> colours = loadImg("texture1.ppm");
    for(u_int i = 0; i < canvastriangles.size(); i++)
    {

        CanvasPoint p1 = canvastriangles[i].vertices[0];
        CanvasPoint p2 = canvastriangles[i].vertices[1];
        CanvasPoint p3 = canvastriangles[i].vertices[2];

        for (int i = 0; i < 2; i++)
        {
            if (p3.y < p2.y)
            {
                CanvasPoint temp = p2;
                p2 = p3;
                p3 = temp;
            }
            if (p2.y < p1.y)
            {
                CanvasPoint temp = p1;
                p1 = p2;
                p2 = temp;
            }
            if (p3.y < p1.y)
            {
                CanvasPoint temp = p1;
                p1 = p3;
                p3 = temp;
            }
        }

        double ratio = (p3.y - p2.y)/(p3.y - p1.y);
        double point4Depth = p3.depth - ratio * (p3.depth - p1.depth);
        CanvasPoint p4 = CanvasPoint(p3.x - ratio * (p3.x - p1.x), p3.y - ratio * (p3.y - p1.y), point4Depth);

        p4.texturePoint.x = p3.texturePoint.x - ratio * (p3.texturePoint.x - p1.texturePoint.x);
        p4.texturePoint.y = p3.texturePoint.y - ratio * (p3.texturePoint.y - p1.texturePoint.y);

        vector<CanvasPoint> p1to4 = interpolation2(p1,p4,(p2.y-p1.y)+1);
        vector<CanvasPoint> p1to2 = interpolation2(p1,p2,(p2.y-p1.y)+1);
        vector<CanvasPoint> p4to3 = interpolation2(p4,p3,(p3.y-p2.y)+1);
        vector<CanvasPoint> p2to3 = interpolation2(p2,p3,(p3.y-p2.y)+1);

        for (u_int i = 0; i < p1to4.size(); i++)
        {
            vector<CanvasPoint> points = interpolation2(p1to4[i],p1to2[i],abs(p1to4[i].x-p1to2[i].x)+1);
            for(u_int k = 0; k < points.size(); k++)
            {
                CanvasPoint curr = points[k];
                if(((int)curr.x >= 0) && ((int)curr.x < WIDTH) && ((int)curr.y >= 0) && ((int)curr.y < HEIGHT))
                {
                    window.setPixelColour((int)curr.x, (int)curr.y, colours[round(curr.texturePoint.x) + round(curr.texturePoint.y) * 300 ]);
                }
            }
        }

        for (u_int i = 0; i < p4to3.size(); i++)
        {
            vector<CanvasPoint> points = interpolation2(p4to3[i],p2to3[i],abs(p4to3[i].x-p2to3[i].x)+1);
            for(u_int k = 0; k < points.size(); k++)
            {
                CanvasPoint curr = points[k];
                if((int)(curr.x >= 0) && ((int)curr.x < WIDTH) && ((int)curr.y >= 0) && ((int)curr.y < HEIGHT))
                {
                    window.setPixelColour((int)curr.x, (int)curr.y, colours[round(curr.texturePoint.x) + round(curr.texturePoint.y) * 300 ]);
                }
            }
        }
    }
}

vector<CanvasTriangle> modelToCanvas(vec3 camera, vector<ModelTriangle> triangles, vector<ModelTexture> textureTriangles, float focalDistance, float canvasScale)
{

    vector<CanvasTriangle> results;

    for(u_int i=0; i<triangles.size(); i++)
    {

        CanvasPoint t1,t2,t3;
        CanvasPoint P1,P2,P3;
        P1.brightness = 0;
        P2.brightness = 0;
        P3.brightness = 0;
        float P1x,P1y,P1z,P2x,P2y,P2z,P3x,P3y,P3z;
        t1.x = triangles[i].vertices[0].x;
        t2.x = triangles[i].vertices[1].x;
        t3.x = triangles[i].vertices[2].x;
        t1.y = triangles[i].vertices[0].y;
        t2.y = triangles[i].vertices[1].y;
        t3.y = triangles[i].vertices[2].y;
        t1.depth = triangles[i].vertices[0].z;
        t2.depth = triangles[i].vertices[1].z;
        t3.depth = triangles[i].vertices[2].z;


        TexturePoint p1 = textureTriangles[i].vertices[0];
        TexturePoint p2 = textureTriangles[i].vertices[1];
        TexturePoint p3 = textureTriangles[i].vertices[2];

        P1x = t1.x - camera.x;
        P1y = t1.y - camera.y;
        P1z = t1.depth - camera.z;

        vec3 camPer1 = vec3(P1x,P1y,P1z);
        vec3 adjustedPos1 = camPer1 * camOrien;

        float p1Screen = focalDistance/-adjustedPos1.z;
        int P1X = (adjustedPos1.x * p1Screen * canvasScale) + WIDTH/2;
        int P1Y = ((1 - adjustedPos1.y) * p1Screen * canvasScale) + HEIGHT/2;
        P1.x = P1X;
        P1.y = P1Y;
        P1.depth = 1/-(adjustedPos1.z);
        P1.texturePoint = p1;


        P2x = t2.x - camera.x;
        P2y = t2.y - camera.y;
        P2z = t2.depth - camera.z;


        vec3 camPer2 = vec3(P2x,P2y,P2z);
        vec3 adjustedPos2 = camPer2 * camOrien;

        float p2Screen = focalDistance/-adjustedPos2.z;
        int P2X = (adjustedPos2.x * p2Screen * canvasScale) + WIDTH/2;
        int P2Y = ((1 - adjustedPos2.y) * p2Screen  * canvasScale) + HEIGHT/2;
        P2.x = P2X;
        P2.y = P2Y;
        P2.depth = 1/-(adjustedPos2.z);
        P2.texturePoint = p2;


        P3x = t3.x - camera.x;
        P3y = t3.y - camera.y;
        P3z = t3.depth - camera.z;

        vec3 camPer3 = vec3(P3x,P3y,P3z);
        vec3 adjustedPos3 = camPer3 * camOrien;

        float p3Screen = focalDistance/-adjustedPos3.z;
        int P3X = (adjustedPos3.x * p3Screen  * canvasScale) + WIDTH/2;
        int P3Y = ((1 - adjustedPos3.y) * p3Screen  * canvasScale) + HEIGHT/2;
        P3.x = P3X;
        P3.y = P3Y;
        P3.depth = 1/-(adjustedPos3.z);
        P3.texturePoint = p3;


        results.push_back(CanvasTriangle(P1,P2,P3,triangles[i].colour));
    }

    return results;
}


vec3 rayDir(int x, int y)
{
    return normalize(vec3(x-WIDTH/2, -(y-HEIGHT/2),-500)-camera)* glm::inverse(camOrien);
}

bool inShadow(vec3 surfacePoint, vec3 lightSource, vector<ModelTriangle> triangles, int index)
{
    bool shadow = false;
    vec3 dir = (lightSource - surfacePoint);
    float distance = length(dir);
    for(u_int i=0; i<triangles.size(); i++)
    {
        vec3 e0 = triangles[i].vertices[1] - triangles[i].vertices[0];
        vec3 e1 = triangles[i].vertices[2] - triangles[i].vertices[0];
        vec3 SPVector = surfacePoint - (triangles[i].vertices[0]);
        mat3 DEMatrix(-normalize(dir), e0, e1);
        vec3 possible = inverse(DEMatrix) * SPVector;
        float x = possible.x;
        float y = possible.y;
        float z = possible.z;
        if ((y>= 0) && (y<=1) && (z>= 0) && (z<=1) && ((y+z)<=1) && ((int)i != index))
        {
            if (x < distance && x > 0.0f)
            {
                shadow = true;
                break;
            }
        }
    }
    return shadow;
}

vec3 normaltoVertices(ModelTriangle trianglex)
{
    return normalize(cross(trianglex.vertices[1]-trianglex.vertices[0],trianglex.vertices[2]-trianglex.vertices[0]));
}

int getIndex(ModelTriangle triangle,vector<ModelTriangle> triangles)
{

    for(u_int i=0; i<triangles.size(); i++)
    {
        if((triangles[i].vertices[0] == triangle.vertices[0]) && (triangles[i].vertices[1] == triangle.vertices[1]) && (triangles[i].vertices[2] == triangle.vertices[2]))
        {
            return i;
        }
    }
    return 0;
}


float compBrightness( vec3 vectorX, vec3 normaltovertices)
{
    vec3 p = light - vectorX;
    float dotProduction = (dot(normaltovertices,normalize(p)));
    if(dotProduction < 0.0f)
        dotProduction = 0.0f;
    float d = length(p);
    float b = 5 * (dotProduction)/(0.5*M_PI* d * d);
    if(b > 1.0f)
        b = 1.0f;
    if(b < 0.2f)
        b = 0.2f;
    return b;
}

pair<ModelTriangle,vector<vec3>> compare(ModelTriangle triangle)
{
    pair<ModelTriangle,vector<vec3>> temp;
    for(u_int i=0; i<triangleNormals.size(); i++)
    {
        if((triangleNormals[i].first.vertices[0] == triangle.vertices[0]) && (triangleNormals[i].first.vertices[1] == triangle.vertices[1]) && (triangleNormals[i].first.vertices[2] == triangle.vertices[2]))
        {
            temp = triangleNormals[i];
            break;
        }
    }
    return temp;
}
pair<ModelTriangle,ModelTexture> compareTP(ModelTriangle triangle)
{
    pair<ModelTriangle,ModelTexture> temp;
    for(u_int i=0; i<texturePoints.size(); i++)
    {
        if((texturePoints[i].first.vertices[0] == triangle.vertices[0]) && (texturePoints[i].first.vertices[1] == triangle.vertices[1]) && (texturePoints[i].first.vertices[2] == triangle.vertices[2]))
        {
            temp = texturePoints[i];
            break;
        }
    }
    return temp;
}


vector<ModelTriangle> removeTriangle(ModelTriangle triangle,vector<ModelTriangle> triangles)
{
    vector<ModelTriangle> result;
    for(u_int i=0; i<triangles.size(); i++)
    {
        if(!((triangles[i].vertices[0] == triangle.vertices[0]) && (triangles[i].vertices[1] == triangle.vertices[1]) && (triangles[i].vertices[2] == triangle.vertices[2])))
        {
            result.push_back(triangles[i]);
        }
    }
    return result;
}

vec3 returnTUandV(vec3 cameraPosition,ModelTriangle triX,vec3 rayDirection)
{
    vec3 v0 = triX.vertices[1] - triX.vertices[0];
    vec3 v1 = triX.vertices[2] - triX.vertices[0];
    vec3 SPVector = cameraPosition - (triX.vertices[0]);
    mat3 DEMatrix(-rayDirection, v0, v1);
    vec3 possible = inverse(DEMatrix) * SPVector;
    return possible;
}

float RandomFloat(float min, float max)
{
    assert(max > min);
    float r = ((float) rand()) / (float) RAND_MAX;
    return (r*(max-min)) + min;
}

vector<ModelTexture> bumptrianglesray = readTextureIndexForRayTracer("rightwall.obj");
vector<ModelTriangle> triangles4 = readtrianglesBump("rightwall.obj");
vector<vec3> surfaceNormals = loadBumpMap("normal.ppm");

RayTriangleIntersection getClosestIntersection(vec3 cameraPosition, vec3 rayDirection,vector<ModelTriangle> triangles,vector<ModelTexture> textureTriangles,vector<uint32_t> colours)
{
    RayTriangleIntersection intersectionP;
    intersectionP.distanceFromCamera = INFINITY;
    for(u_int i = 0; i < triangles.size(); i++)
    {
        ModelTriangle triX = triangles[i];
        vec3 possibleSolution = returnTUandV(cameraPosition,triX,rayDirection);
        float t = possibleSolution.x;
        float u = possibleSolution.y;
        float v = possibleSolution.z;
        vec3 e0 = triX.vertices[1] - triX.vertices[0];
        vec3 e1 = triX.vertices[2] - triX.vertices[0];
        if ((u>= 0) && (u<=1) && (v>= 0) && (v<=1) && ((u+v)<=1))
        {
            if (t < intersectionP.distanceFromCamera)
            {
                if((intersectionP.distanceFromCamera - t) > 0.05)
                {
                    intersectionP.distanceFromCamera = t;
                    intersectionP.intersectedTriangle = triangles[i];
                    intersectionP.intersectionPoint = triangles[i].vertices[0] + (u*e0) + (v*e1);
                }
            }
        }
    }
    if(intersectionP.distanceFromCamera == INFINITY)
    {
        intersectionP.distanceFromCamera = -INFINITY;
    }
    vec3 solution = returnTUandV(cameraPosition,intersectionP.intersectedTriangle,rayDirection);
    float u = solution.y;
    float v = solution.z;
    vec3 normaltovertices = normalize(cross(intersectionP.intersectedTriangle.vertices[1]-intersectionP.intersectedTriangle.vertices[0],intersectionP.intersectedTriangle.vertices[2]-intersectionP.intersectedTriangle.vertices[0]));

    if(intersectionP.distanceFromCamera != -INFINITY)
    {
        if(intersectionP.intersectedTriangle.name == "cornell")
        {
            int ind = getIndex(intersectionP.intersectedTriangle,triangles);
            float brightness = compBrightness(intersectionP.intersectionPoint, normaltovertices);
            if(brightness < 0.2f) brightness = 0.2f;
            float count = 0;
            for(u_int i = 0; i<lights.size(); i++)
            {
                if(inShadow(intersectionP.intersectionPoint, lights[i],triangles,ind))
                {
                    count++;
                }
            }
            float ratio = count/13;
            brightness *= 1 - ratio;
            if(brightness<0.15f)
            {
                brightness = 0.15f;
            }
            if(brightness > 1.0f) brightness = 1.0f;
            Colour c = intersectionP.intersectedTriangle.colour;
            c.red = c.red * brightness;
            c.green = c.green * brightness;
            c.blue = c.blue * brightness;
            intersectionP.intersectedTriangle.colour = c;
        }
        else if(intersectionP.intersectedTriangle.name == "sphere")
        {
            int ind = getIndex(intersectionP.intersectedTriangle,triangles);
            pair<ModelTriangle,vector<vec3>> rightOne = compare(intersectionP.intersectedTriangle);
            vector<vec3> normals = rightOne.second;

            vec3 newNormal = normals[0] + (u*(normals[1]-normals[0])) + (v*(normals[2]-normals[0]));

            float brightness = compBrightness(intersectionP.intersectionPoint,newNormal);
            if(brightness < 0.2f) brightness = 0.2f;
            float count = 0;
            for(u_int i = 0; i<lights.size(); i++)
            {
                if(inShadow(intersectionP.intersectionPoint, lights[i],triangles,ind))
                {
                    count++;
                }
            }
            float ratio = count/13;
            brightness *= 1 - ratio;
            if(brightness<0.15f)
            {
                brightness = 0.15f;
            }
            if(brightness > 1.0f) brightness = 1.0f;
            Colour c = intersectionP.intersectedTriangle.colour;

            c.red = c.red * brightness;
            c.green = c.green * brightness;
            c.blue = c.blue * brightness;

            intersectionP.intersectedTriangle.colour = c;
        }
        else if(intersectionP.intersectedTriangle.name == "hackspace")
        {
            int ind = getIndex(intersectionP.intersectedTriangle,triangles);
            pair<ModelTriangle,ModelTexture> rightTextureP = compareTP(intersectionP.intersectedTriangle);
            ModelTexture textureTriangle = rightTextureP.second;
            TexturePoint p1 = textureTriangle.vertices[0];
            TexturePoint p2 = textureTriangle.vertices[1];
            TexturePoint p3 = textureTriangle.vertices[2];

            vec2 tp1 = vec2(p1.x,p1.y) * 300.0f;
            vec2 tp2 = vec2(p2.x,p2.y) * 300.0f;
            vec2 tp3 = vec2(p3.x,p3.y) * 300.0f;
            vec2 texPoint = tp1 + (u*(tp2-tp1)) + (v*(tp3-tp1));
            uint32_t tempC = colours[round(texPoint.x) + round(texPoint.y) * 300 ];
            int red = (tempC >> 16) & 255;
            int green = (tempC >> 8) & 255;
            int blue = (tempC) & 255;
            float brightness = compBrightness(intersectionP.intersectionPoint, normaltovertices);
            if(brightness < 0.2f) brightness = 0.2f;
            float count = 0;
            for(u_int i = 0; i<lights.size(); i++)
            {
                if(inShadow(intersectionP.intersectionPoint, lights[i],triangles,ind))
                {
                    count++;
                }
            }
            float ratio = count/13;
            brightness *= 1 - ratio;
            if(brightness<0.15f)
            {
                brightness = 0.15f;
            }
            if(brightness > 1.0f) brightness = 1.0f;
            Colour c = intersectionP.intersectedTriangle.colour;
            c.red = red * brightness;
            c.green = green * brightness;
            c.blue = blue * brightness;
            intersectionP.intersectedTriangle.colour = c;
        }
        else if(intersectionP.intersectedTriangle.name == "rightwall")
        {
            int indB = getIndex(intersectionP.intersectedTriangle,triangles4);
            int ind = getIndex(intersectionP.intersectedTriangle,triangles);
            ModelTexture textureTriangle = bumptrianglesray[indB];
            TexturePoint p1 = textureTriangle.vertices[0];
            TexturePoint p2 = textureTriangle.vertices[1];
            TexturePoint p3 = textureTriangle.vertices[2];

            vec2 tp1 = vec2(p1.x,p1.y) * 949.0f;
            vec2 tp2 = vec2(p2.x,p2.y) * 949.0f;
            vec2 tp3 = vec2(p3.x,p3.y) * 949.0f;
            vec3 surfacenormal;
            vec2 texPoint = tp1 + (u*(tp2-tp1)) + (v*(tp3-tp1));

            if((int)texPoint.x >=0 && (int)texPoint.y>=0 && (int)texPoint.y <= 949 && (int)texPoint.x <= 949 )
            {
                surfacenormal = surfaceNormals[(int)texPoint.x + (int)texPoint.y * 949 ];
            }
            else surfacenormal = normaltovertices;
            float brightness = compBrightness(intersectionP.intersectionPoint, surfacenormal);
            if(brightness < 0.2f) brightness = 0.2f;
            float count = 0;
            for(u_int i = 0; i<lights.size(); i++)
            {
                if(inShadow(intersectionP.intersectionPoint, lights[i],triangles,ind))
                {
                    count++;
                }
            }
            float ratio = count/13;
            brightness *= 1 - ratio;
            if(brightness<0.15f)
            {
                brightness = 0.15f;
            }
            if(brightness > 1.0f) brightness = 1.0f;
            Colour c = intersectionP.intersectedTriangle.colour;
            c.red = c.red * brightness;
            c.green = c.green * brightness;
            c.blue = c.blue * brightness;
            intersectionP.intersectedTriangle.colour = c;
        }
        else if(intersectionP.intersectedTriangle.name == "metal")
        {
            float randNum = RandomFloat(-0.1,0.1);
            vec3 reflectionNormal = normaltoVertices(intersectionP.intersectedTriangle);
            vec3 reflectionRay = rayDirection - ((2 + randNum) * reflectionNormal * (dot(rayDirection,reflectionNormal)));
            vector<ModelTriangle> newSet = removeTriangle(intersectionP.intersectedTriangle,triangles);
            RayTriangleIntersection intersectR = getClosestIntersection(intersectionP.intersectionPoint,reflectionRay,newSet,textureTriangles,colours);
            if(intersectR.distanceFromCamera == -INFINITY)
            {
                Colour c = intersectionP.intersectedTriangle.colour;
                c.red = 0;
                c.green = 0;
                c.blue = 0;
                intersectionP.intersectedTriangle.colour = c;
            }
            else
            {
                Colour reflectedColour = intersectR.intersectedTriangle.colour;
                Colour wallcolour = intersectionP.intersectedTriangle.colour;
                wallcolour.red = (0.6*(wallcolour.red)) + (0.4*reflectedColour.red);
                wallcolour.green = (0.6*(wallcolour.green)) + (0.4*reflectedColour.green);
                wallcolour.blue = (0.6*(wallcolour.blue)) + (0.4*reflectedColour.blue);

                intersectionP.intersectedTriangle.colour = wallcolour;
            }
        }
        if(intersectionP.intersectedTriangle.name == "mirror")
        {
            vec3 reflectionNormal = normaltoVertices(intersectionP.intersectedTriangle);
            vec3 reflectionRay = rayDirection - (2.0f * reflectionNormal * (dot(rayDirection,reflectionNormal)));
            vector<ModelTriangle> newSet = removeTriangle(intersectionP.intersectedTriangle,triangles);
            RayTriangleIntersection intersectR = getClosestIntersection(intersectionP.intersectionPoint,reflectionRay,newSet,textureTriangles,colours);
            if(intersectR.distanceFromCamera == -INFINITY)
            {
                Colour c = intersectionP.intersectedTriangle.colour;
                c.red = 0;
                c.green = 0;
                c.blue = 0;
                intersectionP.intersectedTriangle.colour = c;
            }
            else
            {
                Colour reflectedColour = intersectR.intersectedTriangle.colour;
                intersectionP.intersectedTriangle.colour = reflectedColour;
            }
        }
    }

    return intersectionP;
}


void antialiasing(vector<ModelTriangle> triangles,vector<ModelTexture> textureTriangles,vector<uint32_t> colours)
{
    for(int x=0; x<WIDTH; x++)
    {
        for(int y=0; y<HEIGHT; y++)
        {
            vec3 dir = rayDir(x,y);
            RayTriangleIntersection intersectP = getClosestIntersection(camera, dir,triangles,textureTriangles,colours);
            vec3 dir1 = rayDir(x + 0.5,y + 0.5);
            RayTriangleIntersection intersectP1 = getClosestIntersection(camera, dir1,triangles,textureTriangles,colours);
            vec3 dir2 = rayDir(x + 0.5,y - 0.5);
            RayTriangleIntersection intersectP2 = getClosestIntersection(camera, dir2,triangles,textureTriangles,colours);
            vec3 dir3 = rayDir(x - 0.5,y + 0.5);
            RayTriangleIntersection intersectP3 = getClosestIntersection(camera, dir3,triangles,textureTriangles,colours);
            vec3 dir4 = rayDir(x - 0.5,y - 0.5);
            RayTriangleIntersection intersectP4 = getClosestIntersection(camera, dir4, triangles,textureTriangles,colours);

            ModelTriangle trianglex = intersectP.intersectedTriangle;
            ModelTriangle triangle1 = intersectP1.intersectedTriangle;
            ModelTriangle triangle2 = intersectP2.intersectedTriangle;
            ModelTriangle triangle3 = intersectP3.intersectedTriangle;
            ModelTriangle triangle4 = intersectP4.intersectedTriangle;

            Colour c = intersectP.intersectedTriangle.colour;
            Colour c1 = intersectP1.intersectedTriangle.colour;
            Colour c2 = intersectP2.intersectedTriangle.colour;
            Colour c3 = intersectP3.intersectedTriangle.colour;
            Colour c4 = intersectP4.intersectedTriangle.colour;

            float avgred = (c.red + c1.red + c2.red + c3.red + c4.red)/5;
            float avggreen = (c.green + c1.green + c2.green + c3.green + c4.green)/5;
            float avgblue = (c.blue + c1.blue + c2.blue + c3.blue + c4.blue)/5;


            vec3 normaltoVerticesx = normaltoVertices(trianglex);
            vec3 normaltoVertices1 = normaltoVertices(triangle1);
            vec3 normaltoVertices2 = normaltoVertices(triangle2);
            vec3 normaltoVertices3 = normaltoVertices(triangle3);
            vec3 normaltoVertices4 = normaltoVertices(triangle4);

            float brightnessx = compBrightness(intersectP.intersectionPoint, normaltoVerticesx);
            float brightness1 = compBrightness(intersectP1.intersectionPoint, normaltoVertices1);
            float brightness2 = compBrightness(intersectP2.intersectionPoint, normaltoVertices2);
            float brightness3 = compBrightness(intersectP3.intersectionPoint, normaltoVertices3);
            float brightness4 = compBrightness(intersectP4.intersectionPoint, normaltoVertices4);


            float avgB = (brightnessx + brightness1 + brightness2 + brightness3 + brightness4)/5;

            uint32_t avg = (255<<24) + (int(avgred * avgB)<<16) + (int(avggreen * avgB)<<8) + int(avgblue * avgB);

            if(intersectP.distanceFromCamera != -INFINITY)
            {
                window.setPixelColour(x, y, avg);
            }
        }
    }
}
vector<ModelTriangle> culledTriangles(vector<ModelTriangle> triangles)
{
    vector<ModelTriangle> results;
    for (u_int i = 0; i < triangles.size(); i++)
    {
        vec3 t1 = triangles[i].vertices[0];
        vec3 t2 = triangles[i].vertices[1];
        vec3 t3 = triangles[i].vertices[2];
        vec3 normaltovertices = normalize(cross(t2-t1,t3-t1));
        vec3 centroid = vec3((t1.x + t2.x + t3.x)/3,(t1.y + t2.y + t3.y)/3,(t1.z + t2.z + t3.z)/3);
        vec3 camtoCentroid = normalize(camera - centroid);
        float dotProd = (dot(normaltovertices,camtoCentroid));
        if(dotProd >= 0)
        {
            results.push_back(triangles[i]);
        }
    }
    return results;
}

void culling(vector<CanvasTriangle> triangles)
{

    double **buffer = malloc2dArray(WIDTH, HEIGHT);
    for(u_int y = 0; y < HEIGHT; y++)
    {
        for(u_int x = 0; x < WIDTH; x++)
        {
            buffer[x][y] = -INFINITY;
        }
    }
    for (u_int i = 0; i < triangles.size(); i++)
    {
        Colour c = triangles[i].colour;
        uint32_t colour = (255<<24) + (int(c.red)<<16) + (int(c.green)<<8) + int(c.blue);

        float numberOfSteps = std::max(abs(triangles[i].vertices[1].x - triangles[i].vertices[0].x), abs(triangles[i].vertices[1].y - triangles[i].vertices[0].y));
        vector<CanvasPoint> points1to2 = interpolation2(triangles[i].vertices[0], triangles[i].vertices[1], (int)numberOfSteps);
        numberOfSteps = std::max(abs(triangles[i].vertices[2].x - triangles[i].vertices[0].x), abs(triangles[i].vertices[2].y - triangles[i].vertices[0].y));
        vector<CanvasPoint> points1to3 = interpolation2(triangles[i].vertices[0], triangles[i].vertices[2], (int)numberOfSteps);
        numberOfSteps = std::max(abs(triangles[i].vertices[2].x - triangles[i].vertices[1].x), abs(triangles[i].vertices[2].y - triangles[i].vertices[1].y));
        vector<CanvasPoint> points2to3 = interpolation2(triangles[i].vertices[1], triangles[i].vertices[2], (int)numberOfSteps);

        for (u_int i = 0; i < points1to2.size(); i++)
        {
            if ((int)points1to2[i].x >= 0 && (int)points1to2[i].x < WIDTH && (int)points1to2[i].y >= 0 && (int)points1to2[i].y < HEIGHT)
            {
                if (points1to2[i].depth > buffer[(int)points1to2[i].x][(int)points1to2[i].y])
                {
                    buffer[(int)points1to2[i].x][(int)points1to2[i].y] = points1to2[i].depth;
                    window.setPixelColour((int)points1to2[i].x,(int)points1to2[i].y, colour);
                }
            }
        }

        for (u_int i = 0; i < points1to3.size(); i++)
        {
            if ((int)points1to3[i].x >= 0 && (int)points1to3[i].x < WIDTH && (int)points1to3[i].y >= 0 && (int)points1to3[i].y < HEIGHT)
            {
                if (points1to3[i].depth > buffer[(int)points1to3[i].x][(int)points1to3[i].y])
                {
                    buffer[(int)points1to3[i].x][(int)points1to3[i].y] = points1to3[i].depth;
                    window.setPixelColour((int)points1to3[i].x, (int)points1to3[i].y, colour);
                }
            }
        }

        for (u_int i = 0; i < points2to3.size(); i++)
        {
            if ((int)points2to3[i].x >= 0 && (int)points2to3[i].x < WIDTH && (int)points2to3[i].y >= 0 && (int)points2to3[i].y < HEIGHT)
            {
                if (points2to3[i].depth > buffer[(int)points2to3[i].x][(int)points2to3[i].y])
                {
                    buffer[(int)points2to3[i].x][(int)points2to3[i].y] = points2to3[i].depth;
                    window.setPixelColour((int)points2to3[i].x, (int)points2to3[i].y, colour);
                }
            }
        }
    }
}


void computeRayT(vector<ModelTriangle> triangles,vector<ModelTexture> textureTriangles,vector<vec3> whitelights,vector<uint32_t> colours, vec3 camera)
{
    for(int x=0; x<WIDTH; x++)
    {
        for(int y=0; y<HEIGHT; y++)
        {
            vec3 dir = rayDir(x,y);
            RayTriangleIntersection intersectP = getClosestIntersection(camera, dir,triangles,textureTriangles,colours);
            ModelTriangle trianglex = intersectP.intersectedTriangle;
            Colour c = trianglex.colour;
            uint32_t colour = (255<<24) + (int(c.red )<<16) + (int(c.green)<<8) + int(c.blue);
            if(intersectP.distanceFromCamera != -INFINITY)
            {
                window.setPixelColour(x, y, colour);
            }
        }
    }
}


void initializePair(vector<ModelTriangle> triangles)
{
    for(u_int i = 0; i<triangles.size(); i++)
    {
        vector<vec3> normals;
        for(u_int j = 0; j<3; j++)
        {
            vec3 test1 = triangles[i].vertices[j];
            vec3 avgNormal = vec3(0,0,0);
            int count = 0;
            for(u_int k = 0; k<triangles.size(); k++)
            {
                for(u_int l = 0; l<3; l++)
                {
                    vec3 test2 = triangles[k].vertices[l];
                    if(test1 == test2)
                    {
                        vec3 normaltoVertices2 = normaltoVertices(triangles[k]);
                        avgNormal += normaltoVertices2;
                        count++;
                    }
                }
            }
            avgNormal = avgNormal / ((float) count);
            normals.push_back(avgNormal);
        }
        triangleNormals.push_back(std::make_pair(triangles[i], normals));
    }
}
void initializeTexturePoints(vector<ModelTriangle> triangles,vector<ModelTexture> Texturetriangles)
{
    for(u_int i = 0; i<triangles.size(); i++)
    {
        texturePoints.push_back(std::make_pair(triangles[i], Texturetriangles[i]));
    }
}

void lookAt (vec3 camera, vec3 point)
{
    vec3 vertical = vec3(0, 1, 0);
    vec3 forward = normalize(camera - point);
    vec3 right = normalize(cross(vertical, forward));
    vec3 up = normalize(cross(forward, right));
    mat3 newCamOrien = mat3(right, up,forward);
    camOrien = inverse(transpose(newCamOrien));
}


vector<Colour> colours = readmat("cornell-box.mtl");
vector<ModelTriangle> triangles = readtriangles("cornell-box.obj", colours);
vector<ModelTriangle> triangles2 = readtriangles2("sphere.obj");
vector<ModelTriangle> triangles3 = readtriangles3("logo.obj");
vector<ModelTexture> textureTriangles = readTextureIndex("logo.obj");
vector<ModelTexture> textureTriangles2 = readTextureIndex("sphere.obj");
vector<ModelTexture> textureTriangles3 = readTextureIndex("cornell-box.obj");
vector<ModelTexture> textureTrianglesRay = readTextureIndexForRayTracer("logo.obj");
vector<uint32_t> coloursTextures = loadImg("texture1.ppm");


void moveVertices(float x)
{
    for(u_int i=0; i<triangles.size(); i++)
    {
        triangles[i].vertices[0].y += x;
        triangles[i].vertices[1].y += x;
        triangles[i].vertices[2].y += x;
    }

    for(u_int i=0; i<triangles2.size(); i++)
    {
        triangles2[i].vertices[0].y += x;
        triangles2[i].vertices[1].y += x;
        triangles2[i].vertices[2].y += x;
    }

    for(u_int i=0; i<triangles3.size(); i++)
    {
        triangles3[i].vertices[0].y += x;
        triangles3[i].vertices[1].y += x;
        triangles3[i].vertices[2].y += x;
    }
}

void draw()
{
    initializeTexturePoints(triangles3,textureTrianglesRay);
    initializePair(triangles2);
    vector<ModelTexture> completeTextures = textureTriangles3;

    for(u_int i = 0; i<textureTriangles2.size(); i++)
    {
        completeTextures.push_back(textureTriangles2[i]);
    }
    for(u_int i = 0; i<textureTriangles.size(); i++)
    {
        completeTextures.push_back(textureTriangles[i]);
    }
    vector<ModelTriangle> complete = triangles;
    for(u_int i = 0; i<triangles2.size(); i++)
    {
        complete.push_back(triangles2[i]);
    }
    for(u_int i = 0; i<triangles3.size(); i++)
    {
        complete.push_back(triangles3[i]);
    }
    window.clearPixels();
    if (mode == 1)
    {
        vector<CanvasTriangle> canvas = modelToCanvas(camera, triangles,textureTriangles3, 5, 100);
        vector<CanvasTriangle> canvas2 = modelToCanvas(camera, triangles2,textureTriangles2, 5, 100);
        vector<CanvasTriangle> canvas3 = modelToCanvas(camera, triangles3,textureTriangles, 5, 100);
        vector<CanvasTriangle> completeM = canvas;
        for(u_int i = 0; i<canvas2.size(); i++)
        {
            completeM.push_back(canvas2[i]);
        }
        for(u_int i = 0; i<canvas3.size(); i++)
        {
            completeM.push_back(canvas3[i]);
        }
        draw3DWireframes(completeM);
    }
    else if (mode == 2)
    {
        vector<CanvasTriangle> canvas = modelToCanvas(camera, triangles,textureTriangles3, 5, 100);
        vector<CanvasTriangle> canvas2 = modelToCanvas(camera, triangles2,textureTriangles2,  5, 100);
        vector<CanvasTriangle> canvas3 = modelToCanvas(camera, triangles3,textureTriangles, 5, 100);

        vector<CanvasTriangle> completeM = canvas;
        for(u_int i = 0; i<canvas2.size(); i++)
        {
            completeM.push_back(canvas2[i]);
        }
        for(u_int i = 0; i<canvas3.size(); i++)
        {
            completeM.push_back(canvas3[i]);
        }
        vector<uint32_t> colours = loadImg("texture1.ppm");
        cout<<colours.size();
        draw3DRasterisedTriangles(completeM,colours);
    }

    else if (mode == 3)
    {
        vector<ModelTriangle> complete2 = culledTriangles(complete);
        computeRayT(complete2,textureTrianglesRay,lights,coloursTextures,camera);
    }
    else if (mode == 4)
    {
        antialiasing(complete,textureTrianglesRay,coloursTextures);
    }
    else if (mode == 5)
    {
        vector<ModelTriangle> complete2 = culledTriangles(complete);
        vector<CanvasTriangle> culledTrianglesx = modelToCanvas(camera, complete2,completeTextures, 5, 100);
        culling(culledTrianglesx);
    }
    else if (mode == 6)
    {
        vector<CanvasTriangle> canvas = modelToCanvas(camera, triangles,textureTriangles3, 5, 100);
        vector<CanvasTriangle> canvas2 = modelToCanvas(camera, triangles2,textureTriangles2,  5, 100);
        vector<CanvasTriangle> canvas3 = modelToCanvas(camera, triangles3,textureTriangles, 5, 100);
        vector<CanvasTriangle> complete = canvas;
        for(u_int i = 0; i<canvas2.size(); i++)
        {
            complete.push_back(canvas2[i]);
        }
        for(u_int i = 0; i<canvas3.size(); i++)
        {
            complete.push_back(canvas3[i]);
        }
        for(u_int i = 0; i<complete.size(); i++)
        {
            CanvasPoint point1 = complete[i].vertices[0];
            CanvasPoint point2 = complete[i].vertices[1];
            CanvasPoint point3 = complete[i].vertices[2];
            Colour c = complete[i].colour;
            float red = c.red;
            float green = c.green;
            float blue = c.blue;
            uint32_t colour = (255<<24) + (int(red)<<16) + (int(green)<<8) + int(blue);
            drawWuLines(point1, point2, colour);
            drawWuLines(point2, point3, colour);
            drawWuLines(point1, point3, colour);
        }
    }
}

void animate(float displacement)
{
    float u = sqrt(2 * 9.81 * displacement);
    float time = (2*u)/9.81;
    for(float i=0; i<time; i+=0.03)
    {
        float displaceby = (u * i)+(-9.81 * i * i)/2;
        moveVertices(displaceby);
        draw();
        window.renderFrame();
        // savePPM();
        // cout<<"ImageSaved"<<endl;
        // fileCount++;
        moveVertices(-displaceby);
    }
}

void jump()
{
    for(float a = 3; a >= 0; a-=0.5)
    {
        animate(a);
    }
}

int main(int argc, char* argv[])
{
    SDL_Event e;
    draw();
    while(true)
    {
        if(window.pollForInputEvents(&e))
        {
            handleEvent(e);
        }
        window.renderFrame();
    }
}

void rotateX(float angle)
{
    mat3 rotationMatrix(vec3(1.0,0.0,0.0),
                        vec3(0,cos(angle),-sin(angle)),
                        vec3(0,sin(angle),cos(angle)));
    camOrien = camOrien * rotationMatrix;
}

void rotateY(float angle)
{
    mat3 rotationMatrix(vec3(cos(angle),0,sin(angle)),
                        vec3(0,1,0),
                        vec3(-sin(angle),0,cos(angle)));
    camOrien = camOrien * rotationMatrix;
}

void rotateZ(float angle)
{
    mat3 rotationMatrix(vec3(cos(angle),-sin(angle),0),
                        vec3(sin(angle),cos(angle),0),
                        vec3(0,0,1));
    camOrien = camOrien * rotationMatrix;
}


void handleEvent(SDL_Event event)
{
    if(event.type != SDL_KEYDOWN){
        return;
    }
    if(event.key.keysym.sym == SDLK_LEFT)
    {
        cout << "camera left" << endl;
        camera.x -= 0.1;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_RIGHT)
    {
        cout << "camera right" << endl;
        camera.x += 0.1;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_UP)
    {
        cout << "camera up" << endl;
        camera.y += 0.1;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_DOWN)
    {
        cout << "camera down" << endl;
        camera.y -= 0.1;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_0)
    {
        cout << "camera forward" << endl;
        camera.z -= 0.1;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_9)
    {
        cout << "camera backward" << endl;
        camera.z += 0.1;
        draw();
    }

    else if(event.key.keysym.sym == SDLK_a)
    {
        light.x -= 0.1;
        cout << "light left" << endl;
        draw();
        // savePPM();
        // fileCount++;
        // cout<<"ImageSaved"<<endl;
    }
    else if(event.key.keysym.sym == SDLK_d)
    {
        light.x += 0.1;
        cout << "light right" << endl;
        draw();
        // savePPM();
        // fileCount++;
        // cout<<"ImageSaved"<<endl;
    }
    else if(event.key.keysym.sym == SDLK_w)
    {
        light.y += 0.1;
        cout << "light up" << endl;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_s)
    {
        light.y -= 0.1;
        cout << "light down" << endl;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_q)
    {
        light.z -= 0.1;
        cout << "light forward" << endl;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_e)
    {
        light.z += 0.1;
        cout << "light backward" << endl;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_p)
    {
        savePPM();
        cout<<"image saved"<<endl;
        fileCount++;
    }

    else if(event.key.keysym.sym == SDLK_1)
    {
        mode = 1;
        cout << "wireframe mode" << endl;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_2)
    {
        mode = 2;
        cout << "rasterizer mode" << endl;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_3)
    {
        mode = 3;
        cout << "raytracer mode" << endl;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_4)
    {
        mode = 4;
        cout << "antialiasing mode" << endl;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_5)
    {
        mode = 5;
        cout << "culling mode" << endl;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_6)
    {
        mode = 6;
        cout << "wu lines mode" << endl;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_c)
    {
        window.clearPixels();
    }
    else if(event.key.keysym.sym == SDLK_x)
    {
        rotateX(angle);
        cout << "rotate x" << endl;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_y)
    {
        rotateY(-angle);
        cout << "rotate y" << endl;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_z)
    {
        rotateZ(angle);
        cout << "rotate z" << endl;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_l)
    {
        lookAt(camera, light);
        draw();
    }
    else if(event.key.keysym.sym == SDLK_SPACE)
    {
        camera = vec3(0,0,6);
        camOrien = mat3();
        cout << "reset" << endl;
        draw();
    }
    else if(event.key.keysym.sym == SDLK_7)
    {
        cout << "animation" << endl;
        jump();
    }


}
// ffmpeg -framerate 40 -i rasteriser_frames/%d.ppm -c:v libx264 -profile:v high -crf 18 -pix_fmt yuv420p -q 4 rasteriser.mp4
