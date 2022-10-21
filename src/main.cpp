/*
╔════════════════════════════════════════════════════════════════╗
║ Автор: ...                                                     ║
║ Версія: ...                                                    ║
╠════════════════════════════════════════════════════════════════╣
║ В майбутеьому варто переглянути фунції, які використовують     ║
║ глобальні об'єкти, такі як:                                    ║
║   ─ RectangleShape *player,                                    ║
║   ─ Ball *ball,                                                ║
║   ─ vector<Plate *> plates;                                    ║
║ Бо неамє сенсу передавати їх в аргументи, якшо вони глобальні. ║
╚════════════════════════════════════════════════════════════════╝
*/

#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <vector>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

using namespace sf;
using namespace std;

/* Розміри ігрового вікна*/
#define WIN_W 640
#define WIN_H 480

/* Початкові значення */
#define PLAYER_SPEED 8
#define BALL_SPEED 3.5
#define BALL_DAMAGE 5
#define PLATE_HEALTH 5

/* Змінні стану гри, тому вони розташованні найвище */
static bool isWin, isRunning;
static int score, streak, kills;

/**
 * Кулька, для зручності, складена в стуркуру
 * з компонентів, щоб зручніше було використовувати в коді.
*/
struct Ball
{
    CircleShape *shape;
    Vector2f *moveDir; // Одиночний вектор напрямку руху кульки
    float speed;
    int damage;
};

/**
 * Аналогічно плиточка
*/
struct Plate
{
    RectangleShape *shape;
    int health;
    bool isActive;
};

/* Глобальні ігрові об'єкти */
static RectangleShape *player;
static Ball *ball;
static vector<Plate *> plates;

/**
 * Ініціалізація гравця (платформи)
*/
RectangleShape *initPlayer()
{
    RectangleShape *player = new RectangleShape;
    player->setSize(Vector2f(115, 15));
    player->setOrigin(player->getGlobalBounds().width / 2, player->getGlobalBounds().height / 2);
    player->setPosition(Vector2f(WIN_W / 2, WIN_H / 1.15));
    player->setFillColor(Color::Green);
    return player;
}


/**
 * Ініціалізація кульки та її налашутвання.
*/
Ball *initBall()
{
    CircleShape *shape = new CircleShape;
    shape->setRadius(10);
    shape->setOrigin(shape->getRadius() / 2, shape->getRadius() / 2);
    shape->setPosition(WIN_W / 2, WIN_H / 2);
    shape->setFillColor(Color::White);

    Ball *ball = new Ball;
    ball->shape = shape;
    ball->moveDir = new Vector2f(1, -1);
    ball->speed = BALL_SPEED;
    ball->damage = BALL_DAMAGE;

    return ball;
}

/**
 * Ініціалізація всіх плиток та 
 * задання їхнього розташування та кольорів.
*/
vector<Plate *> initPlates()
{
    vector<Plate *> vec;

    int n = 10; // Кількість по горизонталі
    int m = 4;  // Кількість по вертикалі

    int plateW = 50; // Ширина плитки
    int plateH = 10; // Висота

    float offsetX = 5; // Відстань між плитками по горизонталі
    float offsetY = 5; // Відстань між плитками по вертикалі

    // Початкова точка. Ліва верхня точка по іксу
    int startX = WIN_W / 2 - ((plateW + offsetX) * (n - 1)) / 2;
    int startY = 40;

    // Ініціалізуємо плитки
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < m; j++)
        {
            auto platesShape = new RectangleShape(Vector2f(plateW, plateH));
            platesShape->setOrigin(platesShape->getSize().x / 2, platesShape->getSize().y / 2);

            // Позиціонуємо плиточки послідовно, матрицею (сіточкою, табличкою etc)
            platesShape->setPosition(
                i * (platesShape->getSize().x + offsetX) + startX,
                j * (platesShape->getSize().y + offsetY) + startY
            );

            // Колір заливки залежить від ітерації
            platesShape->setFillColor(
                Color(
                    255 * sin((2 * 3.14) / (j + 1)),
                    255 * sin((2 * 3.14) / (i + 5)),
                    255 * cos((2 * 3.14) / (j + 1)),
                    255U));

            auto plate = new Plate;
            plate->shape = platesShape;
            plate->isActive = true;
            plate->health = PLATE_HEALTH;

            // Додаємо в кінець черги
            vec.push_back(plate);
        }
    }

    return vec;
};

/**
 * Функція керування гравцем.
 * Обмеження на кордони ігрової кімнати враховуються.
*/
void controllPlayer(RectangleShape *player)
{
    float half = player->getGlobalBounds().width / 2;

    // Так-як функція викликається кожен кадр, то
    // ми "питаємо" чи не натиснув раптом користувач
    // потрібну нам кнопку (Права чи Ліва стрілочки). 
    if (Keyboard::isKeyPressed(Keyboard::Left) && player->getPosition().x > half)
    {
        player->move(-PLAYER_SPEED, 0);
    } 
    else if (Keyboard::isKeyPressed(Keyboard::Right) && player->getPosition().x < (WIN_W - half))
    {
        player->move(PLAYER_SPEED, 0);
    }
}

/**
 * Автоматичний рух кульки з початково заданим напрямком
*/
void moveBall(Ball *ball)
{
    ball->shape->move(
        ball->moveDir->x * ball->speed,
        ball->moveDir->y * ball->speed);
}

/**
 * Перевірка на перетин чотирикутників (getGlobalBounds), які представляють межі данних фігур
*/
bool isBallCollided(Ball *ball, RectangleShape *other)
{
    return ball->shape->getGlobalBounds().intersects(other->getGlobalBounds());
}

/**
 * Функція відстрибування. Змінює координати одиничного вектора напрямку
*/
void bounceOff(Ball *ball, Shape *other)
{
    ball->moveDir->y *= -1;

    /*    
    auto ballPos = ball->shape->getPosition();
    auto r = ball->shape->getRadius();
    auto otherPos = other->getPosition();
    auto otherBounds = other->getGlobalBounds();

    if ((ballPos.x < otherPos.x - otherBounds.width / 2) || (ballPos.x > otherPos.x + otherBounds.width / 2))
        ball->moveDir->x *= -1;
    if ((ballPos.y < otherPos.y - otherBounds.height / 2) || (ballPos.y > otherPos.y + otherBounds.height / 2))
        ball->moveDir->y *= -1;
    */
}

/**
 * Зменшує "здоров'я" плитки та візуальний вигляд 
 * відповідно до змінної 'health'.
*/
bool damagePlate(Plate *plate)
{
    Color color = plate->shape->getFillColor();
    color.a = 255 - 255 / (plate->health + 1);
    plate->shape->setFillColor(color);

    plate->health -= ball->damage;

    if (plate->health <= 0)
    {
        plate->isActive = false;
        return true;
    }

    return false;
}

/**
 * Перевірка чи кулька за межеми кімнати
*/
bool isBallOutOfRoom(Ball *ball)
{
    auto pos = ball->shape->getPosition();
    auto r = ball->shape->getRadius();

    return (pos.x - r <= 0 || pos.x + r >= WIN_W) || (pos.y - r <= 0 || pos.y + r >= WIN_H);
}

/**
 * Алгоритм відбиття від стін ігрової кімнати.
 * Якщо кулька вилетіла за нижню межу - змінна
 * 'isRunning' стає 'false'.
*/
void bounceOffRoom(Ball *ball)
{
    auto ballPos = ball->shape->getPosition();
    auto r = ball->shape->getRadius();

    if (ballPos.x + r >= WIN_W)
        ball->moveDir->x = -1;
    else if (ballPos.x - r <= 0)
        ball->moveDir->x = 1;

    if (ballPos.y - r >= WIN_H)
        isRunning = false;
    else if (ballPos.y - r <= 0)
        ball->moveDir->y = 1;
}

/**
 * Допоміжна функція центрування тексту
*/
void centerText(Text &text)
{
    text.setOrigin(
        text.getGlobalBounds().width / 2,
        text.getGlobalBounds().height / 2);
}

/** 
 * Головна функція підготовки гри до старту
 */
void initGame()
{
    // Міняємо статус гравця. Автоматично він - програв
    isWin = false;
    isRunning = !isRunning;

    // Обнуляємо
    score = 0;
    streak = 0;
    kills = 0;

    // Видаляємо, якщо були, з купи об'єкти
    delete player;
    delete ball;
    for (auto plate : plates)
        delete plate;

    // Знову ініціалізовуємо
    player = initPlayer();
    ball = initBall();
    plates = initPlates();
}

// Обнулення тексту. Роблю через функцію, а не
// на пряму задля естетичного задоволення та
// однорідності коду.
void setTextZero(Text &text) {
    text.setString("0");
}

int main()
{
    /* Ініціалізація та завантаження шрифта */
    Font globalFont;
    globalFont.loadFromFile("assets/font.ttf");

    /* Ініціалізація гри */
    initGame();
    isRunning = false;
    bool isDead = false;
    
    // Ініціалізація графічного вікна програми
    RenderWindow window(VideoMode(WIN_W, WIN_H), "Arkanoid");
    window.setFramerateLimit(120);

    /** Ініціалізація графічного тексту **/

    /* Текст лічильника балів */
    Text scoreText;
    scoreText.setFont(globalFont);
    scoreText.setFillColor(Color(255, 255, 255, 20));
    scoreText.setCharacterSize(400);
    scoreText.setString("0");
    scoreText.setPosition(WIN_W / 2, WIN_H / 6);
    centerText(scoreText);

    /* Меню старту */
    Text startCaption;
    startCaption.setFont(globalFont);
    startCaption.setCharacterSize(50);
    startCaption.setString("MENU");
    startCaption.setPosition(WIN_W / 2, WIN_H / 2.5);
    centerText(startCaption);

    /* Текст підказки */
    Text keyHint;
    keyHint.setFont(globalFont);
    keyHint.setCharacterSize(20);
    keyHint.setString("Press  SPACE  to start");
    keyHint.setPosition(WIN_W / 2, WIN_H / 2);
    centerText(keyHint);

    /* Текст "серія вбивств" */
    Text streakText;
    streakText.setFont(globalFont);
    Color streakColor = Color::Yellow;
    streakColor.a = 140;
    streakText.setFillColor(streakColor);
    streakText.setCharacterSize(50);
    streakText.setString("0");
    streakText.setPosition(WIN_W / 2, WIN_H / 2);
    centerText(streakText);

    /* Текст статуса: виграш/програш */
    Text statusText;
    statusText.setFont(globalFont);
    statusText.setFillColor(Color::Magenta);
    statusText.setCharacterSize(70);
    statusText.setString("");
    statusText.setPosition(WIN_W / 2, WIN_H / 4);
    centerText(statusText);


    // Головний цикл програми
    while (window.isOpen())
    {
        // Цикл подій
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();

            if (event.type == Event::KeyPressed) 
            {
                // Коли натиснута клавіша "Space" і змінна isRunning == true
                if (!isRunning && event.key.code == Keyboard::Space) 
                {
                    // Переініціалізовуємо гру
                    initGame(); 

                    // Текст балів та "серії вбивств", які знаходсять на
                    // задньому фоні змінююємо на "0", щоб не бачити бали
                    // попередньої гри.
                    setTextZero(scoreText);
                    setTextZero(streakText);
                }
            }
        }

        // Фарбуємо задній фон в чорний колір
        window.clear(Color::Black);

        // Поки наша гра активна...
        if (isRunning)
        {
            // Малюємо на екрані:
            window.draw(*player);
            window.draw(*ball->shape);
            window.draw(scoreText);

            // Поки гра активна, кожен кадр рухаєм кульку та керуємо гравцем
            controllPlayer(player); 
            moveBall(ball);

            // Якщо "серія вбивсті" більше двух, то відображаємо 
            if (streak > 1)
                window.draw(streakText);

            // Для кожної нашої плиточки виконуєм певні дії...
            for (auto plate : plates)
            {
                // Якщо плитка не активна, то все пропускаєм
                // і переходимо до наступної ітерації
                if (!plate->isActive)
                    continue;

                // Малюємо конкретну плиточку
                window.draw(*plate->shape);

                // У випадку, якщо кулька зуштовхнулась з плиточкою, то
                // виконуємо наступний код (1)
                // Примітка: така конструкція використана, щоб зменшити вкладеність if-ів
                if (!isBallCollided(ball, plate->shape))
                    continue;

                // (1)
                // Відскокуєм
                bounceOff(ball, plate->shape);

                // Якщо плита не знищена, пропускаєм рядки після цього if-a, і
                // починаємо з наступної ітерації якщо - знищена, тоді цей if 
                // пропускажться і переходмо до (2)
                isDead = damagePlate(plate);
                if (!isDead)
                    continue;
                
                /** Далі йде код, який виконується у випадку, коли функція damagePlate **/
                /** повертає true, тобто, коли знищується плита. **/

                // (2)

                // Так-як це код, який виконується за умови, що плиточка знищена, то
                // збільшуємо дані змінні.                
                score++;
                streak++;
                kills++;

                scoreText.setString("" + std::to_string(score));
                streakText.setString(std::to_string(streak));

                // Якщо текст збільшився, наприклад став ширший, бо
                // з'явився новий роздряд: з 9 став 10, з 99 став 100; 
                // то центруємо його, щоб виглядало естетично.
                centerText(scoreText);
                centerText(streakText);

                // Перевіряємо чи знищені всі плити, якщо так - святкуємо перемогу 🍾🥂
                isWin = kills >= plates.size();
                isRunning = !isWin;
                
            }
            
            // Перевірка на зіштовхування кульки, з гравцем, тобто
            // платформую. В позитивному випадку - відштовхування та
            // обнуляємо "серію вбивств".
            if (isBallCollided(ball, player)) 
            {
                bounceOff(ball, player);
                streak = 0;
            }

            // Перевірка, чи кулька поза межами кімнати, якщо - так, то
            // відбиваємось від "стін" кімнати.
            if (isBallOutOfRoom(ball))
                bounceOffRoom(ball);
            

            /* Наступні рядки потрібно оптимізувати. */
            /* Як мінімум знатий їм краще місце в коді, */
            /* десь в кінці життєвого циклу гравця */
            statusText.setString(std::string(isWin ? "you win " : "loser ").append(std::to_string(score)));
            centerText(statusText);
        }
        else
        {
            /* Якщо гра не активна, малюємо меню */
            window.draw(startCaption);
            window.draw(keyHint);
            window.draw(statusText);
        }
        
        // Відображаємо даний кадр, ітерацію, а саме все, 
        // що передали в функціях 'window.draw' на екрані графічного вікна.
        window.display();
    }

    return EXIT_SUCCESS;
}