#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define ALTURA 20
#define LARGURA 50
#define NAVE_CHAR 'A'
#define TIRO_CHAR '^'
#define OBSTACULO_CHAR '#'
#define INIMIGO_CHAR 'W'
#define COMBUSTIVEL_CHAR 'F'
#define VAZIO_CHAR ' '

#define MAX_TIROS 5
#define MAX_OBJETOS 20
#define DELAY 100

typedef struct {
    int x, y;
    int ativo;
} Tiro;

typedef struct {
    int x, y;
    int tipo; // 0 = obstáculo, 1 = inimigo, 2 = combustível
    int ativo;
} Objeto;

typedef struct {
    int x, y;
} Nave;

Nave nave;
Tiro tiros[MAX_TIROS];
Objeto objetos[MAX_OBJETOS];

int score = 0;
int game_over = 0;
int combustivel = 100;

HANDLE hConsole;
CHAR_INFO consoleBuffer[LARGURA * ALTURA];
COORD bufferSize = {LARGURA, ALTURA};
COORD characterPos = {0, 0};
SMALL_RECT consoleWriteArea = {0, 0, LARGURA - 1, ALTURA - 1};

void reset() {
    nave.x = LARGURA / 2;
    nave.y = ALTURA - 2;
    score = 0;
    combustivel = 100;
    game_over = 0;

    for (int i = 0; i < MAX_TIROS; i++) {
        tiros[i].ativo = 0;
    }

    for (int i = 0; i < MAX_OBJETOS; i++) {
        objetos[i].ativo = 0;
    }
}

void spawn_objetos() {
    for (int i = 0; i < MAX_OBJETOS; i++) {
        if (!objetos[i].ativo && rand() % 20 == 0) {
            objetos[i].ativo = 1;
            objetos[i].x = rand() % (LARGURA - 2) + 1;
            objetos[i].y = 0;
            int tipo_aleatorio = rand() % 100;
            if (tipo_aleatorio < 60)
                objetos[i].tipo = 0; // obstáculo
            else if (tipo_aleatorio < 90)
                objetos[i].tipo = 1; // inimigo
            else
                objetos[i].tipo = 2; // combustível
        }
    }
}

void atualiza_tiros() {
    for (int i = 0; i < MAX_TIROS; i++) {
        if (tiros[i].ativo) {
            tiros[i].y--;
            if (tiros[i].y < 0) {
                tiros[i].ativo = 0;
            }
        }
    }
}

void atualiza_objetos() {
    for (int i = 0; i < MAX_OBJETOS; i++) {
        if (objetos[i].ativo) {
            objetos[i].y++;
            if (objetos[i].y >= ALTURA) {
                objetos[i].ativo = 0;
            }
        }
    }
}

void verificar_colisoes() {
    for (int i = 0; i < MAX_OBJETOS; i++) {
        if (!objetos[i].ativo) continue;

        // Colisão com nave
        if (objetos[i].x == nave.x && objetos[i].y == nave.y) {
            if (objetos[i].tipo == 0 || objetos[i].tipo == 1) {
                game_over = 1;
            } else if (objetos[i].tipo == 2) { // combustível
                combustivel += 30;
                if (combustivel > 100) combustivel = 100;
                objetos[i].ativo = 0;
            }
        }

        // Colisão com tiro
        for (int j = 0; j < MAX_TIROS; j++) {
            if (tiros[j].ativo && tiros[j].x == objetos[i].x && tiros[j].y == objetos[i].y) {
                tiros[j].ativo = 0;
                objetos[i].ativo = 0;
                if (objetos[i].tipo == 1) // só inimigo dá pontos
                    score += 10;
            }
        }
    }
}

void comandos() {
    if (GetAsyncKeyState(VK_LEFT) & 0x8000 && nave.x > 1) {
        nave.x--;
    }
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000 && nave.x < LARGURA - 2) {
        nave.x++;
    }
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
        for (int i = 0; i < MAX_TIROS; i++) {
            if (!tiros[i].ativo) {
                tiros[i].x = nave.x;
                tiros[i].y = nave.y - 1;
                tiros[i].ativo = 1;
                break;
            }
        }
    }
    if (game_over && GetAsyncKeyState(0x52) & 0x8000) { // R
        reset();
    }
}

void desenha_tela() {
    for (int i = 0; i < LARGURA * ALTURA; ++i) {
        consoleBuffer[i].Char.AsciiChar = VAZIO_CHAR;
        consoleBuffer[i].Attributes = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED;
    }

    // Nave
    consoleBuffer[nave.y * LARGURA + nave.x].Char.AsciiChar = NAVE_CHAR;

    // Tiros
    for (int i = 0; i < MAX_TIROS; i++) {
        if (tiros[i].ativo) {
            int idx = tiros[i].y * LARGURA + tiros[i].x;
            if (idx >= 0 && idx < LARGURA * ALTURA)
                consoleBuffer[idx].Char.AsciiChar = TIRO_CHAR;
        }
    }

    // Objetos (inimigos/obstáculos/combustível)
    for (int i = 0; i < MAX_OBJETOS; i++) {
        if (objetos[i].ativo) {
            char c;
            if (objetos[i].tipo == 0) c = OBSTACULO_CHAR;
            else if (objetos[i].tipo == 1) c = INIMIGO_CHAR;
            else c = COMBUSTIVEL_CHAR;

            int idx = objetos[i].y * LARGURA + objetos[i].x;
            if (idx >= 0 && idx < LARGURA * ALTURA)
                consoleBuffer[idx].Char.AsciiChar = c;
        }
    }

    WriteConsoleOutputA(hConsole, consoleBuffer, bufferSize, characterPos, &consoleWriteArea);
    SetConsoleCursorPosition(hConsole, (COORD){0, ALTURA});
    printf("Score: %d | Combustível: %d\n", score, combustivel);
    if (game_over) {
        if (combustivel <= 0)
            printf("Acabou o combustível! Aperte R para reiniciar.\n");
        else
            printf("Você colidiu! Aperte R para reiniciar.\n");
    }
}

int main() {
    srand(time(NULL));
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    reset();

    while (1) {
        if (!game_over) {
            combustivel--;
            if (combustivel <= 0) {
                combustivel = 0;
                game_over = 1;
            }

            spawn_objetos();
            atualiza_tiros();
            atualiza_objetos();
            verificar_colisoes();
        }

        desenha_tela();
        comandos();
        Sleep(DELAY);
    }

    return 0;
} 