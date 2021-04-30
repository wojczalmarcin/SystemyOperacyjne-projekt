// Treść zadania:
// Samochód nie może wjechać na prawą krawędź, gdy jego prędkość jest większa od 
// prędkośći samochodu znajdującego się na tej krawędzi.
// W takim wypadku zatrzymuje się i czeka aż na krawędzi nie będzie wolniejszego auta.

#include <iostream>
#include <thread>
#include <chrono>
#include <ncurses.h>
#include <vector>
#include <ctime>
#include <mutex>

using namespace std;

#define ROAD_PAIR           1
#define EDGE_PAIR           2
#define CAR_PAIR            3
#define GRASS_PAIR          4
#define SPEED_MULTIPLIER    0.8

// zmienna określająca długość poziomego odcinka
static int horizontalRoadLenght = 40;
// zmienna okreslajaca długość pionowego odcinka
static int verticalRoadLenght = 30;
// procent prędkości na poziomym odcinku
static float horizontalSpeedBonus = 1.4;
// procent prędkości na pionowym odcinku
static float verticalSpeedBonus = 0.8;
// liczba okrążeń
static int maxLaps = 3;
// zmienna określająca czy program ma działać
bool isWorking = true;
// mutex
mutex edgeMutex;

///
/// Klasa opisująca samochód
///
class Car
{
    private:
    // prędkość samochodu
    int speed;
    // pozycja na osi x
    int posX;
    // pozycja na osi y
    int posY;
    // numer okrążenia
    int lap;
    // pozycja startowa na osi x
    int startPosX;
    // pozycja startowa na osi y
    int startPosY;
    // zmienna określająca czy samochod jedzie
    bool isDriving;
    // znak oznaczający pojazd
    int id;
    // wektor samochodów znajdujących się na prawej krawędzi
    vector<Car*> &carsOnRightEdgeVector;
    // zmienna określająca, czy samochód znajduje się na prawej krawędzi
    bool isOnRightEdge = false;
    // indeks wolniejszego samochodu na który trzeba poczekać
    int slowerCarIndex;

    public:
    // konstruktor
    Car(int x, int y, int Id, vector<Car*> &cars) 
        : carsOnRightEdgeVector(cars)
    {
        posX = x;
        posY = y;
        startPosX = x;
        startPosY = y;
        speed = rand()%60 + 40;
        lap = 0;
        id = Id;
        isDriving = true;
    }

    // funkcja zwracająca prędkość samochodu
    int getSpeed(){return speed;}
    // funkcja ustawiająca prędkość samochodu
    void setSpeed(int val){speed = val;}

    // funkcja zwracajaca pozycję na osi X samochodu
    int getPosX(){return posX;}
    // funkcja zwracajaca pozycję na osi Y samochodu
    int getPosY(){return posY;}

    // funkcja zwracajaca informację, czy samochód jedzie
    bool getIsDriving(){return isDriving;}

    // funkcja zwracająca znak pojazdu
    int getId(){return id;}

    // funkcja poruszajaca samochód o jedną jednostkę
    void drive()
    {
        // model jazdy
        if(posX>0 && posX<horizontalRoadLenght)
        {
            // wątek jest usypiany na czas zależny od prędkości samochodu
            this_thread::sleep_for(chrono::milliseconds((int)(3000/(speed*horizontalSpeedBonus*SPEED_MULTIPLIER))));
            if(posY == 0)
                posX-=1;
            else
                posX+=1;   
        }
        else if(posX<=0)
        {
             // wątek jest usypiany na czas zależny od prędkości samochodu
            this_thread::sleep_for(chrono::milliseconds((int)(3000/(speed*verticalSpeedBonus*SPEED_MULTIPLIER))));
            if(posY>=verticalRoadLenght)
            {
                posY = verticalRoadLenght;
                posX = 1;
                return;
            }
            posY +=1;  
        }
        else
        {
            // wątek jest usypiany na czas zależny od prędkości samochodu
            this_thread::sleep_for(chrono::milliseconds((int)(3000/(speed*verticalSpeedBonus*SPEED_MULTIPLIER))));
            if(posY<=0)
            {
                posY = 0;
                posX = horizontalRoadLenght-1;
                return;
            }
            posY-=1;
        }
    }

    // funkcja porusza samochód do momentu aż przejdzie 3 okrążenia
    // dodaje również samochód do wektora prawej krawędzi jeśli
    // faktycznie się na niej znajdzie
    void driveLaps(){

        bool mustWait = false;

        while(lap<maxLaps && isWorking == true)
        {
            // Dodawanie samochodu do wektora prawej krawędzi
            edgeMutex.lock();
            if(posX==horizontalRoadLenght)
            {
                if(!isOnRightEdge)
                {
                    carsOnRightEdgeVector.push_back(this);
                    isOnRightEdge = true;
                }   
            }
            else if(isOnRightEdge)
            {
                for (int i = 0;i<carsOnRightEdgeVector.size();i++)
                {
                    if(carsOnRightEdgeVector[i]->getId() == id)
                        carsOnRightEdgeVector.erase(carsOnRightEdgeVector.begin() + i);
                }
                isOnRightEdge = false;
            }
            edgeMutex.unlock();

            // porównanie prędkości samochodów znajdujących się na prawej krawędzi
            mustWait = false;
            if(!isOnRightEdge && posX>=(horizontalRoadLenght-1) && posY>=(verticalRoadLenght-1))
            {
                if(carsOnRightEdgeVector.size()!=0)
                {
                    for(int i = 0; i < carsOnRightEdgeVector.size();i++)
                    {
                        if(carsOnRightEdgeVector[i]->getSpeed() < speed)
                        {
                            mustWait = true;
                            break;
                            slowerCarIndex = i;
                        }
                            
                    }
                }
            }
            if(posY==startPosY && posX==startPosX)
            {
                 lap++;
                 speed = rand()%60 + 40;
            }

            if(!mustWait)
                drive();
            else
            {
                // usypianie wątku na czas aż wolniejsze auto dojedzie do końca prawej krawędzi
                this_thread::sleep_for(chrono::milliseconds((int)(3000/(carsOnRightEdgeVector[slowerCarIndex]->getSpeed()*horizontalSpeedBonus*SPEED_MULTIPLIER))
                *carsOnRightEdgeVector[slowerCarIndex]->getPosY()));
            }
        };
        isDriving = false;
        delete this;
    }
};

// funkcja rysująca samochody
void draw(vector<Car*> &cars)
{
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    scrollok(stdscr, TRUE);
    curs_set(0);
    start_color();
    init_pair(ROAD_PAIR, COLOR_WHITE, COLOR_WHITE);
    init_pair(EDGE_PAIR, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(CAR_PAIR, COLOR_WHITE, COLOR_RED);
    init_pair(GRASS_PAIR, COLOR_GREEN, COLOR_GREEN);
    //jeśli użytkownik nie zakończył programu i wszystkie auta nie przejechały 3 okrążań to program trwa
    while(isWorking)
    {  
        // czyszczenie okna
        erase();
        mvprintw(0,0,"Nacisnij dowolny przycisk, aby zakonczyc program");
        // petle wyswietlajace tor
        for(int i = 0; i <= horizontalRoadLenght*2+9;i++)
        {
            for(int j = 1;j<=verticalRoadLenght + 5;j++)
            {
                attron(COLOR_PAIR(ROAD_PAIR));
                if((j<2 || j>verticalRoadLenght+4) || (i<2 || i > horizontalRoadLenght*2 + 7))
                    attron(COLOR_PAIR(EDGE_PAIR));
                if((i>9 && i < horizontalRoadLenght*2+4) && (j>4 && j < verticalRoadLenght+2))
                    attron(COLOR_PAIR(GRASS_PAIR));
                    
                mvprintw(j,i,"-");
            }
        }
       
        // pętla wyświetlająca samochody
        attron(COLOR_PAIR(CAR_PAIR));
        for(int i = 0; i<cars.size();i++)
        {
            if(cars[i]->getIsDriving())
            {
                string s = std::to_string(cars[i]->getId());
                char const *pchar = s.c_str();
                mvprintw(cars[i]->getPosY()+3,cars[i]->getPosX()*2+5,pchar);
            }
            else
            {
                cars.erase(cars.begin()+i);
            }
        }
        attroff(COLOR_PAIR(CAR_PAIR));
        refresh();
        int ch = getch();

        //sprawdzenie, czy użytkownik nacisnął dowolny przycisk, aby wyjść z programu
        if (ch != ERR) 
        {
            ungetch(ch);
            isWorking = false;
        }
    }
    endwin(); 
    cout<<"Zamykanie wszystkich watkow..."<<endl;
}

// Main
int main()
{
    srand(time(0));
    vector<Car*> carsVector;
    vector<Car*> carsOnRightEdgeVector;
    vector<thread> carThreads;
    carsVector.emplace_back(new Car(10,0,0,ref(carsOnRightEdgeVector)));
    carThreads.emplace_back(&Car::driveLaps,carsVector.back());
    thread ncursesThread(draw,ref(carsVector));

    int number = 0;
    while(isWorking)
    {
        this_thread::sleep_for(chrono::milliseconds(rand()%5000));
        number++;
        if(number>98)
            number = 0;
        carsVector.emplace_back(new Car(10,0,number,ref(carsOnRightEdgeVector)));
        carThreads.emplace_back(&Car::driveLaps,carsVector.back());
    }

    for (thread& thread : carThreads)
        thread.join();
    ncursesThread.join();

    cout<<"Program zakonczony"<<endl;
    return 0;
}