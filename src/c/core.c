#include <math.h>

typedef struct {
    float x;
    float y;
    float r;
    unsigned char type;
    unsigned char flags;
    unsigned short eatenBy;
    unsigned int age;
    float boostX;
    float boostY;
    float boost;
} Cell;

typedef struct {
    float x;
    float y;
    void* tl;
    void* tr;
    void* bl;
    void* br;
    unsigned short count;
    unsigned short indices; // placeholder
} QuadNode;

#define IS_PLAYER(type) type <= 250
#define IS_DEAD(type) type == 251
#define IS_ALIVE(type) type != 251
#define IS_MOTHER_CELL(type) type == 252
#define IS_VIRUS(type) type == 253
#define IS_PELLET(type) type == 254
#define NOT_PELLET(type) type != 254
#define IS_EJECTED(type) type == 255

#define EXIST_BIT 0x1
#define UPDATE_BIT 0x2
#define INSIDE_BIT 0x4
// Removed dead bit to type 251
#define AUTOSPLIT_BIT 0x10
#define REMOVE_BIT 0x20
#define MERGE_BIT 0x40
#define POP_BIT 0x80

#define CLEAR_BITS 0x49

// extern console_log(unsigned short i);

void update(Cell cells[], unsigned short* ptr, float dt_multi,
    float auto_size, float decay_multi, float decay_min,
    float l, float r, float b, float t) {

    Cell* cell = &cells[*ptr];

    // Clear cell data 
    while (cell->flags & REMOVE_BIT) {
        cell->x = 0;
        cell->y = 0;
        cell->r = 0;
        cell->type = 0;
        cell->flags = 0;
        cell->eatenBy = 0;
        cell->age = 0;
        cell->boostX = 0.f;
        cell->boostY = 0.f;
        cell->boost = 0.f;

        cell = &cells[*++ptr]; // increment to next index
    }

    if (!*ptr) return;

    // Player cells
    while (*ptr) {
        // Increment age, clear bits
        cell->age++;
        cell->flags &= CLEAR_BITS;

        // Boost cell
        if (cell->boost > 1) {
            float d = cell->boost / 9.0f * dt_multi;
            cell->x += cell->boostX * d;
            cell->y += cell->boostY * d;
            cell->flags |= UPDATE_BIT;
            cell->boost -= d;
        }

        // Decay and set the autosplit bit for player cells
        if (cell->r > decay_min) {
            cell->r -= cell->r * decay_multi * dt_multi / 50.0f;
            cell->flags |= UPDATE_BIT;
        }
        if (auto_size && cell->r > auto_size) cell->flags |= AUTOSPLIT_BIT;

        // Bounce and clamp the cells in the box
        unsigned char bounce = cell->boost > 1;
        float hr = cell->r / 2;
        if (cell->x < l + hr) {
            cell->x = l + hr;
            cell->flags |= UPDATE_BIT;
            if (bounce) cell->boostX = -cell->boostX;
        } 
        if (cell->x > r - hr) {
            cell->x = r - hr;
            cell->flags |= UPDATE_BIT;
            if (bounce) cell->boostX = -cell->boostX;
        }
        if (cell->y > t - hr) {
            cell->y = t - hr;
            cell->flags |= UPDATE_BIT;
            if (bounce) cell->boostY = -cell->boostY;
        }
        if (cell->y < b + hr) {
            cell->y = b + hr;
            cell->flags |= UPDATE_BIT;
            if (bounce) cell->boostY = -cell->boostY;
        }
        
        cell = &cells[*++ptr]; // increment to next index
    }
}

int is_safe(Cell* cells, float x, float y, float r, QuadNode* root, void** node_stack_pointer) {
    unsigned int stack_counter = 1;
    node_stack_pointer[0] = root;
    QuadNode* curr = root;

    int counter = 0;
    float dx;
    float dy;

    while (stack_counter > 0) {
        // Has leaves, push leaves, if they intersect, to stack
        if (curr->tl) {
            if (y - r < curr->y) {
                if (x + r > curr->x)
                    node_stack_pointer[stack_counter++] = curr->br;
                if (x - r < curr->x)
                    node_stack_pointer[stack_counter++] = curr->bl;
            }
            if (y + r > curr->y) {
                if (x + r > curr->x)
                    node_stack_pointer[stack_counter++] = curr->tr;
                if (x - r < curr->x)
                    node_stack_pointer[stack_counter++] = curr->tl;
            }
        }

        for (unsigned int i = 0; i < curr->count; i++) {
            Cell* cell = &cells[*(&curr->indices + i)];
            if (cell->type > 253) continue;
            dx = cell->x - x;
            dy = cell->y - y;
            counter++;
            if (dx * dx + dy * dy < (r + cell->r) * (r + cell->r)) return -counter;
        }

        // Pop from the stack
        curr = (QuadNode*) node_stack_pointer[--stack_counter];
    }
    return counter;
}

#define PHYSICS_NON 0
#define PHYSICS_EAT 1
#define PHYSICS_COL 2

#define SKIP_RESOLVE_BITS 0xa4

void resolve(Cell cells[],
    unsigned short* ptr,
    QuadNode* root, void** node_stack_pointer, 
    unsigned int noMergeDelay, unsigned int noColliDelay, 
    float eatOverlap, float eatMulti, float virusMaxSize, unsigned int removeTick) {

    while (*ptr) {

        Cell* cell = &cells[*ptr++];

        unsigned char flags = cell->flags;

        // Cell not exist, to be removed, popped, or inside another cell
        if (flags & SKIP_RESOLVE_BITS) {
            cell++;
            continue;
        }

        if (IS_DEAD(cell->type)) {
            if (cell->age > removeTick) {
                cell->flags |= REMOVE_BIT;
                cell->eatenBy = 0;
            }
            continue;
        }

        unsigned int stack_counter = 1;
        node_stack_pointer[0] = root;
        QuadNode* curr = root;

        while (stack_counter > 0) {
            // Has leaves, push leaves, if they intersect, to stack
            if (curr->tl) {
                if (cell->y - cell->r < curr->y) {
                    if (cell->x + cell->r > curr->x)
                        node_stack_pointer[stack_counter++] = curr->br;
                    if (cell->x - cell->r < curr->x)
                        node_stack_pointer[stack_counter++] = curr->bl;
                }
                if (cell->y + cell->r > curr->y) {
                    if (cell->x + cell->r > curr->x)
                        node_stack_pointer[stack_counter++] = curr->tr;
                    if (cell->x - cell->r < curr->x)
                        node_stack_pointer[stack_counter++] = curr->tl;
                }
            }

            for (unsigned int i = 0; i < curr->count; i++) {
                unsigned short other_index = *(&curr->indices + i);
                Cell* other = &cells[other_index];
                if (cell == other) continue; // Same cell
                if (cell->r < other->r) continue; // Skip double check
                else if (cell->r == other->r && cell > other) continue;

                unsigned char other_flags = other->flags;

                // Other cell doesn't exist?! or removed
                if (!(other_flags & EXIST_BIT) || 
                    (other_flags & REMOVE_BIT)) continue;
                unsigned char action = PHYSICS_NON;

                // Check player x player
                if (IS_PLAYER(cell->type)) {
                    if (IS_PLAYER(other->type) && cell->type == other->type) {
                        if (IS_DEAD(cell->type)) {
                            if (IS_DEAD(other->type)) action = PHYSICS_COL;
                        } else {
                            if (IS_DEAD(other->type)) action = PHYSICS_EAT;
                            else if (cell->age < noColliDelay || other->age < noColliDelay)
                                action = PHYSICS_NON;
                            else if ((flags & MERGE_BIT) && (other_flags & MERGE_BIT))
                                action = PHYSICS_EAT;
                            else if (!(flags & INSIDE_BIT)) action = PHYSICS_COL;
                        }
                    // Dead cell can not eat others
                    } else if (IS_ALIVE(cell->type)) action = PHYSICS_EAT;
                } else if (IS_VIRUS(cell->type) && IS_EJECTED(other->type)) {
                    // Virus can only eat ejected cell
                    action = PHYSICS_EAT;
                } else if (IS_EJECTED(cell->type) && IS_EJECTED(other->type)) {
                    // Ejected only collide with ejected cell
                    action = PHYSICS_COL;
                } else if (IS_MOTHER_CELL(cell->type)) {
                    // Mother cell eats everything?
                    action = PHYSICS_EAT;
                }

                if (action == PHYSICS_NON) continue;

                float dx = other->x - cell->x;
                float dy = other->y - cell->y;
                float d = sqrtf(dx * dx + dy * dy);
                float r1 = cell->r;
                float r2 = other->r;

                if (action == PHYSICS_COL) {
                    float m = r1 + r2 - d;
                    if (m <= 0) continue;
                    if (!d) {
                        d = 1.f;
                        dx = 1.f;
                        dy = 0.f;
                    } else {
                        dx /= d; 
                        dy /= d;
                    }
                    
                    // Other cell is inside this cell, mark it
                    if (d + r2 < r1) other->flags |= INSIDE_BIT;

                    float a = r1 * r1;
                    float b = r2 * r2;
                    float aM = b / (a + b);
                    float bM = a / (a + b);
                    cell->x -= dx * (m < r1 ? m : r1) * aM; // * 0.8f;
                    cell->y -= dy * (m < r1 ? m : r1) * aM; // * 0.8f;
                    other->x += dx * (m < r2 ? m : r2) * bM; // * 0.8f;
                    other->y += dy * (m < r2 ? m : r2) * bM; // * 0.8f;
                    // Mark the cell as updated
                    cell->flags |= UPDATE_BIT;
                    other->flags |= UPDATE_BIT;

                } else if (action == PHYSICS_EAT) {
                    if ((cell->type == other->type || 
                         cell->r > other->r * eatMulti) && 
                            d < cell->r - other->r / eatOverlap) {
                        cell->r = sqrtf(r1 * r1 + r2 * r2);
                        if (IS_VIRUS(other->type) || IS_MOTHER_CELL(other->type)) {
                            other->eatenBy = 0;
                        } else {
                            other->eatenBy = ((unsigned int) cell) >> 5;
                        }
                        other->flags |= REMOVE_BIT;
                        if (IS_PLAYER(cell->type) && IS_EJECTED(other->type)) {
                            float ratio = other->r / (cell->r + 100.f);
                            cell->boost += ratio * 0.02f * other->boost;
                            float bx = cell->boostX + ratio * 0.02f * other->boostX;
                            float by = cell->boostY + ratio * 0.02f * other->boostY;
                            float norm = sqrt(bx * bx + by * by);
                            cell->boostX = bx / norm;
                            cell->boostY = by / norm;
                        }
                        if (IS_VIRUS(other->type) || IS_MOTHER_CELL(other->type))
                            cell->flags |= 0x80; // Mark this cell as popped
                        if (IS_VIRUS(cell->type) && IS_EJECTED(other->type) && cell->r >= virusMaxSize) {
                            cell->flags |= 0x80; // Mark this as virus to be split
                            cell->boostX = other->boostX;
                            cell->boostY = other->boostY;
                        }
                    }
                }
            }

            // Pop from the stack
            curr = (QuadNode*) node_stack_pointer[--stack_counter];
        }
    }
}

unsigned int select(Cell cells[], QuadNode* root, 
    void** node_stack_pointer, unsigned short* list_pointer, 
    float l, float r, float b, float t) {
    
    unsigned int list_counter = 0;
    unsigned int stack_counter = 0;

    // Push root to stack
    node_stack_pointer[stack_counter++] = root;

    while (stack_counter > 0) {
        // Pop from the stack
        QuadNode* curr = (QuadNode*) node_stack_pointer[--stack_counter];

        // Has leaves, push leaves, if they intersect, to stack
        if (curr->tl) {
            if (b < curr->y) {
                if (r > curr->x)
                    node_stack_pointer[stack_counter++] = curr->br;
                if (l < curr->x)
                    node_stack_pointer[stack_counter++] = curr->bl;
            }
            if (t > curr->y) {
                if (r > curr->x)
                    node_stack_pointer[stack_counter++] = curr->tr;
                if (l < curr->x)
                    node_stack_pointer[stack_counter++] = curr->tl;
            }
        }

        for (unsigned int i = 0; i < curr->count; i++) {
            unsigned short id = *(&curr->indices + i);
            Cell* cell = &cells[id];
            if (cell->x - cell->r <= r &&
                cell->x + cell->r >= l &&
                cell->y - cell->r <= t &&
                cell->y + cell->r >= b) {
                list_pointer[list_counter++] = id;
            }
        }
    }

    return list_counter;
}