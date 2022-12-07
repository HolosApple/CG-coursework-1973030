#include <glm/glm.hpp>
// #include "Colour.h"
#include <string>
#include <iostream>
#include <string>


class ModelTexture
{
  public:
    TexturePoint vertices[3];
    Colour colour;
    std::string name;

    ModelTexture()
    {
    }

    ModelTexture(TexturePoint v0, TexturePoint v1,TexturePoint v2, Colour trigColour)
    {
      vertices[0] = v0;
      vertices[1] = v1;
      vertices[2] = v2;
      colour = trigColour;
      name = "";
    }
};

std::ostream& operator<<(std::ostream& os, const ModelTexture& triangle)
{
    os << "(" << triangle.vertices[0].x << ", " << triangle.vertices[0].y << ")" << std::endl;
    os << "(" << triangle.vertices[1].x << ", " << triangle.vertices[1].y << ")" << std::endl;
    os << "(" << triangle.vertices[2].x << ", " << triangle.vertices[2].y << ")" << std::endl;
    os << std::endl;
    return os;
}
