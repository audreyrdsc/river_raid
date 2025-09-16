//1 - INCLUSÃO DAS BIBLIOTECAS
#include <stdio.h>          //entrada e saída padrão
#include <stdlib.h>         //funções gerais
#include <time.h>           //geração de números aleatórios diferentes a cada execução

//Compilação condicional - bibliotecas específicas conforme o Sistema Operacional
#ifdef _WIN32   
    #include <conio.h>      //leitura de teclas (Windows)
    #include <windows.h>    //manipulação do console (Windows)
#else           //Se Linux
    #include <ncurses.h>    //manipulação do console (Linux)
    #include <unistd.h>     //funções POSIX (Linux)
#endif

//2 - DEFINIÇÕES E PARÂMETROS GERAIS DO JOGO
#define ALTURA 20            //dimensões do campo (20x50) - Área visível
#define LARGURA 50
#define NAVE_CHAR 'A'        //Caracteres no console para representar a nave
#define TIRO_CHAR '^'        //Caracteres no console para representar tiro
#define OBSTACULO_CHAR '#'   //Caracteres no console para representar obstáculo
#define INIMIGO_CHAR 'E'     //Caracteres no console para representar inimigo
#define COMBUSTIVEL_CHAR 'F' //Caracteres no console para representar combustível
#define VAZIO_CHAR ' '

#define MAX_TIROS 5     //limites de tiros na tela
#define MAX_OBJETOS 20  //objetos simultâneos na tela (inimigos/obstáculos/combustíveis)
#define DELAY 100       //velocidade do jogo, pausa entre atualizações (100ms → 10 frames por segundo)

//Definição simples do rio
#define RIO_INICIO 10
#define RIO_FIM 40

//3 - ESTRUTURAS DE DADOS "OBJETOS DO JOGO"
typedef struct {
    int x, y;   //posição na tela do console
    int ativo;  //1 se existe, 0 se não existe
} Tiro;         //representa cada tiro disparado

//Representa qualquer objeto
typedef struct {
    int x, y;
    int tipo;   // 0 → obstáculo, 1 → inimigo, 2 → combustível
    int ativo;
} Objeto;

typedef struct {
    int x, y;
} Nave;

//4 - VARIÁVEIS GLOBAIS
Nave nave;
Tiro tiros[MAX_TIROS];
Objeto objetos[MAX_OBJETOS];

int score = 0;
int game_over = 0;
int combustivel = 100;

//5 - VARIÁVEIS PARA DESENHAR DIRETAMENTE NO CONSOLE DO WINDOWS
#ifdef _WIN32
HANDLE hConsole;
CHAR_INFO consoleBuffer[LARGURA * ALTURA];
COORD bufferSize = {LARGURA, ALTURA};
COORD characterPos = {0, 0};
SMALL_RECT consoleWriteArea = {0, 0, LARGURA - 1, ALTURA - 1};
#endif

void som_colisao_nave() {
    #ifdef _WIN32
        Beep(300, 100); //Som de colisão com obstáculo
        Beep(150, 120);
    #else
        system("beep -f 300 -l 100");
        system("beep -f 150 -l 120");
    #endif
}

void som_destroi_inimigo() {
    #ifdef _WIN32
        Beep(1200, 150);  // som ao acertar inimigo
    #else
        printf("\a"); fflush(stdout);
    #endif
}    

void som_tiro() {
    #ifdef _WIN32
        Beep(750, 100);  // som de tiro
    #else
        system("beep -f 750 -l 100");   // som ao atirar
    #endif
}

void som_game_over() {
    #ifdef _WIN32
        Beep(880, 200); // Lá5
        Beep(698, 200); // Fá5
        Beep(523, 400); // Dó5
    #else
        system("beep -f 523 -l 200"); // Dó
        system("beep -f 494 -l 200"); // Si
        system("beep -f 440 -l 400"); // Lá
    #endif
}


//6 - FUNÇÃO PARA INICIAR / REINICIAR O JOGO
void reset() {
    nave.x = (RIO_INICIO + RIO_FIM) / 2; // Inicia a nave no centro do rio
    nave.y = ALTURA - 2;
    score = 0;
    combustivel = 100;
    game_over = 0;

    for (int i = 0; i < MAX_TIROS; i++) tiros[i].ativo = 0;
    for (int i = 0; i < MAX_OBJETOS; i++) objetos[i].ativo = 0;
}

//7 - CRIA NOVOS OBJETOS
void gerar_objetos() {
    for (int i = 0; i < MAX_OBJETOS; i++) {
        if (!objetos[i].ativo && rand() % 20 == 0) {
            objetos[i].ativo = 1;
            objetos[i].y = 0;
            // Garante que o objeto nasça dentro do rio
            objetos[i].x = rand() % (RIO_FIM - RIO_INICIO + 1) + RIO_INICIO;
            int tipo_aleatorio = rand() % 100;
            if (tipo_aleatorio < 60) objetos[i].tipo = 0;
            else if (tipo_aleatorio < 90) objetos[i].tipo = 1;
            else objetos[i].tipo = 2;
        }
    }
}

//8 - MOVE TIROS
void atualiza_tiros() {
    for (int i = 0; i < MAX_TIROS; i++) {
        if (tiros[i].ativo) {
            tiros[i].y--;
            if (tiros[i].y < 0) tiros[i].ativo = 0;
        }
    }
}

//9 - MOVE OBJETOS
void atualiza_objetos() {
    for (int i = 0; i < MAX_OBJETOS; i++) {
        if (objetos[i].ativo) {
            objetos[i].y++;
            if (objetos[i].y >= ALTURA) objetos[i].ativo = 0;
        }
    }
}

//10 - VERIFICA COLISÕES
void verificar_colisoes() {
    for (int i = 0; i < MAX_OBJETOS; i++) {
        if (!objetos[i].ativo) continue;

        if (objetos[i].x == nave.x && objetos[i].y == nave.y) {
            if (objetos[i].tipo == 0 || objetos[i].tipo == 1) {            
                som_colisao_nave();
                som_game_over();
                game_over = 1;
            } else if (objetos[i].tipo == 2) {
                combustivel += 30;
                if (combustivel > 100) combustivel = 100;
                objetos[i].ativo = 0;
            }
        }

        for (int j = 0; j < MAX_TIROS; j++) {
            if (tiros[j].ativo && tiros[j].x == objetos[i].x && tiros[j].y == objetos[i].y) {
                tiros[j].ativo = 0;
                
                if (objetos[i].tipo == 2) { //Testa se atirou em combustível
                    objetos[i].ativo = 1;   //mantêm o combustível ativo para capturar 
                } else if (objetos[i].tipo == 0) {
                    objetos[i].ativo = 1;   //mantem o obstáculo (#) ativo - não destrói
                } else {
                    objetos[i].ativo = 0;   //o tiro destrói o Inimigo (E)
                }
              
                if (objetos[i].tipo == 1) {
                    score += 10;
                    som_destroi_inimigo();
                }
              
            }
        }
    }
}

//11 - CONTROLES
void comandos() {
#ifdef _WIN32
    if (GetAsyncKeyState(VK_LEFT) & 0x8000 && nave.x > RIO_INICIO) nave.x--;
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000 && nave.x < RIO_FIM) nave.x++;
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
        for (int i = 0; i < MAX_TIROS; i++) {
            if (!tiros[i].ativo) {
                tiros[i].x = nave.x;
                tiros[i].y = nave.y - 1;
                tiros[i].ativo = 1;
                som_tiro();
                break;
            }
        }
    }
    if (game_over && GetAsyncKeyState(0x52) & 0x8000) reset();
#else
    int ch = getch();
    if (ch == KEY_LEFT && nave.x > RIO_INICIO) nave.x--;
    if (ch == KEY_RIGHT && nave.x < RIO_FIM) nave.x++;
    if (ch == ' ') {
        for (int i = 0; i < MAX_TIROS; i++) {
            if (!tiros[i].ativo) {
                tiros[i].x = nave.x;
                tiros[i].y = nave.y - 1;
                tiros[i].ativo = 1;
                break;
            }
        }
    }
    if (game_over && (ch == 'r' || ch == 'R')) reset();
#endif
}

//12 - DESENHO DA TELA COM FUNDO AZUL/VERDE
void desenha_tela() {
#ifdef _WIN32
    // Primeiro, desenha o fundo (rio e margens)
    for (int y = 0; y < ALTURA; y++) {
        for (int x = 0; x < LARGURA; x++) {
            int idx = y * LARGURA + x;
            consoleBuffer[idx].Char.AsciiChar = ' ';
            if (x >= RIO_INICIO && x <= RIO_FIM) {
                consoleBuffer[idx].Attributes = BACKGROUND_BLUE;
            } else {
                consoleBuffer[idx].Attributes = BACKGROUND_GREEN;
            }
        }
    }

    // Desenha a nave - Cor amarela
    int idx_nave = nave.y * LARGURA + nave.x;
    if (idx_nave >= 0 && idx_nave < LARGURA * ALTURA) {
        consoleBuffer[idx_nave].Char.AsciiChar = NAVE_CHAR;
        consoleBuffer[idx_nave].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | BACKGROUND_BLUE;
    }

    // Desenha os tiros - Cor amarela
    for (int i = 0; i < MAX_TIROS; i++) {
        if (tiros[i].ativo) {
            int idx = tiros[i].y * LARGURA + tiros[i].x;
            if (idx >= 0 && idx < LARGURA * ALTURA) {
                consoleBuffer[idx].Char.AsciiChar = TIRO_CHAR;
                consoleBuffer[idx].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | BACKGROUND_BLUE;
            }
        }
    }

    // Desenha os objetos
    for (int i = 0; i < MAX_OBJETOS; i++) {
        if (objetos[i].ativo) {
            int idx = objetos[i].y * LARGURA + objetos[i].x;
            if (idx >= 0 && idx < LARGURA * ALTURA) {
                char c;
                WORD cor;
                if (objetos[i].tipo == 0) { // Obstáculo - Cor Verde com fundo Magenta
                    c = OBSTACULO_CHAR;
                    cor = FOREGROUND_GREEN | BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
                } else if (objetos[i].tipo == 1) { // Inimigo - Cor Vermelha
                    c = INIMIGO_CHAR;
                    cor = FOREGROUND_RED | BACKGROUND_BLUE;
                } else { // Combustível - Cor Branca
                    c = COMBUSTIVEL_CHAR;
                    cor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_BLUE;
                }
                consoleBuffer[idx].Char.AsciiChar = c;
                consoleBuffer[idx].Attributes = cor;
            }
        }
    }

    WriteConsoleOutputA(hConsole, consoleBuffer, bufferSize, characterPos, &consoleWriteArea);
    SetConsoleCursorPosition(hConsole, (COORD){0, ALTURA});
    printf("Score: %d | Combustivel: %d\n", score, combustivel);
    if (game_over) {
        if (combustivel <= 0) printf("Acabou o combustivel! Aperte R para reiniciar.\n");
        else printf("Voce colidiu! Aperte R para reiniciar.\n");
    }
#else
    clear();
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);   // rio
    init_pair(2, COLOR_BLACK, COLOR_GREEN);  // margem
    init_pair(3, COLOR_YELLOW, COLOR_BLUE);  // nave - amarela
    init_pair(4, COLOR_YELLOW, COLOR_BLUE);  // tiro - amarelo
    init_pair(5, COLOR_GREEN, COLOR_BLUE);   // obstáculo - verde
    init_pair(6, COLOR_RED, COLOR_BLUE);     // inimigo - vermelho
    init_pair(7, COLOR_WHITE, COLOR_BLUE);   // combustível - branco

    for (int y = 0; y < ALTURA; y++) {
        for (int x = 0; x < LARGURA; x++) {
            if (x >= RIO_INICIO && x <= RIO_FIM) {
                attron(COLOR_PAIR(1)); mvaddch(y, x, ' '); attroff(COLOR_PAIR(1));
            } else {
                attron(COLOR_PAIR(2)); mvaddch(y, x, ' '); attroff(COLOR_PAIR(2));
            }
        }
    }

    // Desenha a nave
    attron(COLOR_PAIR(3)); mvaddch(nave.y, nave.x, NAVE_CHAR); attroff(COLOR_PAIR(3));

    // Desenha os tiros
    for (int i = 0; i < MAX_TIROS; i++) {
        if (tiros[i].ativo) {
            attron(COLOR_PAIR(4));
            mvaddch(tiros[i].y, tiros[i].x, TIRO_CHAR);
            attroff(COLOR_PAIR(4));
        }
    }

    // Desenha os objetos
    for (int i = 0; i < MAX_OBJETOS; i++) {
        if (objetos[i].ativo) {
            int cor = (objetos[i].tipo == 0 ? 5 : (objetos[i].tipo == 1 ? 6 : 7));
            attron(COLOR_PAIR(cor));
            mvaddch(objetos[i].y, objetos[i].x,
                     objetos[i].tipo == 2 ? COMBUSTIVEL_CHAR :
                     (objetos[i].tipo == 1 ? INIMIGO_CHAR : OBSTACULO_CHAR));
            attroff(COLOR_PAIR(cor));
        }
    }

    mvprintw(ALTURA, 0, "Score: %d | Combustivel: %d", score, combustivel);
    if (game_over) {
        if (combustivel <= 0) mvprintw(ALTURA + 1, 0, "Acabou o combustivel! Aperte R para reiniciar.");
        else mvprintw(ALTURA + 1, 0, "Voce colidiu! Aperte R para reiniciar.");
    }
    refresh();
#endif
}

//13 - FUNÇÃO PRINCIPAL
int main() {
#ifdef __linux__
    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
#endif
    srand(time(NULL));
#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
    reset();

    while (1) {
        // CORREÇÃO: Garante que o combustível nunca ultrapasse 100
        if (combustivel > 100) {
            combustivel = 100;
        }      

        if (!game_over) {
            combustivel--;
            if (combustivel <= 0) { combustivel = 0; game_over = 1; }
            gerar_objetos();
            atualiza_tiros();
            atualiza_objetos();
            verificar_colisoes();
        }

        desenha_tela();
        comandos();
#ifdef _WIN32
        Sleep(DELAY);
#else
        usleep(DELAY * 1000);
#endif
    }

#ifdef __linux__
    endwin();
#endif
    return 0;
}