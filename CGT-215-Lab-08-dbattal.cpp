#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFPhysics.h>
#include <vector>
#include <SFML/Audio.hpp>

using namespace std;
using namespace sf;
using namespace sfp;

// time between duck spawns
const int DUCK_INTERVAL_MS = 1500;  
// duck speed in ms
const float DUCK_SPEED = 0.25;      

//print the error message if file is missing 
void LoadTex(Texture& tex, string filename) {
    if (!tex.loadFromFile(filename)) {
        cout << "Could not load " << filename << endl;
    }
}

Vector2f GetTextSize(Text text) {
    FloatRect r = text.getGlobalBounds();
    return Vector2f(r.width, r.height);
}

int main()
{
    //creating a window and phsyics simulation, zero gravity present 
    //score starts at 0, arrows strat at 5
    RenderWindow window(VideoMode(800, 600), "Duck Hunter");
    World world(Vector2f(0, 0));
    int score(0);
    int arrows(5);

    //loading the baloon pop sound effect 
    SoundBuffer popBuffer;
    if (!popBuffer.loadFromFile("balloonpop.ogg")) {
        cout << "could not load balloonpop.ogg" << endl;
        exit(5);
    }
    Sound popSound;
    popSound.setBuffer(popBuffer);

    //loaduing the background music and setting it up to start playing right away
    Music music;
    if (!music.openFromFile("circus.ogg")) {
        cout << "Failed to load circus.ogg ";
        exit(6);
    }
    music.play();

    //creating the crossbow sprite and texture, loading the image
    // crossbow doesn't move, positioned at the (horizontal) middle 
    PhysicsSprite crossBow;
    Texture cbowTex;
    LoadTex(cbowTex, "images/crossbow.png");
    crossBow.setTexture(cbowTex);
    Vector2f sz = crossBow.getSize();
    crossBow.setCenter(Vector2f(400, 600 - (sz.y / 2)));

    // one arrow object gets reused every shot
    PhysicsSprite arrow;
    Texture arrowTex;
    LoadTex(arrowTex, "images/arrow.png");
    arrow.setTexture(arrowTex);
    bool drawingArrow(false);

    // setting up the top and right wall
    PhysicsRectangle top;
    top.setSize(Vector2f(800, 10));
    top.setCenter(Vector2f(400, 5));
    top.setStatic(true);
    world.AddPhysicsBody(top);

    PhysicsRectangle right;
    right.setSize(Vector2f(10, 600));
    right.setCenter(Vector2f(850, 300));
    right.setStatic(true);
    world.AddPhysicsBody(right);

    // this collision marks missed arrows 
    top.onCollision = [&drawingArrow, &world, &arrow]
    (PhysicsBodyCollisionResult result) {
        drawingArrow = false;
        world.RemovePhysicsBody(arrow);
        };

    // setting up the ducks! 
    // i just renamed the duck image red_balloon lol
    // the PhysicsShapeList is the collection of sprites that handles creating and removing them
    // + duckTimer, starts at 0 (ms) so duck spawns immediately, counts from DUCK_INTERVAL_MS, spawns another 
    Texture duckTex;
    LoadTex(duckTex, "images/red_balloon.png");   
    PhysicsShapeList<PhysicsSprite> ducks;
    int duckTimer(0);  

    //font and clock yay 
    Font fnt;
    if (!fnt.loadFromFile("arial.ttf")) {
        cout << "Could not load font." << endl;
        exit(3);
    }
    Clock clock;
    Time lastTime(clock.getElapsedTime());
    Time currentTime(lastTime);

    //game runs while you have an arrow OR an arrow still flying
    while ((arrows > 0) || drawingArrow) {
        currentTime = clock.getElapsedTime();
        Time deltaTime = currentTime - lastTime;
        long deltaMS = deltaTime.asMilliseconds();
        if (deltaMS > 9) {
            lastTime = currentTime;
            world.UpdatePhysics(deltaMS);

            //if timer goes down to 0, reset to 1500, spawn duck!
            duckTimer -= deltaMS;
            if (duckTimer <= 0) {
                duckTimer = DUCK_INTERVAL_MS;
                PhysicsSprite& duck = ducks.Create();
                duck.setTexture(duckTex);
                Vector2f dsz = duck.getSize();
                duck.setCenter(Vector2f(-dsz.x / 2, 20 + (dsz.y / 2)));
                duck.setVelocity(Vector2f(DUCK_SPEED, 0));
                world.AddPhysicsBody(duck);

                //each duck gets its own handler
                // if the other object in the collision is an arrow, remove both objects, get points, go again
                // if its the right wall, duck removed with no score :( wah wah 
                duck.onCollision =
                    [&drawingArrow, &world, &arrow, &duck, &ducks, &score, &popSound, &right]
                    (PhysicsBodyCollisionResult result) {
                    if (result.object2 == arrow) {
                        popSound.play();
                        drawingArrow = false;
                        world.RemovePhysicsBody(arrow);
                        world.RemovePhysicsBody(duck);
                        ducks.QueueRemove(duck);
                        score += 10;
                    }
                    if (result.object2 == right) {
                        world.RemovePhysicsBody(duck);
                        ducks.QueueRemove(duck);
                    }
                    };
            }

            // shoot if space is pressed, theres no arrow in flight, AND have arrows remainingn
            // (0, -1) means it goes straight up the y axis 
            if (Keyboard::isKeyPressed(Keyboard::Space) && !drawingArrow && arrows > 0) {
                drawingArrow = true;
                arrows = arrows - 1;
                arrow.setCenter(crossBow.getCenter());
                arrow.setVelocity(Vector2f(0, -1));
                world.AddPhysicsBody(arrow);
            }

            //wipe previous frame, draw arrow if its active
            // remove duck (queued for removal (struck duck)) before drawing duck! 
            window.clear();
            if (drawingArrow) {
                window.draw(arrow);
            }
            ducks.DoRemovals();
            for (PhysicsShape& duck : ducks) {
                window.draw((PhysicsSprite&)duck);
            }
            window.draw(crossBow);

            // arrow count bottom left of window
            Text arrowCountText;
            arrowCountText.setFont(fnt);
            arrowCountText.setString(to_string(arrows));
            arrowCountText.setPosition(Vector2f(10, 600 - GetTextSize(arrowCountText).y - 15));
            window.draw(arrowCountText);

            // score bottom right of window 
            Text scoreText;
            scoreText.setFont(fnt);
            scoreText.setString(to_string(score));
            scoreText.setPosition(Vector2f(790 - GetTextSize(scoreText).x,
                600 - GetTextSize(scoreText).y - 15));
            window.draw(scoreText);

            window.display();
        }
    }

    // keeps last game frame on screen, text is centered
    window.display(); 
    Text gameOverText;
    gameOverText.setString("GAME OVER");
    gameOverText.setFont(fnt);
    sz = GetTextSize(gameOverText);
    gameOverText.setPosition(400 - (sz.x / 2), 300 - (sz.y / 2));
    window.draw(gameOverText);
    window.display();

    while (Keyboard::isKeyPressed(Keyboard::Space));
    while (!Keyboard::isKeyPressed(Keyboard::Space));
}