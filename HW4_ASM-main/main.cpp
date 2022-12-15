#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <random>
#include <vector>
#include <queue>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>


using namespace std;

int n = 0;  // количество булавок
int checkedPins = 0;  // количество проверенных булавок 1-м мастером
bool isGoodNumber = true;  // для проверки числа на корректность (изначально считаем, что число корректное)

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

ofstream fout("output.txt");  // файловый поток вывода

struct Pin{ // структура "Булавка"
    int number = -1; // номер булавки
    bool isNotCurve = false;  // кривизна булавки
    int sharpQuality = -1;  // заточка булавки
    bool wasCheckedQuality = false;  // качество булавки

    Pin(int n) : number(n) {
        isNotCurve = false;  // мы еще не проверяли кривизну булавки
        sharpQuality = -1;  //  мы еще не точили булавку
        wasCheckedQuality = false;  // мы еще не проверяли качество булавки
    }
    Pin(){}
};


vector<Pin> v;  // массив булавок
queue<int> q1;  // очередь для заточки (индексы булавок из v)
queue<int> q2;  // очередь для проверки заточки (индексы булавок из v)


ostream& operator<<(ostream &os, const Pin& pin) {  // как выводить Pin
    string curve = " is curved!\n";
    string sharp = "";
    if (pin.isNotCurve) {
        curve = " is not curved,";
        sharp = " quality was not checked\n";
        if (pin.sharpQuality == -1) {
            sharp = " not sharped yet\n";
        }
        if (pin.wasCheckedQuality) {
            if (pin.sharpQuality == 0) {
                sharp = " has bad quality\n";
            }
            if (pin.sharpQuality == 1) {
                sharp = " has average quality\n";
            }
            if (pin.sharpQuality == 2) {
                sharp = " has good quality\n";
            }
        }
    }
    return os << "Pin number " << pin.number + 1 << curve << sharp;
}

void* checkCurve(void *args) {
    for (int i = 0; i < n; i++, checkedPins++) {  // перебираем все булавки
        v[i] = Pin(i);
        v[i].isNotCurve = rand() % 2 == 0;  // проверяем кривизну булавки
        cout << v[i];
        fout << v[i];

        if (v[i].isNotCurve) {
            pthread_mutex_lock(&mutex1);  // заморозили доступ к q1 другим потокам
            q1.push(i);
            pthread_mutex_unlock(&mutex1);
        }
        sleep(rand() % 2 + 1);  // мастер отдыхает
    }
}

void* sharpening(void *args) {
    while (checkedPins < n || !q1.empty()) {
        pthread_mutex_lock(&mutex1);
        if (!q1.empty()) {
            sleep(rand() % 3);  // мастер отдыхает
            int curr = q1.front();  // достаем булавку из очереди
            pthread_mutex_lock(&mutex2);
            q1.pop();  // удаляем булавку из очереди
            v[curr].sharpQuality = rand() % 3;  // точим булавку
            cout << v[curr];
            fout << v[curr];
            q2.push(curr);
            pthread_mutex_unlock(&mutex2);
            sleep(rand() % 2 + 1);  // мастер отдыхает
        }
        pthread_mutex_unlock(&mutex1);
    }
}

void* checkSharpQuality(void *args) {
    while (checkedPins < n || !q2.empty() || !q1.empty()) {
        pthread_mutex_lock(&mutex2);
        if (!q2.empty()) {
            sleep(rand() % 3 + 1);  // мастер отдыхает
            int curr = q2.front();  // достаем булавку из очереди
            q2.pop();  // удаляем булавку из очереди
            pthread_mutex_unlock(&mutex2);
            v[curr].wasCheckedQuality = true;  // проверили качество булавки
            cout << v[curr];
            fout << v[curr];
            sleep(rand() % 3 + 1);  // мастер отдыхает
        } else {
            pthread_mutex_unlock(&mutex2);
        }
    }
}

void fileInput() {  // функция для файлового ввода данных
    ifstream fin("input.txt");
    fin >> n;
    fin.close();
    if (n < 1 || n > 25) {
        isGoodNumber = false;
    }
}

int main (int argc, char *argv[]) {
    srand(time(0));  // чтобы каждый раз генерировались разные числа
    string command = "";
    if (argc == 1) {
        cout << "Enter input type: \nConsole;\nFile;\nRandom;\n";
        while (cin >> command) {
            if (command == "Console" || command == "console") {
                cout << "Enter number of pins (1<=n<=25): ";
                while (cin >> n) {
                    if (n > 0 && n < 26) {
                        break;
                    } else {
                        cout << "Mistake! Try again: ";
                    }
                }
                break;
            }
            if (command == "File" || command == "file") {
                fileInput();
                cout << "Number is " << n << '\n';
                if (!isGoodNumber) {
                    cout << "Unfortunately, number from the file is incorrect(1<=n<=25), change file and try again.";
                    fout.close();
                    return 0;
                }
                break;
            }

            if (command == "Random" || command == "random") {
                n = rand() % 25 + 1;
                cout << "Generated number is " << n << '\n';
                break;
            }
            cout << "Mistake! Try again:\n";
        }
    } else {
        if (!strcmp(argv[1], "file") || !strcmp(argv[1], "File")) {
            fileInput();
            cout << "Number is " << n << '\n';
            if (!isGoodNumber) {
                cout << "Unfortunately, number from the file is incorrect(1<=n<=25), change file and try again.";
                fout.close();
                return 0;
            }
        }
        if (!strcmp(argv[1], "random") || !strcmp(argv[1], "Random")) {
            n = rand() % 25 + 1;
            cout << "Generated number is " << n << '\n';
        }
        if (!strcmp(argv[1], "number") || !strcmp(argv[1], "Number")) {
            if (argc < 2) {
                cout << "Too little data; The number will be randomly generated.\n";
                n = rand() % 25 + 1;
                cout << "Generated number is " << n << '\n';
            } else {
                n = stoi(argv[2]);
                if (n < 0 || n > 25) {
                    cout << "Unfortunately, your number is incorrect; The number will be randomly generated.\n";
                    n = rand() % 25 + 1;
                    cout << "Generated number is " << n << '\n';
                } else {
                    cout << "Number is " << n << '\n';
                }
            }
        }
    }
    v.resize(n);
    pthread_t thread1;  // для проверки кривизны булавки
    pthread_t thread2;  // для заточки булавки
    pthread_t thread3;  // для проверки качества булавки


    pthread_mutex_init(&mutex1, NULL);  // создаем мутексы
    pthread_mutex_init(&mutex2, NULL);


    pthread_create(&thread1, NULL, checkCurve, NULL);  // создаем потоки
    pthread_create(&thread2, NULL, sharpening, NULL);
    pthread_create(&thread3, NULL, checkSharpQuality, NULL);


    pthread_join(thread1, NULL);  // создаем последовательность потоков
    pthread_join(thread2, NULL);  // не останавливать программу пока потоки работают
    pthread_join(thread3, NULL);

    pthread_mutex_destroy(&mutex1);  // уничтожаем мутексы
    pthread_mutex_destroy(&mutex2);

    fout.close();  // закрываем файловый вывод

    return 0;
}