#ifndef WORLD_HPP
#define WORLD_HPP

#include "/usr/local/include/GLFW/glfw3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <unordered_set>
#include <vector>
#include "SceneObjects/SceneParams.h"
#include "SceneObject.hpp"
#include "TreeObject.h"
#include "RockObject.h"

namespace ParamWorld {

struct Square {
    int x;
    int z;

    Square(int xx, int zz) : x(xx), z(zz) {}

    bool operator==(const Square s) const {
        return s.x == x && s.z == z;
    }
};
}

namespace std {
template<>
struct hash<ParamWorld::Square> {
    size_t operator()(const ParamWorld::Square &sq) const {
        return std::hash<int>()(sq.x) ^ std::hash<int>()(sq.z);
    }
};
}

namespace ParamWorld {

class World {
public:
    void Render(glm::mat4 Perspective, glm::mat4 View);
    void updateExploredSquares(glm::vec3 playerPosition, float theta);
    World(float worldExtent, GLuint matID);

private:
    void AddMoreThings(float x, float z, float horizontalAngle);

    // The parameters that generate the new parts of the world.
    SceneParams sceneParams;
    // All objects (RockObjects, TreeObjects, etc.) in the world.
    std::vector<SceneObject> worldObjects;
    // Model representing the floor.
    Ground g;
    // The set of all grid spaces that have been explored in this world. Kept at TODO intervals.
    std::unordered_set<Square> exploredSquares;

    GLuint MatrixID;

};
} // end namespace



#endif