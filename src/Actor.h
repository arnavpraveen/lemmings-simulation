#ifndef ACTOR_H_
#define ACTOR_H_

#include "GraphObject.h"

class StudentWorld;

// base actor class

class Actor : public GraphObject {
public:
    Actor(int imageID, int startX, int startY, int dir, StudentWorld* world);
    virtual ~Actor() {}
    // every actor will have its own doSomething()
    virtual void doSomething() = 0;
    
    // getters and setters
    
    // access the world this actor belongs to
    StudentWorld* getWorld() const;
    
    virtual bool isSolid() const;
    virtual bool isClimbable() const { return false; }
    virtual bool isPheromone() const { return false; }
    virtual int getFallDistance() const { return 0; }
    virtual bool startBouncing(int targetDistance) { return false; }
    virtual bool isLemming() const { return false; }
    virtual void getSaved() {}
    // alive/dead state
    bool isAlive() const { return m_alive; }
    virtual void die() { m_alive = false; }
    
private:
    bool m_alive;
    StudentWorld* m_world;
};


// only purpose is to be rendered and be solid
class FloorBrick : public Actor {
public:
    FloorBrick(int startX, int startY, StudentWorld* world);
    virtual ~FloorBrick() {}
    virtual void doSomething();
    
    virtual bool isSolid() const {
        return true;
    }
};

// icemonster implementation
class IceMonster : public Actor {
public:
    IceMonster(int startX, int startY, StudentWorld* world);
    virtual ~IceMonster() {}
    
    virtual void doSomething();
    
private:
    // controls movement speed (moves every 10 ticks)
    int m_tickCounter;
};


// lemming implementation
class Lemming : public Actor {
public:
    // 4 possible states: walking, falling, climbing, bouncing
    enum State { WALKING, FALLING, CLIMBING, BOUNCING };
    Lemming(int startX, int startY, StudentWorld* world);
    virtual ~Lemming() {}
    virtual bool isLemming() const { return true; }
    virtual void doSomething();
    virtual void die();
    void getSaved();
    // bounce/fall interface used by trampolines and springs
    virtual bool startBouncing(int targetDistance);
    virtual int getFallDistance() const { return m_fallDistance; }
    
private:
    State m_state;
    // controls movement speed
    int m_tickCounter;
    // how far the lemming has fallen so far
    int m_fallDistance;
    // how many squares upward this bounce should go
    int m_bounceTarget;
    // how many upward squares completed so far
    int m_bounceCurrent;

    // helper functions for each movement state
    void processPheromones();
    void doWalking();
    void doFalling();
    void doClimbing();
    void doBouncing();
};

// stationary hazard
class Bonfire : public Actor {
public:
    Bonfire(int startX, int startY, StudentWorld* world);
    virtual ~Bonfire() {}
    virtual void doSomething();
    
private:
};

// builder cursor controlled by the player
class Player : public Actor {
public:
    Player(StudentWorld* world);
    virtual ~Player() {}
    
    virtual void doSomething();
};

// bounces falling lemmings upward based on fall distance
class Trampoline : public Actor {
public:
    Trampoline(int startX, int startY, StudentWorld* world);
    virtual ~Trampoline() {}
    
    virtual void doSomething();
};

// allows lemmings to climb vertically
class Net : public Actor {
public:
    Net(int startX, int startY, StudentWorld* world);
    virtual ~Net() {}
    
    virtual void doSomething();
    
    virtual bool isClimbable() const { return true; }
};

// forces an actor's direction
class OneWayDoor : public Actor {
public:
    OneWayDoor(int startX, int startY, int dir, StudentWorld* world);
    virtual ~OneWayDoor() {}
    
    virtual void doSomething();
};

// spawns lemmings every 100 ticks, up to 10 total
class LemmingFactory : public Actor {
public:
    LemmingFactory(int startX, int startY, StudentWorld* world);
    virtual ~LemmingFactory() {}
    
    virtual void doSomething();

private:
    int m_tickCount; // ticks since last spawn
    int m_spawnCount; // total lemmings spawned so far
};

// passively attracts lemmings within 5 horizontal squares
class Pheromone : public Actor {
public:
    Pheromone(int startX, int startY, StudentWorld* world);
    virtual ~Pheromone() {}
    
    virtual void doSomething();
    // lemmings check for this
    virtual bool isPheromone() const { return true; }
};

// launches lemmings upward with a fixed bounce height of 15
class Spring : public Actor {
public:
    Spring(int startX, int startY, StudentWorld* world);
    virtual ~Spring() {}
    
    virtual void doSomething();
};

// destination square where lemmings are saved
class Exit : public Actor {
public:
    Exit(int startX, int startY, StudentWorld* world);
    virtual ~Exit() {}
    
    virtual void doSomething();
};

#endif // ACTOR_H_

