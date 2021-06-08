// Treść zadania:
// Samochód nie może wjechać na prawą krawędź, gdy jego prędkość jest większa od 
// prędkośći samochodu znajdującego się na tej krawędzi.
// W takim wypadku zatrzymuje się i czeka aż na krawędzi nie będzie wolniejszego auta.
//
// Semafor, który będzie się zapalał na czerwony, gdy auta stoją, potem na zielony i
// dopiero wtedy auto porusza się do przodu, stojące auta w kolejce są ustawione jeden za drugim,
// jak wjedzie pierwszy to się przesuwają, semafor zapala się na czerwone

#include <iostream>
#include <thread>
#include <chrono>
#include <ncurses.h>
#include <vector>
#include <ctime>
#include <mutex>
#include <queue>
#include <semaphore.h>

using namespace std;

#define ROAD_PAIR           1
#define EDGE_PAIR           2
#define CAR_PAIR            3
#define GRASS_PAIR          4
#define RED_LIGHT_PAIR      5
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
// kolejka czekających samochodów
//queue<int> waitingCars;
// Semafor
sem_t mySemaphore;

bool RedLight = false;

// liczba czekających samochodów
int numberOfWaitingCars = 0;
// mutex do modyfikacji liczby czekających samochodów
mutex waitingCarsMutex;

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
    vector<Car*> &carsVector;
    // wektor samochodów znajdujących się na prawej krawędzi
    vector<Car*> &carsOnRightEdgeVector;
    // zmienna określająca, czy samochód znajduje się na prawej krawędzi
    bool isOnRightEdge = false;
    // indeks wolniejszego samochodu na który trzeba poczekać
    int slowerCarIndex;
    // zmienna określająca czy samochód musi czekać
    bool mustWait = false;
    // zmienna określająca czy samochód musi się zatrzymać na czerwonym
    //bool redLight = false;
    std::mutex mtx;
    public:
    // konstruktor
    Car(int x, int y, int Id, vector<Car*> &cars,vector<Car*> &cars2) 
        : carsOnRightEdgeVector(cars), carsVector(cars2)
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
    int getSpeed(){lock_guard<mutex> lk(mtx); return speed;}
    // funkcja ustawiająca prędkość samochodu
    void setSpeed(int val){speed = val;}

    // funkcja zwracajaca pozycję na osi X samochodu
    int getPosX(){lock_guard<mutex> lk(mtx); return posX;}
    // funkcja zwracajaca pozycję na osi Y samochodu
    int getPosY(){lock_guard<mutex> lk(mtx); return posY;}

    // funkcja zwracajaca informację, czy samochód jedzie
    bool getIsDriving(){lock_guard<mutex> lk(mtx); return isDriving;}

    // funkcja zwracająca znak pojazdu
    int getId(){return id;}
    // funkcja zwracająca czy samochód jest na prawej krawędzi

    bool getIsOnRightEdge(){lock_guard<mutex> lk(mtx); return isOnRightEdge;}
    // funkcja zwracająca status czekania
    bool getMustWait(){lock_guard<mutex> lk(mtx); return mustWait;}
    // funkcja ustawiająca status czekania
    void setMustWait(bool wait){mustWait = wait;}
    // funkcja zwracająca status czerwonego światła
    //bool getRedLight(){return redLight;}
    // funkcja ustawiająca status czerwonego światła
    //void setRedLight(bool red){redLight = red;}
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

            // sprawdzenie czy samochód znajdujący się przed stoi, jeśli tak to przerwij jazdę
            if(carsVector.size()>1)
            {
                for(int i = 0; i < carsVector.size();i++)
                {
                    if(posX == carsVector[i]->posX - 1 && carsVector[i]->mustWait && posY == carsVector[i]->posY && posY>=(verticalRoadLenght-1))
                    {
                        mustWait = true;
                    }
                }
            }
           
            if(RedLight && posX >= (horizontalRoadLenght-1) && posY >=(verticalRoadLenght-1))
                mustWait = true;
            //if(redLight)
            //    mustWait = true;
            /*
            // porównanie prędkości samochodów znajdujących się na prawej krawędzi
            if(!isOnRightEdge && posX>=(horizontalRoadLenght-1) && posY>=(verticalRoadLenght-1))
            {
                if(carsOnRightEdgeVector.size()!=0)
                {
                    for(int i = 0; i < carsOnRightEdgeVector.size();i++)
                    {
                        if(carsOnRightEdgeVector[i]->getSpeed() < speed)
                        {
                            mustWait = true;
                            slowerCarIndex = i;
                            break;
                            
                        }
                            
                    }
                }
            }*/
            if(posY==startPosY && posX==startPosX)
            {
                 lap++;
                 speed = rand()%60 + 40;
            }

            if(!mustWait)
                drive();
            else
            {
                waitingCarsMutex.lock();
                numberOfWaitingCars++;
                waitingCarsMutex.unlock();

                sem_wait(&mySemaphore);
                mustWait = false;
                waitingCarsMutex.lock();
                numberOfWaitingCars--;
                waitingCarsMutex.unlock();
                sem_post(&mySemaphore);
                // usypianie wątku na czas aż wolniejsze auto dojedzie do końca prawej krawędzi
                //this_thread::sleep_for(chrono::milliseconds((int)(3000/(carsOnRightEdgeVector[slowerCarIndex]->getSpeed()*horizontalSpeedBonus*SPEED_MULTIPLIER))
                //*carsOnRightEdgeVector[slowerCarIndex]->getPosY()));
                //break;
            }
        };
        
        if(lap>=maxLaps)
        {
            isDriving = false;
            delete this;
        }
    }
};

// funckja włączająca czerwone światło zmuszające auta do czekania
void redLight(vector<Car*> &cars,vector<Car*> &carsOnRightEdgeVector)
{
    int slowerCarIndex = 0;
    // NAPISAĆ TE PĘTLĘ INACZEJ!!!
    while(isWorking)
    {
        if(cars.size()>1)
        {
            if(numberOfWaitingCars==0)
            {
                for(int j = 0;j< cars.size();j++)
                {
                    if(!cars[j]->getIsOnRightEdge() && cars[j]->getPosX()>=(horizontalRoadLenght-1) && cars[j]->getPosY()>=(verticalRoadLenght-1))
                    {
                        if(carsOnRightEdgeVector.size()!=0)
                        {
                            for(int i = 0; i < carsOnRightEdgeVector.size();i++)
                            {
                                if(carsOnRightEdgeVector[i]->getSpeed() < cars[j]->getSpeed())
                                {
                                    slowerCarIndex = i;
                                    RedLight = true;
                                    break;
                                }                        
                            }
                        }
                    }
                }
            }
            else if(carsOnRightEdgeVector.size()>0)
            {
                slowerCarIndex = 0;
                RedLight = true;
            }
        }
        if(RedLight)
        {
            sem_wait(&mySemaphore);
            this_thread::sleep_for(chrono::milliseconds((int)(3000/(carsOnRightEdgeVector[slowerCarIndex]->getSpeed()*horizontalSpeedBonus*SPEED_MULTIPLIER) + 10)
                *carsOnRightEdgeVector[slowerCarIndex]->getPosY()));
            RedLight= false;
            sem_post(&mySemaphore);
        }
    }
}
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
    init_pair(RED_LIGHT_PAIR, COLOR_RED, COLOR_RED);
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
        if(RedLight)
            attron(COLOR_PAIR(RED_LIGHT_PAIR));
        else
            attron(COLOR_PAIR(GRASS_PAIR));
        mvprintw(verticalRoadLenght+6,horizontalRoadLenght*2 + 5,"OO");
        attron(COLOR_PAIR(CAR_PAIR));
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
    sem_init(&mySemaphore, 0, 1);
    srand(time(0));
    vector<Car*> carsVector;
    vector<Car*> carsOnRightEdgeVector;
    vector<thread> carThreads;
    carsVector.emplace_back(new Car(10,0,0,ref(carsOnRightEdgeVector),ref(carsVector)));
    carThreads.emplace_back(&Car::driveLaps,carsVector.back());
    thread ncursesThread(draw,ref(carsVector));
    thread redLightThread(redLight,ref(carsVector),ref(carsOnRightEdgeVector));

    int number = 0;
    while(isWorking)
    {
        this_thread::sleep_for(chrono::milliseconds(rand()%5000));
        number++;
        if(number>98)
            number = 0;
        carsVector.emplace_back(new Car(10,0,number,ref(carsOnRightEdgeVector),ref(carsVector)));
        carThreads.emplace_back(&Car::driveLaps,carsVector.back());
    }

    for (thread& thread : carThreads)
        thread.join();
    ncursesThread.join();
    redLightThread.join();
    sem_destroy(&mySemaphore);
    cout<<"Program zakonczony"<<endl;
    return 0;
}