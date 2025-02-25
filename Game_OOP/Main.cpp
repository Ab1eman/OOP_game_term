#include <iostream>
#include <vector>
#include <fstream>
#include <conio.h>
#include <cstdlib>
#include <string>
using namespace std;

// Класс для координат
class Position {
public:
    int x, y;
    Position(int x_ = 0, int y_ = 0) : x(x_), y(y_) {}
    bool operator==(const Position& other) const { return x == other.x && y == other.y; }
    int distanceTo(const Position& other) const { return abs(x - other.x) + abs(y - other.y); }
};

// Абстрактный класс для объектов на карте
class GameObject {
protected:
    Position pos;
    char lastMove;
public:
    GameObject(int x, int y) : pos(x, y), lastMove(' ') {}
    virtual void update() = 0;
    virtual char getSymbol() const = 0;
    Position getPosition() const { return pos; }
    void setPosition(Position newPos, char direction) {
        pos = newPos;
        lastMove = direction;
    }
    char getLastMove() const { return lastMove; }
};

// Класс карты
class Map {
private:
    vector<vector<char>> grid;
    vector<vector<int>> noiseTimer;
    int width, height;
public:
    Map(const string& filename, int w, int h) : width(w), height(h) {
        grid.resize(height, vector<char>(width, '.'));
        noiseTimer.resize(height, vector<int>(width, 0));
        ifstream file(filename);
        if (file.is_open()) {
            for (int y = 0; y < height && !file.eof(); y++) {
                string line;
                getline(file, line);
                for (int x = 0; x < min(width, (int)line.size()); x++) {
                    grid[y][x] = line[x];
                }
            }
            file.close();
        }
        else {
            for (int i = 0; i < width; i++) {
                grid[0][i] = '#';
                grid[height - 1][i] = '#';
            }
            for (int i = 0; i < height; i++) {
                grid[i][0] = '#';
                grid[i][width - 1] = '#';
            }
        }
    }
    bool isWalkable(Position pos) const {
        return pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height && grid[pos.y][pos.x] != '#';
    }
    bool isExit(Position pos) const {
        return pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height && grid[pos.y][pos.x] == 'E';
    }
    void setSymbol(Position pos, char symbol) {
        grid[pos.y][pos.x] = symbol;
        if (symbol == '!') noiseTimer[pos.y][pos.x] = 2;
    }
    char getSymbol(Position pos) const { return grid[pos.y][pos.x]; }
    void updateNoise() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (grid[y][x] == '!' && noiseTimer[y][x] > 0) {
                    noiseTimer[y][x]--;
                    if (noiseTimer[y][x] == 0) grid[y][x] = '.';
                }
            }
        }
    }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
};

// Класс игрока
class Player : public GameObject {
private:
    int health;
public:
    Player(int x, int y) : GameObject(x, y), health(3) {}
    void update() override {}
    void move(int dx, int dy, Map& map) {
        Position newPos(pos.x + dx, pos.y + dy);
        if (map.isWalkable(newPos)) {
            char direction = ' ';
            if (dx > 0) direction = '>';
            else if (dx < 0) direction = '<';
            else if (dy > 0) direction = 'v';
            else if (dy < 0) direction = '^';
            setPosition(newPos, direction);
            makeNoise(map);
        }
    }
    void makeNoise(Map& map) {
        for (int dir = 0; dir < 4; dir++) {
            int dx = (dir == 0 ? 1 : dir == 1 ? -1 : 0);
            int dy = (dir == 2 ? 1 : dir == 3 ? -1 : 0);
            for (int i = 1; i <= 2; i++) {
                Position noisePos(pos.x + dx * i, pos.y + dy * i);
                if (!map.isWalkable(noisePos)) break;
                if (map.getSymbol(noisePos) != 'E') {
                    map.setSymbol(noisePos, '!');
                }
            }
        }
    }
    char getSymbol() const override { return '@'; }
    int getHealth() const { return health; }
    void takeDamage() { if (health > 0) health--; }
};

// Базовый класс врага
class Enemy : public GameObject {
protected:
    char symbol;
    bool chasing;
public:
    Enemy(int x, int y, char sym) : GameObject(x, y), symbol(sym), chasing(false) {}
    char getSymbol() const override { return symbol; }
    virtual void hearNoise(const Map& map, const Position& playerPos) {
        for (int dy = -3; dy <= 3; dy++) {
            for (int dx = -3; dx <= 3; dx++) {
                Position checkPos(pos.x + dx, pos.y + dy);
                if (checkPos.x >= 0 && checkPos.x < map.getWidth() && checkPos.y >= 0 && checkPos.y < map.getHeight()) {
                    if (map.getSymbol(checkPos) == '!') {
                        chasing = true;
                        return;
                    }
                }
            }
        }
    }
};

// Враг, патрулирующий по маршруту
class PatrolEnemy : public Enemy {
private:
    vector<Position> patrolRoute;
    int routeIndex;
    const Player* target;
    const Map* map;
public:
    PatrolEnemy(int x, int y, const Player* player, const Map* m, int routeType = 0)
        : Enemy(x, y, 'P'), routeIndex(0), target(player), map(m) {
        if (routeType == 0) {
            patrolRoute = { Position(x, y), Position(x + 1, y), Position(x + 2, y), Position(x + 1, y + 1), Position(x, y + 1) };
        }
        else if (routeType == 1) {
            patrolRoute = { Position(x, y), Position(x, y + 1), Position(x + 1, y + 1), Position(x + 1, y), Position(x, y) };
        }
        else {
            patrolRoute = { Position(x, y), Position(x + 1, y), Position(x + 2, y), Position(x + 3, y), Position(x + 3, y + 1) };
        }
    }
    void update() override {
        if (chasing) {
            Position playerPos = target->getPosition();
            int dx = playerPos.x - pos.x;
            int dy = playerPos.y - pos.y;
            Position newPos = pos;
            char direction = ' ';
            if (abs(dx) > abs(dy)) {
                newPos.x += (dx > 0 ? 1 : -1);
                direction = (dx > 0 ? '>' : '<');
            }
            else {
                newPos.y += (dy > 0 ? 1 : -1);
                direction = (dy > 0 ? 'v' : '^');
            }
            if (map->isWalkable(newPos)) setPosition(newPos, direction);
        }
        else {
            routeIndex = (routeIndex + 1) % patrolRoute.size();
            Position newPos = patrolRoute[routeIndex];
            int dx = newPos.x - pos.x;
            int dy = newPos.y - pos.y;
            char direction = ' ';
            if (dx > 0) direction = '>';
            else if (dx < 0) direction = '<';
            else if (dy > 0) direction = 'v';
            else if (dy < 0) direction = '^';
            if (map->isWalkable(newPos)) setPosition(newPos, direction);
        }
    }
};

// Враг, преследующий игрока
class ChaseEnemy : public Enemy {
private:
    const Player* target;
    const Map* map;
public:
    ChaseEnemy(int x, int y, const Player* player, const Map* m) : Enemy(x, y, 'C'), target(player), map(m) {}
    void update() override {
        if (!chasing) return;
        Position playerPos = target->getPosition();
        int dx = playerPos.x - pos.x;
        int dy = playerPos.y - pos.y;
        Position newPos = pos;
        char direction = ' ';
        if (abs(dx) > abs(dy)) {
            newPos.x += (dx > 0 ? 1 : -1);
            direction = (dx > 0 ? '>' : '<');
        }
        else {
            newPos.y += (dy > 0 ? 1 : -1);
            direction = (dy > 0 ? 'v' : '^');
        }
        if (map->isWalkable(newPos)) setPosition(newPos, direction);
    }
};

// Класс обработки ввода
class InputHandler {
public:
    void handleInput(Player& player, Map& map) {
        char input = _getch();
        switch (input) {
        case 'w': player.move(0, -1, map); break;
        case 's': player.move(0, 1, map); break;
        case 'a': player.move(-1, 0, map); break;
        case 'd': player.move(1, 0, map); break;
        }
    }
};

// Класс отрисовки
class Renderer {
public:
    void render(const Map& map, const Player& player, const vector<Enemy*>& enemies, int levelNum) {
        system("cls");
        for (int y = 0; y < map.getHeight(); y++) {
            for (int x = 0; x < map.getWidth(); x++) {
                Position pos(x, y);
                bool objectRendered = false;
                if (pos == player.getPosition()) {
                    cout << player.getLastMove() << player.getSymbol();
                    objectRendered = true;
                }
                else {
                    for (const auto* enemy : enemies) {
                        if (pos == enemy->getPosition()) {
                            cout << enemy->getLastMove() << enemy->getSymbol();
                            objectRendered = true;
                            break;
                        }
                    }
                }
                if (!objectRendered) cout << " " << map.getSymbol(pos);
            }
            cout << endl;
        }
        cout << "Level: " << levelNum + 1 << " | Health: " << player.getHealth() << endl;
        cout << "Legend: @ - Player, P - Patrol Enemy, C - Chase Enemy, # - Wall, E - Exit, ! - Noise, Arrows - Last Move" << endl;
    }
    void renderVictory() {
        system("cls");
        cout << endl << endl;
        cout << "       Congratulations! You Won!" << endl << endl;
        cout << "             _______" << endl;
        cout << "            /       \\" << endl;
        cout << "           /_________\\" << endl;
        cout << "           |  TROPHY  |" << endl;
        cout << "           |_________|" << endl << endl;
        cout << "Press any key to exit..." << endl;
        _getch();
    }
};

// Класс уровня
class Level {
private:
    Map map;
    Player* player;
    vector<Enemy*> enemies;
public:
    Level(const string& mapFile, int startX, int startY, int width, int height) : map(mapFile, width, height) {
        player = new Player(startX, startY);
        if (width == 15) { // Level 1
            enemies.push_back(new PatrolEnemy(5, 5, player, &map, 0));
            enemies.push_back(new ChaseEnemy(10, 5, player, &map));
        }
        else if (width == 25) { // Level 2
            enemies.push_back(new PatrolEnemy(10, 5, player, &map, 0));
            enemies.push_back(new PatrolEnemy(15, 10, player, &map, 1));
            enemies.push_back(new ChaseEnemy(20, 5, player, &map));
        }
        else { // Level 3
            enemies.push_back(new PatrolEnemy(10, 10, player, &map, 0));
            enemies.push_back(new PatrolEnemy(15, 15, player, &map, 1));
            enemies.push_back(new PatrolEnemy(5, 20, player, &map, 2));
            enemies.push_back(new ChaseEnemy(20, 15, player, &map));
            enemies.push_back(new ChaseEnemy(25, 10, player, &map));
        }
    }
    void update() {
        map.updateNoise();
        for (auto* enemy : enemies) {
            enemy->hearNoise(map, player->getPosition());
            enemy->update();
            if (enemy->getPosition() == player->getPosition()) {
                player->takeDamage();
            }
        }
    }
    Player* getPlayer() { return player; }
    const vector<Enemy*>& getEnemies() const { return enemies; }
    Map& getMap() { return map; }
    ~Level() {
        delete player;
        for (auto* enemy : enemies) delete enemy;
    }
};

// Главный класс игры
class Game {
private:
    vector<Level*> levels;
    int currentLevelIndex;
    InputHandler inputHandler;
    Renderer renderer;
    bool running;
public:
    Game() : currentLevelIndex(0), running(true) {
        levels.push_back(new Level("level1.txt", 1, 1, 15, 10));  // Маленький
        levels.push_back(new Level("level2.txt", 1, 1, 25, 15));  // Средний
        levels.push_back(new Level("level3.txt", 1, 1, 40, 25));  // Большой
    }
    void run() {
        while (running) {
            Level* currentLevel = levels[currentLevelIndex];
            renderer.render(currentLevel->getMap(), *currentLevel->getPlayer(), currentLevel->getEnemies(), currentLevelIndex);

            if (currentLevel->getPlayer()->getHealth() <= 0) {
                cout << "Game Over!" << endl;
                running = false;
                break;
            }

            inputHandler.handleInput(*currentLevel->getPlayer(), currentLevel->getMap());
            currentLevel->update();

            if (currentLevel->getMap().isExit(currentLevel->getPlayer()->getPosition())) {
                if (currentLevelIndex < levels.size() - 1) {
                    currentLevelIndex++;
                    cout << "Moving to next level..." << endl;
                    _getch();
                }
                else {
                    renderer.renderVictory();
                    running = false;
                }
            }
        }
    }
    ~Game() {
        for (auto* level : levels) delete level;
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}