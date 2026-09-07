#ifndef STUDENTWORLD_H_
#define STUDENTWORLD_H_

#include "GameWorld.h"
#include "Level.h"
#include <string>
#include <vector>

// Students:  Add code to this file, StudentWorld.cpp, Actor.h, and Actor.cpp

class Actor;
class Player;

// manages the game worl
class StudentWorld : public GameWorld
{
public:
    StudentWorld(std::string assetPath);
    virtual ~StudentWorld();
    
    virtual int init();
    virtual int move();
    virtual void cleanUp();
    
    // world queries used by actors
    bool isSolid(int x, int y) const;
    bool isValidPos(int x, int y) const;
    bool hasClimbable(int x, int y) const;
    bool getClosestPheromoneDirection(int x, int y, int& outDir) const;
    // lemming lifecycle tracking
    void increaseDeadLemmings();
    void increaseSavedLemmings();
    bool killLemmingsAt(int x, int y);
    void tryPlaceTool(int x, int y, int key);
    void bounceLemmingAt(int x, int y);
    void setActorsDirectionAt(int x, int y, int dir);
    void spawnLemmingAt(int x, int y);
    void springLemmingAt(int x, int y);
    void saveOneLemmingAt(int x, int y);

private:
    std::vector<Actor*> m_actors; // all actors except the player cursor
    std::vector<Actor*> m_newActors;
    Player* m_player;
    
    // per-level counters
    int m_lemmingsSaved;
    int m_lemmingsDead;
    int m_totalLemmingsNeeded;
    int m_inventoryTrampolines;
    int m_inventoryNets;
    int m_inventoryPheromones;
    int m_inventorySprings;
    int m_inventoryLeftDoors;
    int m_inventoryRightDoors;
    int m_timeLimit;
};

#endif // STUDENTWORLD_H_
