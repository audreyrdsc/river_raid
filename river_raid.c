
//1 - INCLUSÃO DAS BIBLIOTECAS
#include <stdio.h>          //entrada e saída padrão
#include <stdlib.h>         //funções gerais, gerenciamento de memória, processos, números aleatórios
#include <time.h>           //medição de tempo atual e decorrido

//Compilação condicional - bibliotecas específicas conforme o Sistema Operacional
#ifdef _WIN32   
    #include <conio.h>      //leitura de teclas (Windows)
    #include <windows.h>    //manipulação do console (Windows)
#else           //Se Linux
    #include <ncurses.h>    //manipulação do console (Linux)
    #include <unistd.h>     //funções POSIX (Linux), I/O, manipulação de arquivos, controle do sistema
#endif

// Funções de abstração para leitura de teclado e manipulação de console
// Implementações separadas para Windows e Linux adicionadas abaixo     
  
 
//2 - DEFINIÇÕES E PARÂMETROS GERAIS DO JOGO
#define ALTURA 20               //dimensões do campo (20x50) - Área visível
#define LARGURA 50
#define NAVE_CHAR 'A'           //Caracteres no console para representar a nave
#define TIRO_CHAR '^'           //Caracteres no console para representar tiro
#define OBSTACULO_CHAR '#'      //Caracteres no console para representar obstáculo
#define INIMIGO_CHAR 'E'        //Caracteres no console para representar inimigo
#define COMBUSTIVEL_CHAR 'F'    //Caracteres no console para representar combustível
#define VAZIO_CHAR ' '

#define MAX_TIROS 5     //limites de tiros na tela
#define MAX_OBJETOS 20  //objetos simultâneos na tela (inimigos/obstáculos/combustíveis)
#define DELAY 100       //velocidade do jogo, pausa entre atualizações (100ms → 10 frames por segundo)

//3 - ESTRUTURAS DE DADOS "OBJETOS DO JOGO"
//Representa cada tiro
typedef struct {
    int x, y;   //posição na tela do console
    int ativo;  //1 se existe, 0 se não existe
} Tiro;         //representa cada tiro disparado

//Representa qualquer dos 3 objetos
typedef struct {
    int x, y;
    int tipo;   // 0 → obstáculo, 1 → inimigo, 2 → combustível
    int ativo;
} Objeto;       //pode ser um obstáculo, inimigo ou combustível

//Representa a nave do jogador
typedef struct {
    int x, y;   //posição da nave
} Nave;         //representa a nave

//4 - VARIÁVEIS GLOBAIS - controlam o estado geral do jogo
Nave nave;                  //Posição da nave
Tiro tiros[MAX_TIROS];      //Lista de tiros ativos
Objeto objetos[MAX_OBJETOS];//Lista de inimigos, obstáculos e combustíveis

int score = 0;              //Pontos acumulados
int game_over = 0;          //Fim do jogo
int combustivel = 100;      //Combustível inicial da nave (100)

// 5 - VARIÁVEIS PARA DESENHAR DIRETAMENTE NO CONSOLE DO WINDOWS
#ifdef _WIN32       //Diretiva de compilação condicional: compilado quando Windowss
HANDLE hConsole;    //Declara um handle (ponteiro/opaco) para um objeto do sistema
CHAR_INFO consoleBuffer[LARGURA * ALTURA];  //Declara um vetor com LARGURA * ALTURA elementos do tipo CHAR_INFO (Caractere, cor, brilho)
COORD bufferSize = {LARGURA, ALTURA};       //COORD é uma estrutura da API do console com dois SHORT: X e Y
COORD characterPos = {0, 0};                //indica a posição inicial (offset) dentro do consoleBuffer a partir da qual a API
SMALL_RECT consoleWriteArea = {0, 0, LARGURA - 1, ALTURA - 1}; //Define a região da janela do console que será atualizada
#endif  //Finaliza o bloco de compilação condicional iniciado em #ifdef _WIN32

//6 - FUNÇÃO PARA INICIAR / REINICIAR O JOGO
void reset() {
    nave.x = LARGURA / 2;   //Centraliza a nave
    nave.y = ALTURA - 2;    //Posiciona embaixo
    score = 0;              //zera pontuação
    combustivel = 100;      //recarrega combustível
    game_over = 0;          //desativa todos os tiros e objetos

    for (int i = 0; i < MAX_TIROS; i++) {
        tiros[i].ativo = 0;   //Todos os tiros começam inativos
    }

    for (int i = 0; i < MAX_OBJETOS; i++) {
        objetos[i].ativo = 0; //Todos os objetos começam inativos
    }
}

//7 - FUNÇÃO RESPONSÁVEL POR CRIAR NOVOS OBJETOS ALEATÓRIOS NA PARTE SUPERIOR DA TELA COM BASE EM PROBABILIDADE
    //60% de chance de ser obstáculo
    //30% de ser inimigo
    //10% de ser combustível.
    //jogo mais dinâmico, desafios e oportunidades aleatórias
void gerar_objetos() {
    for (int i = 0; i < MAX_OBJETOS; i++) {
        if (!objetos[i].ativo && rand() % 20 == 0) {    //Condição para gerar na posição i: objeto inativo e chance 1/20 = 5% por iteração.
            objetos[i].ativo = 1;                       //Marca o slot (objeto) como ativo
            objetos[i].x = rand() % (LARGURA - 2) + 1;  //Define a coluna (x) onde o objeto nasce (bordas 0 e LARGURA-1 são evitadas)
            objetos[i].y = 0;                           //Define a linha (y) inicial no topo da tela (linha 0)
            int tipo_aleatorio = rand() % 100;          //Gera números aleatórios de 0 a 99
            if (tipo_aleatorio < 60)                    //0 a 59 (60 valores)
                objetos[i].tipo = 0; // obstáculo
            else if (tipo_aleatorio < 90)               //60 a 89 (30 valores)
                objetos[i].tipo = 1; // inimigo
            else                                        //90 a 99 (10 valores)
                objetos[i].tipo = 2; // combustível
        }
    }
}

//8 - FUNÇÃO QUE MOVE OS TIROS PARA CIMA - Se saírem da tela (y < 0), são desativados
void atualiza_tiros() {     //percorre o vetor global tiros[] e atualiza a posição vertical (y) de cada tiro ativo
    for (int i = 0; i < MAX_TIROS; i++) {   //Usa variáveis globais
        if (tiros[i].ativo) {               //Verifica se o slot i contém um tiro ativo
            tiros[i].y--;                   //Decrementa a coordenada vertical y do tiro: faz o tiro subir uma linha
            if (tiros[i].y < 0) {           //Checa se o tiro ultrapassou o topo da tela
                tiros[i].ativo = 0;         //Marca o tiro como inativo
            }
        }
    }
}

//9 - FUNÇÃO QUE MOVE OS OBJETOS PARA BAIXO - se saírem da tela, são desativados
void atualiza_objetos() {                   //Move os objetos ativos para baixo no eixo vertical (y)
    for (int i = 0; i < MAX_OBJETOS; i++) { //Percorre todos os slots do vetor objetos[]
        if (objetos[i].ativo) {             //Processa apenas o objeto se ele estiver ativo (1)
            objetos[i].y++;                 //Faz o objeto descer uma linha na tela
            if (objetos[i].y >= ALTURA) {   //Verifica se o objeto passou da borda inferior da tela
                objetos[i].ativo = 0;       //Marca o objeto como inativo - evita desenhar e processar nos frames seguintes
            }
        }
    }
}

//10 - FUNÇÃO RESPONSÁVEL POR VERIFICAR COLISÕES
    //Verifica duas coisas:
        //Colisão com a nave:
            //Obstáculo ou inimigo = fim de jogo.
            //Combustível = recarrega.
        //Colisão com tiros:
            //Se tiro acerta um inimigo, ambos são desativados e o jogador ganha pontos.
void verificar_colisoes() {
    for (int i = 0; i < MAX_OBJETOS; i++) {
        if (!objetos[i].ativo) continue;

        // Colisão com nave
        if (objetos[i].x == nave.x && objetos[i].y == nave.y) { //Verifica se a posição do objeto coincide com a posição da nave → colisão direta
            if (objetos[i].tipo == 0 || objetos[i].tipo == 1) { //tipo == 0 → obstáculo ou tipo == 1 → inimigo (game over)
                game_over = 1;
            } else if (objetos[i].tipo == 2) {  //tipo == 2 → combustível - colisão da nave com combustível
                combustivel += 30;              //Acrescenta +30 de combustível
                if (combustivel > 100) combustivel = 100;   //Impede que o combustível ultrapasse o máximo (100)
                objetos[i].ativo = 0;           //O objeto que colidiu some da tela
            }
        }

        // Colisão com tiro
        for (int j = 0; j < MAX_TIROS; j++) {   //Percorre todos os tiros ativos
            if (tiros[j].ativo && tiros[j].x == objetos[i].x && tiros[j].y == objetos[i].y) { //Verifica se o tiro j está ativo e se atingiu objeto (x, y)
                tiros[j].ativo = 0;         //O tiro é desativado (desaparece)
                objetos[i].ativo = 0;       //O objeto também some
                if (objetos[i].tipo == 1)   //Se acertar inimigo incrementa 10 pontos
                    score += 10;        
            }
        }
    }
}

//11 - FUNÇÃO QUE PERMITE CONTROLE EM TEMPO REAL DA NAVE
    //Lê as teclas pressionadas:
        //← e →: movimentam a nave
        //Espaço: atira
        //R: reinicia (após morte)
void comandos() {
#ifdef _WIN32   //Diretiva de compilação condicional, se Windows
    //Função da API do Windows que retorna o estado da tecla seta esquerda
    if (GetAsyncKeyState(VK_LEFT) & 0x8000 && nave.x > 1) {
        //& 0x8000 → Máscara usada para verificar se a tecla está pressionada
        //nave.x > 1 → Evita que a nave vá para além da borda esquerda da tela
        nave.x--;   //Move a nave 1 unidade para a esquerda
    }
    //Função da API do Windows que retorna o estado da tecla seta direita
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000 && nave.x < LARGURA - 2) { //(nave.x < LARGURA - 2) Garante que a nave não ultrapasse a borda direita
        nave.x++;   //Move a nave 1 unidade para a direita
    }
    //Função da API do Windows que retorna o estado da tecla espaço
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
        for (int i = 0; i < MAX_TIROS; i++) {   //Percorre o vetor tiros[]
            if (!tiros[i].ativo) {              //Procura o primeiro tiro inativo
                tiros[i].x = nave.x;            //Coloca o tiro na posição (x) da nave
                tiros[i].y = nave.y - 1;        //Coloca o tiro na posição (y) da nave um pouco acima
                tiros[i].ativo = 1;             //Ativa o tiro
                break;
            }
        }
    }
    if (game_over && GetAsyncKeyState(0x52) & 0x8000) { // Clicar R - 0x52 → Código hexadecimal da tecla R
        reset();                                        //Só funciona se o jogo estiver em estado de Game Over
    }
#else
    int ch = getch();                               //Lê teclas diretamente do terminal sem precisar dar Enter
    if (ch == KEY_LEFT && nave.x > 1) {             //Se a tecla for seta esquerda (KEY_LEFT) → move a nave para a esquerda
        nave.x--;
    }
    if (ch == KEY_RIGHT && nave.x < LARGURA - 2) {  //Se for seta direita (KEY_RIGHT) → move para a direita
        nave.x++;
    }
    if (ch == ' ') {    //Atira se pressionar a tecla espaço
        for (int i = 0; i < MAX_TIROS; i++) {
            if (!tiros[i].ativo) {
                tiros[i].x = nave.x;
                tiros[i].y = nave.y - 1;
                tiros[i].ativo = 1;
                break;
            }
        }
    }
    if (game_over && (ch == 'r' || ch == 'R')) { //Se o jogo estiver em Game Over e a tecla for r ou R, chama reset()
        reset();
    }
#endif
}

//12 - FUNÇÃO QUE RENDERIZA O ESTADO ATUAL DO JOGO NO CONSOLE:
    //Limpa o buffer.
    //Desenha nave, tiros, objetos.
    //Escreve a pontuação e combustível.
void desenha_tela() {
#ifdef _WIN32   //Diretiva de compilação condicional, se Windows
    for (int i = 0; i < LARGURA * ALTURA; ++i) {        //Percorre toda a tela (cada posição é uma célula do console)
        consoleBuffer[i].Char.AsciiChar = VAZIO_CHAR;   //Define cada posição como vazia
        consoleBuffer[i].Attributes = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED;  //pinta de branco (mistura RGB no console)
    }

    // Coloca o caractere da nave na posição (x, y)
    consoleBuffer[nave.y * LARGURA + nave.x].Char.AsciiChar = NAVE_CHAR;

    // Coloca o caractere de tiro na posição (x, y)
    for (int i = 0; i < MAX_TIROS; i++) {   //Percorre todos os tiros
        if (tiros[i].ativo) {
            int idx = tiros[i].y * LARGURA + tiros[i].x;    //Se o tiro estiver ativo, calcula a posição (idx)
            if (idx >= 0 && idx < LARGURA * ALTURA)         //Se estiver dentro da tela, desenha o tiro
                consoleBuffer[idx].Char.AsciiChar = TIRO_CHAR;
        }
    }

    // Objetos (inimigos/obstáculos/combustível)
    for (int i = 0; i < MAX_OBJETOS; i++) { //Percorre todos os objetos
        if (objetos[i].ativo) {             //Se objeto ativo, escolhe o caractere de acordo com o tipo
            char c;
            if (objetos[i].tipo == 0) c = OBSTACULO_CHAR;       //Define caractere #
            else if (objetos[i].tipo == 1) c = INIMIGO_CHAR;    //Define caractere E
            else c = COMBUSTIVEL_CHAR;                          //Define caractere F

            int idx = objetos[i].y * LARGURA + objetos[i].x;    //Calcula posição do objeto na tela
            if (idx >= 0 && idx < LARGURA * ALTURA)
                consoleBuffer[idx].Char.AsciiChar = c;          //Desenha o caractere
        }
    }

    //Atualiza toda a tela de uma vez - desenho fica suave (sem flickering)
    WriteConsoleOutputA(hConsole, consoleBuffer, bufferSize, characterPos, &consoleWriteArea);
    //Move o cursor do console para baixo da tela e imprime o placar e o combustível restante
    SetConsoleCursorPosition(hConsole, (COORD){0, ALTURA});
    printf("Score: %d | Combustível: %d\n", score, combustivel);

    if (game_over) {    //Se o jogo terminou - Mostra mensagem diferente se foi colisão ou falta de combustível
        if (combustivel <= 0)
            printf("Acabou o combustível! Aperte R para reiniciar.\n");
        else
            printf("Você colidiu! Aperte R para reiniciar.\n");
    }
#else
    clear();    //Limpa toda a tela
    // Desenha a Nave
    mvaddch(nave.y, nave.x, NAVE_CHAR);
    // Desenha os Tiros
    for (int i = 0; i < MAX_TIROS; i++) {
        if (tiros[i].ativo)
            mvaddch(tiros[i].y, tiros[i].x, TIRO_CHAR); //Para cada tiro ativo, desenha na tela o caractere de tiro ^
    }
    // Desenha os objetos - Idêntico ao Windows, mas usando mvaddch da ncurses
    for (int i = 0; i < MAX_OBJETOS; i++) {
        if (objetos[i].ativo) {
            char c;
            if (objetos[i].tipo == 0) c = OBSTACULO_CHAR;
            else if (objetos[i].tipo == 1) c = INIMIGO_CHAR;
            else c = COMBUSTIVEL_CHAR;
            mvaddch(objetos[i].y, objetos[i].x, c);
        }
    }
    //Mostra os valores na linha abaixo da tela do jogo (placar e combustível)
    mvprintw(ALTURA, 0, "Score: %d | Combustível: %d", score, combustivel);
    //Mensagem final em caso de game over
    if (game_over) {
        if (combustivel <= 0)
            mvprintw(ALTURA + 1, 0, "Acabou o combustível! Aperte R para reiniciar.");
        else
            mvprintw(ALTURA + 1, 0, "Você colidiu! Aperte R para reiniciar.");
    }
    refresh();  //Atualizar tela
#endif
}

//13 - FUNÇÃO PRINCIPAL
    //Loop principal:
        //Se o jogo ainda não acabou:
            //Gasta combustível
            //Gera novos objetos
            //Atualiza movimento de tiros e objetos
            //verifica colisões
        //Sempre:
            //desenha tela
            //lê comandos do jogador
            //espera 100 ms (delay).
int main() {
#ifdef __linux__            //Bloco condicional para Linux (ncurses)
    initscr();              //Inicia o modo ncurses, que permite desenhar no terminal
    noecho();               //Impede que as teclas digitadas apareçam na tela (sem eco)
    curs_set(FALSE);        //Oculta o cursor do terminal
    keypad(stdscr, TRUE);   //Habilita leitura de teclas especiais (setas, F1 etc)
    nodelay(stdscr, TRUE);  //Leitura não bloqueante do teclado (o getch() não trava esperando input)
#endif
    srand(time(NULL));      //Inicializa o gerador de números aleatórios
#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE); //Obtém o handle (ponteiro para dispositivo de saída padrão = console)
#endif
    reset();

    while (1) {
        if (!game_over) {           //Verifica se o jogo ainda está rodando
            combustivel--;          //Diminua o combustível (um decremento por loop)
            if (combustivel <= 0) { //Se combustível <= 0 zerar combustível e GAME OVER
                combustivel = 0;
                game_over = 1;
            }

            gerar_objetos();        //Criação aleatória dos objetos (# E F)
            atualiza_tiros();       //Move os tiros para cima - Desativa os que saírem da tela
            atualiza_objetos();     //Move os objetos para baixo - Desativa os que saírem da tela
            verificar_colisoes();   //Colisão com a nave (#, E, F) e objetos com tiros
        }

        desenha_tela();             //Renderiza a tela - Desenha o frame atual
        comandos();                 //Lê as teclas pressionadas
#ifdef _WIN32
        Sleep(DELAY);               //No Windows → Sleep() já recebe milissegundos
#else
        usleep(DELAY * 1000);       //No Linux → usleep() recebe microssegundos → multiplica por 1000
#endif
    }

#ifdef __linux__
    endwin();                       //Para Linux/ncurses: restaura o terminal ao estado normal antes de sair
#endif
    return 0;                       //Encerra o programa corretamente
}