#include "StudentWorld.h"
#include "Actor.h"
#include "GameConstants.h"
#include <string>
#include <iomanip>
#include <sstream>

using namespace std;

GameWorld* createStudentWorld(string assetPath)
{
    return new StudentWorld(assetPath);
}

// Students:  Add code to this file, StudentWorld.h, Actor.h, and Actor.cpp
// Do not change or remove the createStudentWorld implementation above.

StudentWorld::StudentWorld(string assetPath)
: GameWorld(assetPath), m_player(nullptr)
{
}

StudentWorld::~StudentWorld() {
    cleanUp();
}

int StudentWorld::init()
{
    // reset per-level state
    m_lemmingsSaved = 0;
    m_lemmingsDead = 0;
    m_timeLimit = 2000;
    ostringstream oss;
    // create the builder cursor at the center of the board
    m_player = new Player(this);
    // build the level filename from the current level number
    oss << "level" << setfill('0') << setw(2) << getLevel() << ".txt";
    string levelFile = oss.str();
    
    // load and validate the level file
    Level lev(assetPath());
    Level::LoadResult result = lev.loadLevel(levelFile);
    
    if (result == Level::load_fail_file_not_found)
        return GWSTATUS_PLAYER_WON;
    if (result == Level::load_fail_bad_format)
        return GWSTATUS_LEVEL_ERROR;
    
    // parse the tools line into inventory counts
    m_inventoryTrampolines = 0;
    m_inventoryNets        = 0;
    m_inventoryLeftDoors   = 0;
    m_inventoryRightDoors  = 0;
    m_inventoryPheromones  = 0;
    m_inventorySprings     = 0;
    
    string tools = lev.getTools();
    for (char c : tools) {
        switch (c) {
            case 'T': m_inventoryTrampolines++; break;
            case 'N': m_inventoryNets++; break;
            case '<': m_inventoryLeftDoors++; break;
            case '>': m_inventoryRightDoors++; break;
            case 'P': m_inventoryPheromones++; break;
            case 'S': m_inventorySprings++; break;
        }
    }
    // create actors for every occupied square in the level grid
    for (int x = 0; x < VIEW_WIDTH; ++x) {
        for (int y = 0; y < VIEW_HEIGHT; ++y) {
            Level::MazeEntry item = lev.getContentsOf(Coord(x, y));
            
            if (item == Level::floor) {
                m_actors.push_back(new FloorBrick(x, y, this));
            }
            else if (item == Level::ice_monster) {
                m_actors.push_back(new IceMonster(x, y, this));
            }
            else if (item == Level::bonfire) {
                m_actors.push_back(new Bonfire(x, y, this));
            }
            else if (item == Level::trampoline) {
                m_actors.push_back(new Trampoline(x, y, this));
            }
            else if (item == Level::net) {
                m_actors.push_back(new Net(x, y, this));
            }
            else if (item == Level::left_one_way_door) {
                m_actors.push_back(new OneWayDoor(x, y, GraphObject::left, this));
            }
            else if (item == Level::right_one_way_door) {
                m_actors.push_back(new OneWayDoor(x, y, GraphObject::right, this));
            }
            else if (item == Level::lemming_factory) {
                m_actors.push_back(new LemmingFactory(x, y, this));
            }
            else if (item == Level::pheromone) {
                m_actors.push_back(new Pheromone(x, y, this));
            }
            else if (item == Level::spring) {
                m_actors.push_back(new Spring(x, y, this));
            }
            else if (item == Level::lemming_exit) {
                m_actors.push_back(new Exit(x, y, this));
            }
        }
    }
    
    return GWSTATUS_CONTINUE_GAME;
}

int StudentWorld::move()
{
    // decrement the level timer
    m_timeLimit--;
    
    ostringstream oss;
        
    oss << setfill('0');
    
    // update the status bar at the top of the screen
    oss << "Score: " << setw(5) << getScore() << "  ";
    oss << "Level: " << setw(2) << getLevel() << "  ";
    oss << "Lives: " << setw(2) << getLives() << "  ";
    oss << "Saved: " << setw(2) << m_lemmingsSaved << "  ";
    
    // build the tools display string from inventory counts
    string tools = "";
    for (int i = 0; i < m_inventoryTrampolines; i++) tools += 'T';
    for (int i = 0; i < m_inventoryNets; i++)        tools += 'N';
    for (int i = 0; i < m_inventoryPheromones; i++)  tools += 'P';
    for (int i = 0; i < m_inventorySprings; i++)     tools += 'S';
    for (int i = 0; i < m_inventoryLeftDoors; i++)   tools += '<';
    for (int i = 0; i < m_inventoryRightDoors; i++)  tools += '>';

    oss << "Tools: " << (tools.empty() ? "None" : tools) << "  ";

    oss << "Time left: " << setw(4) << m_timeLimit;

    setGameStatText(oss.str());
    
    // player goes first
    if (m_player != nullptr && m_player->isAlive()) {
        m_player->doSomething();
    }
    
    // every other actor goes after the player
    for (Actor* actor : m_actors) {
        if (actor->isAlive()) {
            actor->doSomething();
        }
    }
    
    // merge any actors spawned during this tick
    for (Actor* a : m_newActors)
        m_actors.push_back(a);
    m_newActors.clear();

    // remove dead actors from the world
    vector<Actor*>::iterator it = m_actors.begin();
    while (it != m_actors.end()) {
        if (!(*it)->isAlive()) {
            delete *it;
            it = m_actors.erase(it);
        } else {
            ++it;
        }
    }
    
    // check loss condition
    if (m_lemmingsDead >= 6) {
        decLives();
        return GWSTATUS_PLAYER_DIED;
    }
    
    // check if all lemmings have been accounted for
    if (m_lemmingsSaved + m_lemmingsDead == 10) {
        if (m_lemmingsSaved >= 5) {
            increaseScore(m_timeLimit);
            playSound(SOUND_FINISHED_LEVEL);
            return GWSTATUS_FINISHED_LEVEL;
        } else {
            decLives();
            return GWSTATUS_PLAYER_DIED;
        }
    }
    
    // check if the timer ran out
    if (m_timeLimit <= 0) {
        if (m_lemmingsSaved >= 5) {
            playSound(SOUND_FINISHED_LEVEL);
            return GWSTATUS_FINISHED_LEVEL;
        } else {
            decLives();
            return GWSTATUS_PLAYER_DIED;
        }
    }

    return GWSTATUS_CONTINUE_GAME;
}

// delete everything
void StudentWorld::cleanUp()
{
    for(Actor* actor : m_actors) {
        delete actor;
    }
    for(Actor* actor : m_newActors) {
        delete actor;
    }
    if (m_player != nullptr) {
        delete m_player;
        m_player = nullptr;
    }
    m_actors.clear();
    m_newActors.clear();
}

// returns true if any solid actor occupies (x,y)
bool StudentWorld::isSolid(int x, int y) const
{
    for (Actor* actor : m_actors) {
        if (actor->getCoord().x == x && actor->getCoord().y == y && actor->isSolid()) {
            return true;
        }
    }
    return false;
}

bool StudentWorld::isValidPos(int x, int y) const {
    return (x >= 0 && x < VIEW_WIDTH && y >= 0 && y < VIEW_HEIGHT);
}

bool StudentWorld::hasClimbable(int x, int y) const {
    for (Actor* actor : m_actors) {
        if (actor->getCoord().x == x && actor->getCoord().y == y && actor->isClimbable()) {
            return true;
        }
    }
    return false;
}

// find the closest pheromone within 5 squares on the same row
bool StudentWorld::getClosestPheromoneDirection(int x, int y, int& outDir) const {
    int closestDist = 999;
    bool found = false;

    for (Actor* actor : m_actors) {
        if (actor->isPheromone() && actor->getCoord().y == y) {
            int dist = actor->getCoord().x - x;
            int absDist = std::abs(dist);

            if (absDist > 0 && absDist <= 5) {
                if (absDist < closestDist) {
                    closestDist = absDist;
                    found = true;
                    outDir = (dist < 0) ? GraphObject::left : GraphObject::right;
                }
            }
        }
    }
    return found;
}

void StudentWorld::increaseDeadLemmings() {
    m_lemmingsDead++;
}

void StudentWorld::increaseSavedLemmings() {
    m_lemmingsSaved++;
    increaseScore(SCORE_SAVED_LEMMING);
}

// kill all lemmings at (x,y), returns true if any were killed
bool StudentWorld::killLemmingsAt(int x, int y) {
    bool killedAny = false;
    for (Actor* actor : m_actors) {
        if (actor->getCoord().x == x && actor->getCoord().y == y && actor->isLemming()) {
            actor->die();
            killedAny = true;
        }
    }
    return killedAny;
}

// attempt to place a tool at (x,y)
void StudentWorld::tryPlaceTool(int x, int y, int key) {
    // check if any actor already occupies this square
    for (Actor* actor : m_actors) {
        if (actor->getCoord().x == x && actor->getCoord().y == y) {
            return;
        }
    }

    switch (key) {
        case 't': case 'T':
            if (m_inventoryTrampolines > 0) {
                m_actors.push_back(new Trampoline(x, y, this));
                m_inventoryTrampolines--;
            }
            break;
            
        case 'n': case 'N':
            if (m_inventoryNets > 0) {
                m_actors.push_back(new Net(x, y, this));
                m_inventoryNets--;
            }
            break;
        case '<':
            if (m_inventoryLeftDoors > 0) {
                m_actors.push_back(new OneWayDoor(x, y, GraphObject::left, this));
                m_inventoryLeftDoors--;
            }
            break;
            
        case '>':
            if (m_inventoryRightDoors > 0) {
                m_actors.push_back(new OneWayDoor(x, y, GraphObject::right, this));
                m_inventoryRightDoors--;
            }
            break;
        case 'p': case 'P':
            if (m_inventoryPheromones > 0) {
                m_actors.push_back(new Pheromone(x, y, this));
                m_inventoryPheromones--;
            }
            break;
        case 's': case 'S':
            if (m_inventorySprings > 0) {
                m_actors.push_back(new Spring(x, y, this));
                m_inventorySprings--;
            }
            break;
    }
}

// trampoline bounce
void StudentWorld::bounceLemmingAt(int x, int y) {
    for (Actor* actor : m_actors) {
        if (actor->getCoord().x == x && actor->getCoord().y == y && actor->isLemming()) {
            
            int height = actor->getFallDistance() - 1;
            if (height < 0) {
                height = 0;
            }
            
            if (actor->startBouncing(height)) {
                playSound(SOUND_BOUNCE);
            }
        }
    }
}

// force all actors at (x,y) to face the given direction
void StudentWorld::setActorsDirectionAt(int x, int y, int dir) {
    for (Actor* actor : m_actors) {
        if (actor->getCoord().x == x && actor->getCoord().y == y) {
            actor->setDirection(dir);
        }
    }
}

void StudentWorld::spawnLemmingAt(int x, int y) {
    m_newActors.push_back(new Lemming(x, y, this));
}

// spring bounce
void StudentWorld::springLemmingAt(int x, int y) {
    for (Actor* actor : m_actors) {
        if (actor->getCoord().x == x && actor->getCoord().y == y && actor->isLemming()) {
            if (actor->startBouncing(15)) {
                playSound(SOUND_BOUNCE);
            }
        }
    }
}

// save one lemming at the exit per tick
void StudentWorld::saveOneLemmingAt(int x, int y) {
    for (Actor* actor : m_actors) {
        if (actor->isAlive() && actor->isLemming() && actor->getCoord().x == x && actor->getCoord().y == y) {
            actor->getSaved();
            return;
        }
    }
}
