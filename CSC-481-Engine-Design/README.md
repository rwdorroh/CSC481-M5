# CSC-481 Team 20 Game Engine Documentation

## Milestone 1

Task 1: Core graphics setup is handled by the Engine.h/.cpp files

Task 2: Generic entity system is handled by the Entity.h/.cpp files and used by the Engine files

Task 3: Physics system is handled by the Physics.h/.cpp files and used by the Entity files

Task 4: Input handling system is handled by the Input.h/.cpp files

Task 5: Collision detections is handled by the Collision.h/.cpp files and used by the Entity files

Types.h is a header for the OrderedPair and Velocity structs

## Milestone 2

Task 1: Measuring and representing time is handled by the Timeline.h/.cpp files and used by our game's main.cpp files

Task 2: The client server system is handled by Client.h/.cpp, Server.cpp, NetworkTypes.h and used by our game's main.cpp files

Task 3 & 4: Multithreaded loop architecture and asynchronicity is handled by Engine.cpp (worker threads for updating player entities), 
            Server.cpp (client threads and shared moving object thread), and used by our game's main.cpp files

## Milestone 3

Task 1: We created a component-based game object model in our engine. We did this by refactoring our Entity system / code to GameObject.h,
        Component.h, and several other component classes for things like rendering, gravity, movement, etc. This is extensible because we
        can add new components to our engine as we need, and then add them to the game objects that need them in our games.

Task 2: Our networking was already mostly functional from the last milestone, all we had to do was update our Server controlled objects to use
        the new game object model, handle an arbitrary number of clients using a listener, and manage disconnects. All this code can be found in
        Client.h/.cpp, Server.cpp, NetworkTypes.h, and NetworkComponent.h

Both of these tasks are also used / handled in our game's code.

## Milestone 4

Task 1: The Event Management System lives entirely in the engine. The code can be found in Event.h and EventManager.h/.cppp
		We create events as strings with a map of parameters as well as an age and priority for queuing purposes.
		The event manager can be used to setup event handlers, raise and dispatch events, as well as manage queuing of events.

Task 2: To handle networked event management we added the ability for clients to send an array of events to the server. This code is located in 
		NetworkTypes.h and Client.cpp, we also moved the server to our respective games instead of being in the engine.

## Milestone 5

Task 1: The Memory Management system lives entirely in the engine. The code can be found in the GameObjectAllocator.h/.cpp and GameObjectPool.h/.cpp files.
        Our code allocates memory based on the generalized GameObject, not a specific type of GameObject. 

Task 2: The Input Chord system lives in entirely in the engine. The code can be found in the Input.h/.cpp files.
        Our code stores inputs pressed, processes them from the largest defined chord to the smallest, and if it was a 
        valid chord with all keys pressed within the window then it's converted to a bit mask to match our current input system.

Both tasks were completed by repurposing the boilerplate, and are used in our game's code.
