#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <vector>
#include <optional>

// 1 meter in Box2D = 30 pixels in SFML
const float SCALE = 30.f; 

struct PhysicsObject {
    b2BodyId bodyId;
    sf::CircleShape shape;
};

int main() {
    // SFML 3 Window Setup
    sf::RenderWindow window(sf::VideoMode({800, 600}), "FusionBox Physics");
    window.setFramerateLimit(60);

    // Box2D 3.0 World Setup
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 9.8f};
    b2WorldId worldId = b2CreateWorld(&worldDef);

    // Create Ground (Static Body)
    b2BodyDef groundBodyDef = b2DefaultBodyDef();
    groundBodyDef.position = {400.0f / SCALE, 580.0f / SCALE};
    b2BodyId groundId = b2CreateBody(worldId, &groundBodyDef);
    
    b2Polygon groundBox = b2MakeBox(400.0f / SCALE, 10.0f / SCALE);
    b2ShapeDef groundShapeDef = b2DefaultShapeDef();
    b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);

    // SFML representation of ground
    sf::RectangleShape groundView(sf::Vector2f({800.f, 20.f}));
    groundView.setFillColor(sf::Color::Red);
    groundView.setOrigin({400.f, 10.f});
    groundView.setPosition({400.f, 580.f});

    std::vector<PhysicsObject> objects;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (event->is<sf::Event::MouseButtonPressed>()) {
                b2BodyDef bodyDef = b2DefaultBodyDef();
                bodyDef.type = b2_dynamicBody;
                
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                bodyDef.position = { (float)mousePos.x / SCALE, (float)mousePos.y / SCALE };
                
                b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
                
                // Create a circle shape
                b2Circle circle = { {0.0f, 0.0f}, 20.0f / SCALE };
                
                // Use default shape definition (this handles friction/restitution internally)
                b2ShapeDef shapeDef = b2DefaultShapeDef();
                
                b2CreateCircleShape(bodyId, &shapeDef, &circle);

                sf::CircleShape view(20.f);
                view.setFillColor(sf::Color::Cyan);
                view.setOrigin({20.f, 20.f});
                objects.push_back({bodyId, view});
            }
        }

        // Step Physics
        b2World_Step(worldId, 1.0f / 60.0f, 4);

        // Sync positions
        for (auto& obj : objects) {
            b2Vec2 pos = b2Body_GetPosition(obj.bodyId);
            b2Rot rot = b2Body_GetRotation(obj.bodyId);
            float angle = b2Rot_GetAngle(rot);

            obj.shape.setPosition({pos.x * SCALE, pos.y * SCALE});
            obj.shape.setRotation(sf::degrees(angle * 180.f / 3.14159f));
        }

        window.clear(sf::Color(30, 30, 30));
        window.draw(groundView);
        for (auto& obj : objects) {
            window.draw(obj.shape);
        }
        window.display();
    }

    b2DestroyWorld(worldId);
    return 0;
}