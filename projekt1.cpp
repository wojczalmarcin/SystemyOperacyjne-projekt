#include <iostream>
#include <thread>
#include <chrono>
#include <ncurses.h>
#include <vector>
#include <ctime>

using namespace std;

// zmienna określająca długość poziomego odcinka
static int horizontalRoadLenght = 80;
// zmienna okreslajaca długość pionowego odcinka
static int verticalRoadLenght = 20;
// procent prędkości na poziomym odcinku
static float horizontalSpeedBonus = 1.4;
// procent prędkości na pionowym odcinku
static float verticalSpeedBonus = 0.8;
// liczba okrążeń
static int maxLaps = 3;
// zmienna określająca czy program ma działać
bool isWorking = true;

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

    public:
    // konstruktor
    Car(int x, int y, int Id)
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
            this_thread::sleep_for(chrono::milliseconds((int)(3000/(speed*horizontalSpeedBonus))));
            if(posY == 0)
                posX-=1;
            else
                posX+=1;   
        }
        else if(posX<=0)
        {
             // wątek jest usypiany na czas zależny od prędkości samochodu
            this_thread::sleep_for(chrono::milliseconds((int)(3000/(speed*verticalSpeedBonus))));
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
            this_thread::sleep_for(chrono::milliseconds((int)(3000/(speed*verticalSpeedBonus))));
            if(posY<=0)
            {
                posY = 0;
                posX = horizontalRoadLenght-1;
                return;
            }
            posY-=1;
        }
    }

    // funkcja poruszająca samochód do momentu aż przejdzie 3 okrążenia
    // argumentem funkcji jest const bool w celu zapewnienia, że zmienna
    // nie będzie modyfikowana i nie nastąpi "data race"
    void driveLaps(){
        while(lap<maxLaps && isWorking == true)
        {
            drive();
            if(posY==startPosY && posX==startPosX)
            {
                 lap++;
                 speed = rand()%60 + 40;
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

    //jeśli użytkownik nie zakończył programu i wszystkie auta nie przejechały 3 okrążań to program trwa
    while(isWorking)
    {  
        // czyszczenie okna
        erase();
        mvprintw(0,0,"Nacisnij dowolny przycisk, aby zakonczyc program lub poczekaj az wszystkie auta przejada 3 okrazenia");
        // petle wyswietlajace tor
        for(int i = 1; i <= horizontalRoadLenght+5;i++)
        {
            mvprintw(1,i,"_");
            mvprintw(verticalRoadLenght+4,i,"_");
            if(i>7 && i <horizontalRoadLenght-1)
            {
                mvprintw(4,i,"_");
                mvprintw(verticalRoadLenght+1,i,"_");
            }
        }
        for(int i = 2; i <= verticalRoadLenght + 4;i++)
        {
            mvprintw(i,0,"|");
            mvprintw(i,horizontalRoadLenght+6,"|");
            
            if(i>4 && i <= verticalRoadLenght+1)
            {
                mvprintw(i,7,"|");
                mvprintw(i,horizontalRoadLenght-1,"|");
            }
        }

        // pętla wyświetlająca samochody
        for(int i = 0; i<cars.size();i++)
        {
            if(cars[i]->getIsDriving())
            {
                string s = std::to_string(cars[i]->getId());
                char const *pchar = s.c_str();
                mvprintw(cars[i]->getPosY()+3,cars[i]->getPosX()+3,pchar);
            }
            else
            {
                cars.erase(cars.begin()+i);
            }
        }
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

int main()
{
    srand(time(0));
    vector<Car*> carsVector;
    vector<thread> carThreads;
    carsVector.emplace_back(new Car(10,0,0));
    carThreads.emplace_back(&Car::driveLaps,carsVector.back());
    thread ncursesThread(draw,ref(carsVector));

    int number = 0;
    while(isWorking)
    {
        this_thread::sleep_for(chrono::milliseconds(rand()%10000));
        number++;
        if(number>9)
            number = 0;
        carsVector.emplace_back(new Car(10,0,number));
        carThreads.emplace_back(&Car::driveLaps,carsVector.back());
    }

    for (thread& thread : carThreads)
        thread.join();
    ncursesThread.join();

    cout<<"Program zakonczony"<<endl;
    return 0;
}