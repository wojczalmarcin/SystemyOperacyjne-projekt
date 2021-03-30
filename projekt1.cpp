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
    const char* id;

    public:
    // konstruktor
    Car(int x, int y, const char* newId)
    {
        posX = x;
        posY = y;
        startPosX = x;
        startPosY = y;
        speed = rand()%60 + 40;
        lap = 0;
        id = newId;
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
    const char* getId(){return id;}

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
    void driveLaps(int random){
        isDriving = false;
        if(random>0)
            this_thread::sleep_for(chrono::milliseconds(random));
        isDriving = true;
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
void draw(vector<Car*> cars)
{
    //wektor zawierający informację, które samochody jadą
    vector<bool>drivingCars;
    for(int i = 0; i<cars.size();i++)
    {
        drivingCars.push_back(cars[i]->getIsDriving());
    }
    //zmienna kontrolująca, czy wszystkie auta przejechały, jeśli tak to będzie ona wynosić 0
    short allCarsNotDriving = 1;

    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    scrollok(stdscr, TRUE);

    //jeśli użytkownik nie zakończył programu i wszystkie auta nie przejechały 3 okrążań to program trwa
    while(isWorking && allCarsNotDriving!=0)
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
                mvprintw(cars[i]->getPosY()+3,cars[i]->getPosX()+3,cars[i]->getId());
                drivingCars[i] = true;
            }
            else
            {
                drivingCars[i] = false;
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

        //zsumowanie wartości informacji o jadących samochodach, jeśli suma wynosi 0 to znaczy że wszystkie auta przejechały trasę
        allCarsNotDriving = 0;
        for(int i = 0; i<drivingCars.size();i++)
        {
            allCarsNotDriving+=drivingCars[i];
        }
    }
    endwin(); 
    cout<<"Zamykanie wszystkich watkow..."<<endl;
}

int main()
{
    srand(time(0));
    vector<Car*> carsVector;
    Car * car1 = new Car(10,0,"A");
    Car * car2 = new Car(10,0,"B");
    Car * car3 = new Car(10,0,"C");
    Car * car4 = new Car(10,0,"D");
    carsVector.push_back(car1);
    carsVector.push_back(car2);
    carsVector.push_back(car3);
    carsVector.push_back(car4);

    thread t1(&Car::driveLaps,car1,0);
    thread t2(&Car::driveLaps,car2,rand()%100);
    thread t3(&Car::driveLaps,car3,rand()%100);
    thread t4(&Car::driveLaps,car4,rand()%100);
    thread ncursesThread(draw,carsVector);   
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    ncursesThread.join();

    cout<<"Program zakonczony"<<endl;
    return 0;
}