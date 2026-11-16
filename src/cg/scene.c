#include "cg/scene.h"

#include <GL/freeglut.h>
#include <stdlib.h>

SceneNode *scene_node_create(const char *name, Geometry *geometry) {
    SceneNode *node = calloc(1, sizeof(*node));
    if (node) {
        node->local = node->world = mat4_identity();
        node->name = name;
        node->geometry = geometry;
    }
    return node;
}
bool scene_node_add(SceneNode *parent, SceneNode *child) {
    if (!parent || !child || child->parent)
        return false;
    for (SceneNode *p = parent; p; p = p->parent)
        if (p == child)
            return false;
    child->parent = parent;
    child->next_sibling = parent->first_child;
    parent->first_child = child;
    return true;
}
void scene_node_destroy(SceneNode *root) {
    if (!root)
        return;
    SceneNode *child = root->first_child;
    while (child) {
        SceneNode *next = child->next_sibling;
        scene_node_destroy(child);
        child = next;
    }
    if (root->geometry)
        root->geometry->destroy(root->geometry);
    free(root);
}
void scene_update(SceneNode *root, Mat4 parent_world) {
    if (!root)
        return;
    root->world = mat4_mul(parent_world, root->local);
    for (SceneNode *child = root->first_child; child; child = child->next_sibling)
        scene_update(child, root->world);
}
void scene_draw(SceneNode *root) {
    if (!root)
        return;
    if (root->geometry) {
        glPushMatrix();
        glMultMatrixf(root->world.m);
        root->geometry->draw(root->geometry); /* Actual polymorphic dispatch. */
        glPopMatrix();
    }
    for (SceneNode *child = root->first_child; child; child = child->next_sibling)
        scene_draw(child);
}
AABB scene_bounds(const SceneNode *node) {
    AABB bounds = node->geometry->bounds;
    bounds.transform(&bounds, node->world);
    return bounds;
}
