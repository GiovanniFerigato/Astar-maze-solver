// ====================================================================
// RESOLUÇÃO AUTÔNOMA DE JOGOS COM ESTRUTURAS DE DADOS E IA CLÁSSICA
// Labirinto com custo dinâmico: jogo manual e busca A*
// ====================================================================
// O algoritmo A* é o mesmo de antes (NoBusca, menorF, processarVizinho,
// expandirVizinhos). Tudo que é desenho fica na "CAMADA VISUAL".
// ====================================================================
#include <stdio.h>
#include <stdlib.h>
#include "FILA.h"

// TAM é o tamanho MÁXIMO da matriz (o do labirinto difícil).
// tam é o tamanho do labirinto escolhido agora (8, 12 ou 17).
// As matrizes são criadas com TAM, mas os laços e os limites usam tam.
#define TAM 17
int tam = 12;
const char *nomeNivel = "Médio (12x12)";

// Início sempre no canto de cima à esquerda, chegada no canto de baixo à direita
#define LINHA_INICIO  0
#define COLUNA_INICIO 0
#define LINHA_FIM     (tam - 1)
#define COLUNA_FIM    (tam - 1)

// 1 = abre o programa sozinho no Windows Terminal (emoji colorido)
// 0 = roda na janela normal do CodeBlocks
#define ABRIR_NO_WINDOWS_TERMINAL 1

// Velocidades das animações, em milissegundos (menor = mais rápido)
#define VELOCIDADE_BUSCA 40
#define VELOCIDADE_CAMINHO 25
#define VELOCIDADE_ROBO  110

// Posição das coisas na tela (em colunas e linhas do console)
// Tudo foi pensado para caber em 30 linhas (o Windows Terminal não deixa mudar o tamanho)
#define Y_MAPA    5     // linha da primeira casa do mapa
// Coluna da direita: legenda, caixa de resultado e caixa da heurística, uma embaixo da outra
#define LARGURA_PAINEL 44
#define Y_LEGENDA   (Y_MAPA - 1)   // ocupa 12 linhas
#define Y_RESULTADO (Y_MAPA + 11)  // ocupa 7 linhas
#define Y_HEURISTICA (Y_MAPA + 18) // ocupa 6 linhas
#define Y_RODAPE    29             // "Pressione uma tecla..."

// Tela da lista de coordenadas (caminho do A* ou histórico da fila)
#define POR_LINHA 11            // coordenadas por linha
#define MAX_LINHAS_LISTA 20     // no máximo 20 linhas; o resto vira "..."
#define MAX_HISTORICO 1000      // movimentos guardados do jogo manual

// ====================================================================
// CAMADA DE SISTEMA: funções que dependem do Windows
// (no Windows usa windows.h e conio.h; em outro sistema usa um plano B)
// ====================================================================
#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>

    #ifndef ENABLE_QUICK_EDIT_MODE
        #define ENABLE_QUICK_EDIT_MODE 0x0040
    #endif
    #ifndef ENABLE_EXTENDED_FLAGS
        #define ENABLE_EXTENDED_FLAGS 0x0080
    #endif

    // fundo e texto vão de 0 a 15 (0 preto, 7 cinza claro, 15 branco...)
    void cor(int fundo, int texto)
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), fundo * 16 + texto);
    }

    // move o cursor para a coluna x e a linha y
    void irPara(int x, int y)
    {
        COORD posicao;
        posicao.X = x;
        posicao.Y = y;
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), posicao);
    }

    // Relógio de alta precisão do Windows, em milissegundos (para medir a busca)
    double agoraMs()
    {
        LARGE_INTEGER frequencia;
        LARGE_INTEGER contador;
        QueryPerformanceFrequency(&frequencia);
        QueryPerformanceCounter(&contador);
        return (double)contador.QuadPart * 1000.0 / (double)frequencia.QuadPart;
    }

    void pausa(int milissegundos)
    {
        Sleep(milissegundos);
    }

    void som(int frequencia, int milissegundos)
    {
        Beep(frequencia, milissegundos);
    }

    void mostrarCursor(int mostrar)
    {
        CONSOLE_CURSOR_INFO info;
        info.dwSize = 100;
        info.bVisible = mostrar;
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    }

    void limparTelaSistema()
    {
        system("cls");
    }

    void configurarConsole()
    {
        SetConsoleOutputCP(CP_UTF8);                    // acentos, molduras e emoji
        // O console antigo do Windows desenha emoji em preto e branco.
        // Se não estamos no Windows Terminal (que tem a variável WT_SESSION)
        // e ele está instalado, abrimos o programa de novo lá e fechamos este.
        if(ABRIR_NO_WINDOWS_TERMINAL && getenv("WT_SESSION") == NULL
           && system("where wt >nul 2>nul") == 0)
        {
            char caminho[MAX_PATH];
            char comando[MAX_PATH + 40];
            GetModuleFileNameA(NULL, caminho, MAX_PATH);
            sprintf(comando, "start \"\" wt.exe \"%s\"", caminho);
            system(comando);
            exit(0);
        }

        system("mode con cols=110 lines=32");            // tamanho da janela
        SetConsoleTitleW(L"Resolução Autônoma de Jogos com Estruturas de Dados e IA Clássica");

        // desliga o "modo de seleção": clicar na janela não congela mais o programa
        HANDLE entrada = GetStdHandle(STD_INPUT_HANDLE);
        DWORD modo;
        GetConsoleMode(entrada, &modo);
        SetConsoleMode(entrada, (modo & ~ENABLE_QUICK_EDIT_MODE) | ENABLE_EXTENDED_FLAGS);

        mostrarCursor(0);
    }

    // Lê UMA tecla, sem precisar de Enter.
    // Devolve 'W' 'A' 'S' 'D' (setas também), 'Q' (ESC também), ou a letra em maiúsculo.
    int lerTecla()
    {
        int t = _getch();
        if(t == 0 || t == 224)          // as setas mandam dois códigos
        {
            t = _getch();
            if(t == 72) return 'W';     // seta para cima
            if(t == 80) return 'S';     // seta para baixo
            if(t == 75) return 'A';     // seta para a esquerda
            if(t == 77) return 'D';     // seta para a direita
            return 0;
        }
        if(t >= 'a' && t <= 'z') t = t - 32;
        if(t == 27) return 'Q';
        return t;
    }
#else
    #include <unistd.h>
    #include <time.h>

    double agoraMs()
    {
        struct timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);
        return t.tv_sec * 1000.0 + t.tv_nsec / 1000000.0;
    }

    // Plano B para testar fora do Windows (usa códigos ANSI)
    void cor(int fundo, int texto)
    {
        int ansi[16] = {30, 34, 32, 36, 31, 35, 33, 37, 90, 94, 92, 96, 91, 95, 93, 97};
        printf("\033[%d;%dm", ansi[texto], ansi[fundo] + 10);
    }
    void irPara(int x, int y)            { printf("\033[%d;%dH", y + 1, x + 1); }
    void pausa(int milissegundos)        { fflush(stdout); usleep(milissegundos * 1000); }
    void som(int frequencia, int ms)     { }
    void mostrarCursor(int mostrar)      { printf("\033[?25%c", mostrar ? 'h' : 'l'); }
    void limparTelaSistema()             { printf("\033[2J"); }
    void configurarConsole()             { mostrarCursor(0); }

    int lerTecla()
    {
        int t = getchar();
        while(t == '\n') t = getchar();
        if(t == EOF) return 'Q';
        if(t >= 'a' && t <= 'z') t = t - 32;
        return t;
    }
#endif

// ====================================================================
// A* (a parte que importa para a arguição)
// ====================================================================
typedef struct noBusca
{
    int linha;
    int coluna;
    int g;
    int h;
    int f;
    int fechado;   // 0 = ainda pode ser escolhido, 1 = já foi explorado
    struct noBusca *prox;
    struct noBusca *anterior;
} NoBusca;

NoBusca* menorF(NoBusca *lista)
{
    NoBusca *melhor = NULL;
    NoBusca *aux = lista;

    while(aux != NULL)
    {
        if(aux->fechado == 0)
        {
            if(melhor == NULL || aux->f < melhor->f)
            {
                melhor = aux;
            }
        }
        aux = aux->prox;
    }

    return melhor;
}

// 1 = A* normal (usa a distância de Manhattan)
// 0 = sem heurística: h vale sempre 0, então f = g e o A* vira o algoritmo de Dijkstra
int usarHeuristica = 1;

int heuristica(int linhaFim, int colunaFim, int colunaAtual, int linhaAtual)
{
    if(usarHeuristica == 0)
    {
        return 0;
    }

    int h = abs(linhaAtual - linhaFim) + abs(colunaAtual - colunaFim);

    return h;
}

NoBusca* buscarNaLista(NoBusca *lista, int linha, int coluna)
{
    NoBusca *aux = lista;

    while(aux != NULL)
    {
        if(aux->linha == linha && aux->coluna == coluna)
        {
            return aux;
        }
        aux = aux->prox;
    }

    return NULL;
}

NoBusca* criarVizinho(NoBusca *atual, int novaLinha, int novaColuna, int labirinto[][TAM], int linhaFim, int colunaFim)
{
    NoBusca *vizinho;
    vizinho = (NoBusca *)malloc(sizeof(NoBusca));

    vizinho->linha = novaLinha;
    vizinho->coluna = novaColuna;
    vizinho->g = atual->g + labirinto[novaLinha][novaColuna];
    vizinho->h = heuristica(linhaFim, colunaFim, novaColuna, novaLinha);
    vizinho->f = vizinho->g + vizinho->h;
    vizinho->anterior = atual;
    vizinho->fechado = 0;
    vizinho->prox = NULL;

    return vizinho;
}

// Decide o que fazer com UM vizinho válido:
// 1) se ele já existe na lista e está fechado, ignora
// 2) se ele já existe na lista e ainda está aberto, só atualiza quando o novo caminho é mais barato
// 3) se é novo, cria o nó e coloca na lista
NoBusca* processarVizinho(NoBusca *atual, NoBusca *lista, int novaLinha, int novaColuna, int labirinto[][TAM], int linhaFim, int colunaFim)
{
    int novoG = atual->g + labirinto[novaLinha][novaColuna];
    NoBusca *existente = buscarNaLista(lista, novaLinha, novaColuna);

    if(existente != NULL)
    {
        if(existente->fechado == 0 && novoG < existente->g)
        {
            existente->g = novoG;
            existente->f = existente->g + existente->h;
            existente->anterior = atual;
        }
        return lista;
    }

    NoBusca *vizinho = criarVizinho(atual, novaLinha, novaColuna, labirinto, linhaFim, colunaFim);
    vizinho->prox = lista;
    return vizinho;
}

NoBusca* expandirVizinhos(NoBusca *atual, NoBusca *lista, int labirinto[][TAM], int linhaFim, int colunaFim)
{
    if(atual->linha > 0 && labirinto[atual->linha - 1][atual->coluna] != 0)
    {
        lista = processarVizinho(atual, lista, atual->linha - 1, atual->coluna, labirinto, linhaFim, colunaFim);
    }

    if(atual->linha + 1 < tam && labirinto[atual->linha + 1][atual->coluna] != 0)
    {
        lista = processarVizinho(atual, lista, atual->linha + 1, atual->coluna, labirinto, linhaFim, colunaFim);
    }

    if(atual->coluna > 0 && labirinto[atual->linha][atual->coluna - 1] != 0)
    {
        lista = processarVizinho(atual, lista, atual->linha, atual->coluna - 1, labirinto, linhaFim, colunaFim);
    }

    if(atual->coluna + 1 < tam && labirinto[atual->linha][atual->coluna + 1] != 0)
    {
        lista = processarVizinho(atual, lista, atual->linha, atual->coluna + 1, labirinto, linhaFim, colunaFim);
    }

    return lista;
}

void liberarLista(NoBusca *lista)
{
    while(lista != NULL)
    {
        NoBusca *proximo = lista->prox;
        free(lista);
        lista = proximo;
    }
}

// ====================================================================
// CAMADA VISUAL
// ====================================================================

// Códigos das casas desenhadas. Os valores 0, 1, 3 e 5 são o próprio terreno da matriz.
#define T_PAREDE    0
#define T_LIMPO     1
#define T_LAMA      3
#define T_AGUA      5
#define T_EXPLORADA 10   // já explorada pela busca (lista fechada)
#define T_ABERTA    11   // descoberta, mas ainda não explorada (lista aberta)
#define T_CAMINHO   12   // parte do caminho
#define T_ROBO      13
#define T_INICIO    14
#define T_FIM       15

int usarEmoji = 1;          // 1 = emoji, 0 = blocos coloridos (troca no menu, tecla 3)

// Cada casa ocupa 2 colunas; no modo emoji ganha mais 1 de espaço entre as casas
int larguraTile()
{
    if(usarEmoji) return 3;
    return 2;
}

#define X_MAPA   (1 + larguraTile())                        // coluna da primeira casa do mapa
#define X_PAINEL (X_MAPA + (tam + 1) * larguraTile() + 2)   // coluna da caixa da legenda
int cache[TAM][TAM];        // o que já está desenhado na tela (evita redesenhar e piscar)

void limparCache()
{
    for(int i = 0; i < TAM; i++)
    {
        for(int j = 0; j < TAM; j++)
        {
            cache[i][j] = -1;
        }
    }
}

void limparTela()
{
    cor(0, 7);
    limparTelaSistema();
    limparCache();
}

// Escreve um texto na posição (x, y) com a cor pedida
void escreverEm(int x, int y, int corTexto, const char *texto)
{
    irPara(x, y);
    cor(0, corTexto);
    printf("%s", texto);
    cor(0, 7);
}

// Apaga uma linha inteira da tela
void limparLinha(int y)
{
    irPara(0, y);
    cor(0, 7);
    printf("%95s", "");
}

// Desenha uma caixa com linha dupla
void desenharCaixa(int x, int y, int largura, int altura, int corLinha)
{
    cor(0, corLinha);

    irPara(x, y);
    printf("╔");
    for(int i = 0; i < largura - 2; i++) printf("═");
    printf("╗");

    for(int i = 1; i < altura - 1; i++)
    {
        irPara(x, y + i);
        printf("║");
        irPara(x + largura - 1, y + i);
        printf("║");
    }

    irPara(x, y + altura - 1);
    printf("╚");
    for(int i = 0; i < largura - 2; i++) printf("═");
    printf("╝");

    cor(0, 7);
}

// Desenha UMA casa (ocupa 2 colunas) na posição atual do cursor
void desenharTile(int codigo, int custo)
{
    if(usarEmoji)
    {
        // No console antigo do Windows o emoji sai em preto e branco e usa a cor do texto,
        // por isso cada casa recebe a sua cor antes de ser desenhada.
        // (No Windows Terminal o emoji já é colorido e essas cores não atrapalham.)
        // Fundo preto, como no preview. No Windows Terminal o emoji já vem colorido;
        // a cor do texto só aparece se o programa rodar no console antigo.
        switch(codigo)
        {
            case T_PAREDE:    cor(0, 6);  printf("\xF0\x9F\xA7\xB1"); break;   // tijolo
            case T_LIMPO:     cor(0, 10); printf("\xF0\x9F\x9F\xA9"); break;   // quadrado verde
            case T_LAMA:      cor(0, 6);  printf("\xF0\x9F\x9F\xAB"); break;   // quadrado marrom
            case T_AGUA:      cor(0, 9);  printf("\xF0\x9F\x8C\x8A"); break;   // onda
            case T_EXPLORADA: cor(0, 11); printf("\xF0\x9F\x94\x8D"); break;   // lupa
            case T_ABERTA:    cor(0, 3);  printf("\xF0\x9F\x9F\xA6"); break;   // quadrado azul
            case T_CAMINHO:   cor(0, 13); printf("\xF0\x9F\x91\xA3"); break;   // pegadas
            case T_ROBO:      cor(0, 15); printf("\xF0\x9F\xA4\x96"); break;   // robô
            case T_INICIO:    cor(0, 12); printf("\xF0\x9F\x9A\xA9"); break;   // bandeira
            case T_FIM:       cor(0, 15); printf("\xF0\x9F\x8F\x81"); break;   // chegada
        }
        cor(0, 7);
        printf(" ");   // espaço entre as casas
    }
    else
    {
        switch(codigo)
        {
            case T_PAREDE:    cor(8, 7);   printf("▒▒");         break;
            case T_LIMPO:     cor(10, 0);  printf("  ");         break;
            case T_LAMA:      cor(6, 0);   printf("  ");         break;
            case T_AGUA:      cor(9, 0);   printf("  ");         break;
            case T_EXPLORADA: cor(11, 0);  printf("%d ", custo); break;
            case T_ABERTA:    cor(3, 0);   printf("%d ", custo); break;
            case T_CAMINHO:   cor(14, 0);  printf("%d ", custo); break;
            case T_ROBO:      cor(13, 15); printf("R ");         break;
            case T_INICIO:    cor(15, 0);  printf("S ");         break;
            case T_FIM:       cor(12, 15); printf("E ");         break;
        }
    }
    cor(0, 7);
}

// Decide qual desenho vai em cada casa (robô vence tudo, depois início e fim,
// depois as marcas da busca, por fim o terreno)
int codigoDaCasa(int i, int j, int labirinto[][TAM], int marca[][TAM], int roboL, int roboC)
{
    if(i == roboL && j == roboC) return T_ROBO;
    if(i == LINHA_INICIO && j == COLUNA_INICIO) return T_INICIO;
    if(i == LINHA_FIM && j == COLUNA_FIM) return T_FIM;
    if(marca[i][j] != 0) return marca[i][j];
    return labirinto[i][j];
}

// Redesenha só as casas que mudaram (por isso a tela não pisca)
void desenharMapa(int labirinto[][TAM], int marca[][TAM], int roboL, int roboC)
{
    for(int i = 0; i < tam; i++)
    {
        for(int j = 0; j < tam; j++)
        {
            int codigo = codigoDaCasa(i, j, labirinto, marca, roboL, roboC);
            if(codigo != cache[i][j])
            {
                irPara(X_MAPA + j * larguraTile(), Y_MAPA + i);
                desenharTile(codigo, labirinto[i][j]);
                cache[i][j] = codigo;
            }
        }
    }
}

void itemLegenda(int linha, int codigo, int custo, const char *texto)
{
    irPara(X_PAINEL + 2, Y_LEGENDA + 1 + linha);
    desenharTile(codigo, custo);
    escreverEm(X_PAINEL + 5, Y_LEGENDA + 1 + linha, 7, texto);
}

void desenharLegenda()
{
    desenharCaixa(X_PAINEL, Y_LEGENDA, LARGURA_PAINEL, 12, 11);
    escreverEm(X_PAINEL + 2, Y_LEGENDA, 14, " LEGENDA ");   // título em cima da moldura
    itemLegenda(0, T_PAREDE, 0, "Parede (bloqueada)");
    itemLegenda(1, T_LIMPO, 1, "Chão limpo (custo 1)");
    itemLegenda(2, T_LAMA, 3, "Lama (custo 3)");
    itemLegenda(3, T_AGUA, 5, "Água (custo 5)");
    itemLegenda(4, T_INICIO, 0, "Início");
    itemLegenda(5, T_FIM, 0, "Chegada");
    itemLegenda(6, T_ROBO, 0, "Robô");
    itemLegenda(7, T_EXPLORADA, 1, "Explorada (lista fechada)");
    itemLegenda(8, T_ABERTA, 1, "Descoberta (lista aberta)");
    itemLegenda(9, T_CAMINHO, 1, "Caminho");
}

void desenharTitulo(const char *subtitulo)
{
    desenharCaixa(2, 0, 76, 4, 11);
    escreverEm(4, 1, 14, "RESOLUÇÃO AUTÔNOMA DE JOGOS COM ESTRUTURAS DE DADOS E IA CLÁSSICA");
    escreverEm(4, 2, 15, subtitulo);
}

void desenharTituloComNivel(const char *subtitulo)
{
    char texto[60];
    desenharTitulo(subtitulo);
    sprintf(texto, "Nível: %s", nomeNivel);
    escreverEm(52, 2, 11, texto);
}

// Desenha uma fileira de paredes em volta do labirinto.
// É só desenho: a matriz não muda e quem impede de sair do mapa
// são os testes de limite (linha > 0, linha + 1 < tam...) do código.
void desenharBorda()
{
    for(int j = -1; j <= tam; j++)
    {
        irPara(X_MAPA + j * larguraTile(), Y_MAPA - 1);     // parede de cima
        desenharTile(T_PAREDE, 0);
        irPara(X_MAPA + j * larguraTile(), Y_MAPA + tam);   // parede de baixo
        desenharTile(T_PAREDE, 0);
    }
    for(int i = 0; i < tam; i++)
    {
        irPara(X_MAPA - larguraTile(), Y_MAPA + i);         // parede da esquerda
        desenharTile(T_PAREDE, 0);
        irPara(X_MAPA + tam * larguraTile(), Y_MAPA + i);   // parede da direita
        desenharTile(T_PAREDE, 0);
    }
}

// Limpa tudo e desenha o que é igual nas duas telas de jogo
void prepararTela(const char *subtitulo)
{
    limparTela();
    desenharTituloComNivel(subtitulo);
    desenharBorda();
    desenharLegenda();
    desenharCaixa(X_PAINEL, Y_RESULTADO, LARGURA_PAINEL, 7, 11);
}

// Mostra "rótulo: valor" em uma das 3 linhas da caixa de resultado
void mostrarInfo(int linha, const char *rotulo, int valor)
{
    char texto[80];
    sprintf(texto, "%-22s %-6d", rotulo, valor);
    escreverEm(X_PAINEL + 2, Y_RESULTADO + 1 + linha, 15, texto);
}

// Mensagem na última linha da caixa de resultado (apaga a anterior completando com espaços)
void mostrarMensagem(const char *texto, int corTexto)
{
    char linha[100];
    sprintf(linha, "%-40s", texto);
    escreverEm(X_PAINEL + 2, Y_RESULTADO + 5, corTexto, linha);
}

void rodape(const char *texto)
{
    escreverEm(3, Y_RODAPE, 8, texto);
}

void esperarTecla()
{
    rodape("Pressione qualquer tecla para voltar ao menu");
    lerTecla();
}

// Segunda tela: mostra uma lista de coordenadas (POR_LINHA por linha)
void mostrarListaCompleta(const char *subtitulo, const char *titulo, int linhas[], int colunas[], int quantidade)
{
    char texto[40];
    int y = 5;

    limparTela();
    desenharTituloComNivel(subtitulo);
    escreverEm(3, 4, 14, titulo);

    for(int i = 0; i < quantidade; i++)
    {
        int linhaDaLista = i / POR_LINHA;
        if(linhaDaLista < MAX_LINHAS_LISTA)
        {
            sprintf(texto, "(%d,%d)", linhas[i], colunas[i]);
            y = 5 + linhaDaLista;
            escreverEm(3 + (i % POR_LINHA) * 8, y, 7, texto);
        }
    }
    if(quantidade > MAX_LINHAS_LISTA * POR_LINHA)
    {
        sprintf(texto, "... e mais %d", quantidade - MAX_LINHAS_LISTA * POR_LINHA);
        y = y + 1;
        escreverEm(3, y, 8, texto);
    }
    if(quantidade == 0)
    {
        escreverEm(3, y, 8, "(vazio)");
    }

    escreverEm(3, y + 2, 8, "Pressione qualquer tecla para voltar ao menu");
    lerTecla();
}

// ====================================================================
// JOGO MANUAL
// ====================================================================

// Tenta andar na direção pedida ('W' 'A' 'S' 'D').
// Devolve 1 se andou e 0 se o caminho estava bloqueado.
int mover(int labirinto[][TAM], int *linhaAtual, int *colunaAtual, int *custo_total, Fila *F, int direcao)
{
    int novaLinha = *linhaAtual;
    int novaColuna = *colunaAtual;

    if(direcao == 'W') novaLinha = novaLinha - 1;
    if(direcao == 'S') novaLinha = novaLinha + 1;
    if(direcao == 'A') novaColuna = novaColuna - 1;
    if(direcao == 'D') novaColuna = novaColuna + 1;

    if(novaLinha < 0 || novaLinha >= tam || novaColuna < 0 || novaColuna >= tam)
    {
        return 0;
    }
    if(labirinto[novaLinha][novaColuna] == 0)
    {
        return 0;
    }

    *linhaAtual = novaLinha;
    *colunaAtual = novaColuna;
    *custo_total = *custo_total + labirinto[novaLinha][novaColuna];
    InserFila(F, novaLinha, novaColuna);
    return 1;
}

// ====================================================================
// BUSCA A* (com animação)
// ====================================================================

// Atualiza as cores conforme a lista e desenha um quadro da animação
void animarPasso(NoBusca *lista, int labirinto[][TAM], int marca[][TAM])
{
    int exploradas = 0;
    int abertas = 0;
    NoBusca *aux = lista;

    while(aux != NULL)
    {
        if(aux->fechado == 1)
        {
            marca[aux->linha][aux->coluna] = T_EXPLORADA;
            exploradas++;
        }
        else
        {
            marca[aux->linha][aux->coluna] = T_ABERTA;
            abertas++;
        }
        aux = aux->prox;
    }

    desenharMapa(labirinto, marca, -1, -1);
    mostrarInfo(0, "Casas exploradas:", exploradas);
    mostrarInfo(1, "Na lista aberta:", abertas);
    pausa(VELOCIDADE_BUSCA);
}

// Roda o A* e devolve a lista de nós. O nó de chegada, se existir, está nela.
// animar = 1 desenha cada passo, animar = 0 roda em silêncio.
NoBusca* aEstrela(int labirinto[][TAM], int marca[][TAM], int animar)
{
    NoBusca *lista = NULL;

    // Passo 1: cria o nó do início e coloca na lista
    NoBusca *noInicio;
    noInicio = (NoBusca *)malloc(sizeof(NoBusca));

    noInicio->linha = LINHA_INICIO;
    noInicio->coluna = COLUNA_INICIO;
    noInicio->g = 0;
    noInicio->h = heuristica(LINHA_FIM, COLUNA_FIM, COLUNA_INICIO, LINHA_INICIO);
    noInicio->f = noInicio->g + noInicio->h;
    noInicio->fechado = 0;
    noInicio->anterior = NULL;
    noInicio->prox = NULL;
    lista = noInicio;

    // Passo 2: repete "pega o menor f, fecha, expande os vizinhos"
    NoBusca *atual = menorF(lista);
    int achou = 0;

    while(atual != NULL && achou == 0)
    {
        if(atual->linha == LINHA_FIM && atual->coluna == COLUNA_FIM)
        {
            achou = 1;
        }
        else
        {
            atual->fechado = 1;
            lista = expandirVizinhos(atual, lista, labirinto, LINHA_FIM, COLUNA_FIM);
            atual = menorF(lista);

            if(animar == 1)
            {
                animarPasso(lista, labirinto, marca);
            }
        }
    }

    return lista;
}

// Conta quantos nós foram explorados (fechados) na lista
int contarExploradas(NoBusca *lista)
{
    int total = 0;
    NoBusca *aux = lista;
    while(aux != NULL)
    {
        if(aux->fechado == 1)
        {
            total++;
        }
        aux = aux->prox;
    }
    return total;
}

// ====================================================================
// MEDIÇÃO DE DESEMPENHO (tempo em ms, nós, passos, custo)
// ====================================================================
void carregarLabirinto(int nivel, int labirinto[][TAM]);   // definida lá embaixo, junto dos mapas

#define REPETICOES 1000   // a busca roda 1000 vezes e o tempo é a média

typedef struct
{
    double ms;        // tempo médio de uma busca, em milissegundos
    int expandidos;   // nós fechados (tiveram os vizinhos gerados)
    int gerados;      // nós criados com malloc (tamanho final da lista)
    int passos;       // profundidade da solução (movimentos do início à chegada)
    int custo;        // custo total do caminho (-1 se não existe caminho)
} Medicao;

Medicao medirBusca(int labirinto[][TAM], int comHeuristica)
{
    Medicao m;
    int marcaTemp[TAM][TAM] = {0};

    usarHeuristica = comHeuristica;

    // 1) Tempo: roda várias vezes (criar e liberar a lista faz parte da busca)
    double inicio = agoraMs();
    for(int r = 0; r < REPETICOES; r++)
    {
        NoBusca *lista = aEstrela(labirinto, marcaTemp, 0);
        liberarLista(lista);
    }
    m.ms = (agoraMs() - inicio) / REPETICOES;

    // 2) Contagens: roda mais uma vez e examina a lista
    NoBusca *lista = aEstrela(labirinto, marcaTemp, 0);
    m.expandidos = contarExploradas(lista);
    m.gerados = 0;
    for(NoBusca *aux = lista; aux != NULL; aux = aux->prox)
    {
        m.gerados++;
    }

    NoBusca *destino = buscarNaLista(lista, LINHA_FIM, COLUNA_FIM);
    m.passos = 0;
    m.custo = -1;
    if(destino != NULL)
    {
        m.custo = destino->g;
        for(NoBusca *p = destino; p->anterior != NULL; p = p->anterior)
        {
            m.passos++;
        }
    }
    liberarLista(lista);

    usarHeuristica = 1;
    return m;
}

// Mede o tempo da busca A* e compara com a mesma busca sem heurística (h = 0)
void compararHeuristica(int labirinto[][TAM])
{
    char texto[100];

    mostrarMensagem("Medindo o tempo de execução...", 14);
    Medicao comH = medirBusca(labirinto, 1);
    Medicao semH = medirBusca(labirinto, 0);

    // Tempo de execução na caixa de resultado
    sprintf(texto, "%-22s %.4f", "Tempo da busca (ms):", comH.ms);
    escreverEm(X_PAINEL + 2, Y_RESULTADO + 4, 15, texto);

    desenharCaixa(X_PAINEL, Y_HEURISTICA, LARGURA_PAINEL, 6, 11);
    escreverEm(X_PAINEL + 2, Y_HEURISTICA + 1, 14, "PARA QUE SERVE A HEURÍSTICA?");

    sprintf(texto, "A* (Manhattan):  %3d casas, %.4f ms", comH.expandidos, comH.ms);
    escreverEm(X_PAINEL + 2, Y_HEURISTICA + 2, 10, texto);

    sprintf(texto, "Sem heur. (h=0): %3d casas, %.4f ms", semH.expandidos, semH.ms);
    escreverEm(X_PAINEL + 2, Y_HEURISTICA + 3, 12, texto);

    if(semH.expandidos > comH.expandidos && semH.custo == comH.custo)
    {
        sprintf(texto, "Mesmo custo %d, A* explorou %d%% menos!", comH.custo,
                (semH.expandidos - comH.expandidos) * 100 / semH.expandidos);
    }
    else
    {
        sprintf(texto, "As duas exploraram o mesmo tanto.");
    }
    escreverEm(X_PAINEL + 2, Y_HEURISTICA + 4, 15, texto);
}

void modoBusca(int labirinto[][TAM])
{
    int marca[TAM][TAM] = {0};
    char texto[100];

    prepararTela("Busca A* em andamento");
    desenharMapa(labirinto, marca, -1, -1);
    mostrarInfo(0, "Casas exploradas:", 0);
    mostrarInfo(1, "Na lista aberta:", 1);
    mostrarMensagem("Explorando o labirinto...", 14);
    pausa(700);

    NoBusca *lista = aEstrela(labirinto, marca, 1);
    NoBusca *destino = buscarNaLista(lista, LINHA_FIM, COLUNA_FIM);

    if(destino == NULL)
    {
        mostrarMensagem("Não existe caminho até o destino.", 12);
    }
    else
    {
        // Segue os "anterior" do destino até o início
        int caminhoLinha[TAM * TAM];
        int caminhoColuna[TAM * TAM];
        int tamanho = 0;

        NoBusca *p = destino;
        while(p != NULL)
        {
            caminhoLinha[tamanho] = p->linha;
            caminhoColuna[tamanho] = p->coluna;
            tamanho++;
            p = p->anterior;
        }

        mostrarMensagem("Caminho encontrado! Desenhando...", 10);
        pausa(500);

        // O caminho está guardado do fim para o início, então percorremos de trás para frente
        for(int i = tamanho - 1; i >= 0; i--)
        {
            marca[caminhoLinha[i]][caminhoColuna[i]] = T_CAMINHO;
            desenharMapa(labirinto, marca, -1, -1);
            pausa(VELOCIDADE_CAMINHO);
        }

        mostrarMensagem("O robô vai percorrer o caminho...", 14);
        mostrarInfo(2, "Custo acumulado:", 0);

        int custoAcumulado = 0;
        for(int i = tamanho - 1; i >= 0; i--)
        {
            int l = caminhoLinha[i];
            int c = caminhoColuna[i];

            if(i != tamanho - 1)
            {
                custoAcumulado = custoAcumulado + labirinto[l][c];
            }
            desenharMapa(labirinto, marca, l, c);
            mostrarInfo(2, "Custo acumulado:", custoAcumulado);
            pausa(VELOCIDADE_ROBO);
        }

        compararHeuristica(labirinto);

        sprintf(texto, "Chegou! Custo %d em %d casas", destino->g, tamanho);
        mostrarMensagem(texto, 10);
        som(660, 120);
        som(880, 120);
        som(1175, 250);
        liberarLista(lista);

        rodape("Pressione qualquer tecla para ver o caminho completo");
        lerTecla();

        // O caminho está guardado do fim para o início: invertemos para mostrar na ordem
        int ordemLinha[TAM * TAM];
        int ordemColuna[TAM * TAM];
        for(int i = 0; i < tamanho; i++)
        {
            ordemLinha[i] = caminhoLinha[tamanho - 1 - i];
            ordemColuna[i] = caminhoColuna[tamanho - 1 - i];
        }
        mostrarListaCompleta("Resultado da busca A*", "Caminho encontrado (linha, coluna):", ordemLinha, ordemColuna, tamanho);
        return;
    }

    liberarLista(lista);
    esperarTecla();
}

// ====================================================================
// JOGO MANUAL (tela)
// ====================================================================
void jogoManual(int labirinto[][TAM])
{
    int linhaAtual = LINHA_INICIO;
    int colunaAtual = COLUNA_INICIO;
    int custo_total = 0;
    int passos = 0;
    int tecla = 0;
    int marca[TAM][TAM] = {0};
    char texto[100];
    Fila *F = CriaFila();

    prepararTela("Modo manual");
    escreverEm(X_PAINEL + 2, Y_RESULTADO + 3, 8, "Setas ou WASD andam, Q sai");
    mostrarMensagem("Leve o robô até a chegada!", 14);

    while((linhaAtual != LINHA_FIM || colunaAtual != COLUNA_FIM) && tecla != 'Q')
    {
        desenharMapa(labirinto, marca, linhaAtual, colunaAtual);
        mostrarInfo(0, "Custo acumulado:", custo_total);
        mostrarInfo(1, "Passos:", passos);

        tecla = lerTecla();

        if(tecla == 'W' || tecla == 'A' || tecla == 'S' || tecla == 'D')
        {
            if(mover(labirinto, &linhaAtual, &colunaAtual, &custo_total, F, tecla) == 1)
            {
                passos++;
                marca[linhaAtual][colunaAtual] = T_CAMINHO;
                mostrarMensagem("", 7);
            }
            else
            {
                mostrarMensagem("Caminho bloqueado!", 12);
                som(200, 90);
            }
        }
    }

    desenharMapa(labirinto, marca, linhaAtual, colunaAtual);
    mostrarInfo(0, "Custo acumulado:", custo_total);
    mostrarInfo(1, "Passos:", passos);

    if(tecla == 'Q')
    {
        mostrarMensagem("Jogo encerrado.", 14);
    }
    else
    {
        // Roda o A* em silêncio só para comparar com o seu resultado
        int marcaTemp[TAM][TAM] = {0};
        NoBusca *lista = aEstrela(labirinto, marcaTemp, 0);
        NoBusca *destino = buscarNaLista(lista, LINHA_FIM, COLUNA_FIM);
        int custoOtimo = destino->g;
        liberarLista(lista);

        if(custo_total == custoOtimo)
        {
            sprintf(texto, "Parabéns! Custo %d = ótimo do A*!", custo_total);
        }
        else
        {
            sprintf(texto, "Chegou! Seu custo %d, A* faz %d", custo_total, custoOtimo);
        }
        mostrarMensagem(texto, 10);
        som(660, 120);
        som(880, 120);
        som(1175, 250);
    }

    rodape("Pressione qualquer tecla para ver o histórico (fila)");
    lerTecla();

    // Histórico de movimentos: esvazia a fila, do primeiro ao último movimento
    int histLinha[MAX_HISTORICO];
    int histColuna[MAX_HISTORICO];
    int quantidade = 0;
    while(VaziaFila(F) == 0)
    {
        int l;
        int c;
        RemoveFila(F, &l, &c);
        if(quantidade < MAX_HISTORICO)
        {
            histLinha[quantidade] = l;
            histColuna[quantidade] = c;
            quantidade++;
        }
    }
    F = LiberaFila(F);

    mostrarListaCompleta("Resultado do jogo manual", "Histórico de movimentos (fila):", histLinha, histColuna, quantidade);
}

// ====================================================================
// TABELA COMPARATIVA (métricas para o relatório)
// ====================================================================
void tabelaComparativa(int labirinto[][TAM])
{
    char texto[120];
    char niveis[3] = {'1', '2', '3'};
    int y = 6;

    limparTela();
    desenharTitulo("Tabela comparativa");
    escreverEm(3, 4, 14, "Medindo... (cada busca roda 1000 vezes)");

    escreverEm(3, y, 11, "Cenário           Busca           Tempo (ms)  Expandidos  Gerados  Passos  Custo  Memória");
    escreverEm(3, y + 1, 8, "────────────────────────────────────────────────────────────────────────────────────────────");
    y = y + 2;

    for(int n = 0; n < 3; n++)
    {
        carregarLabirinto(niveis[n], labirinto);
        for(int comH = 1; comH >= 0; comH--)
        {
            Medicao m = medirBusca(labirinto, comH);
            // o nome do nível tem acento, então vai separado para não desalinhar as colunas
            if(comH == 1)
            {
                escreverEm(3, y, 10, nomeNivel);
            }
            sprintf(texto, "%-15s %10.4f  %10d  %7d  %6d  %5d  %5d B",
                    comH == 1 ? "A* (Manhattan)" : "Sem heur. (h=0)",
                    m.ms, m.expandidos, m.gerados, m.passos, m.custo,
                    m.gerados * (int)sizeof(NoBusca));
            escreverEm(21, y, comH == 1 ? 10 : 7, texto);
            y++;
        }
        y++;
    }

    escreverEm(3, 4, 14, "Resultados (média de 1000 execuções de cada busca)      ");
    sprintf(texto, "Memória = nós gerados x sizeof(NoBusca) = nós x %d bytes", (int)sizeof(NoBusca));
    escreverEm(3, y, 8, texto);
    escreverEm(3, y + 2, 8, "Pressione qualquer tecla para voltar ao menu");
    lerTecla();
}

// ====================================================================
// MENU
// ====================================================================
int menuPrincipal()
{
    limparTela();
    desenharTitulo("Labirinto com custo dinâmico");
    desenharCaixa(6, 5, 68, 13, 11);

    escreverEm(10, 7, 14, "ESCOLHA UM MODO");
    escreverEm(10, 9, 15, "1   Jogar manualmente (setas ou WASD)");
    escreverEm(10, 10, 15, "2   Rodar a busca A* (acha o caminho mais barato sozinha)");
    if(usarEmoji)
    {
        escreverEm(10, 11, 15, "3   Visual: EMOJI (aperte 3 para trocar para blocos)");
    }
    else
    {
        escreverEm(10, 11, 15, "3   Visual: BLOCOS COLORIDOS (aperte 3 para trocar para emoji)");
    }
    escreverEm(10, 12, 15, "4   Tabela comparativa (tempo, nós, passos e custo)");
    escreverEm(10, 13, 15, "0   Sair");

    // Amostra dos terrenos
    irPara(10, 15);
    desenharTile(T_PAREDE, 0);
    printf(" ");
    desenharTile(T_LIMPO, 1);
    printf(" ");
    desenharTile(T_LAMA, 3);
    printf(" ");
    desenharTile(T_AGUA, 5);
    printf(" ");
    desenharTile(T_ROBO, 0);
    printf(" ");
    desenharTile(T_FIM, 0);

    return lerTecla();
}

// ====================================================================
// OS TRÊS LABIRINTOS
// Matriz: 0 = parede, 1 = caminho limpo, 3 = lama, 5 = água
// ====================================================================

// FÁCIL (8x8): campo aberto, com um lago no meio para contornar
int mapaFacil[8][8] = {
    {1, 1, 1, 1, 3, 1, 1, 1},
    {1, 0, 0, 1, 5, 5, 0, 1},
    {1, 0, 1, 1, 5, 5, 0, 1},
    {1, 1, 1, 0, 3, 1, 1, 1},
    {0, 0, 1, 0, 1, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 3, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1}
};

// MÉDIO (12x12): corredor em zigue-zague
int mapaMedio[12][12] = {
    {1, 5, 3, 5, 5, 3, 5, 5, 3, 5, 5, 5},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 5},
    {0, 0, 0, 0, 0, 0, 0, 0, 5, 1, 0, 5},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 3},
    {1, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 5},
    {0, 0, 0, 0, 0, 0, 0, 0, 5, 1, 0, 3},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 5},
    {1, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 3},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1}
};

// DIFÍCIL (17x17): labirinto com bifurcações; o caminho mais curto passa
// pela água, então o mais barato é mais longo
int mapaDificil[17][17] = {
    {1, 5, 1, 0, 1, 1, 3, 5, 5, 5, 5, 5, 3, 0, 1, 1, 1},
    {1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 5, 0, 5, 0, 1},
    {1, 0, 1, 1, 1, 3, 5, 5, 5, 5, 1, 0, 3, 1, 1, 1, 1},
    {1, 0, 0, 0, 1, 0, 5, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1},
    {1, 3, 1, 0, 1, 5, 1, 5, 1, 0, 1, 1, 1, 1, 1, 0, 5},
    {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 5, 0, 0, 0, 3, 0, 1},
    {3, 5, 5, 1, 1, 0, 3, 1, 1, 0, 1, 0, 1, 0, 3, 1, 5},
    {0, 0, 3, 0, 1, 0, 5, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1},
    {1, 1, 1, 1, 1, 0, 1, 1, 5, 1, 1, 0, 3, 0, 1, 1, 1},
    {5, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0},
    {1, 0, 1, 0, 1, 0, 1, 3, 3, 3, 1, 1, 1, 1, 1, 3, 1},
    {3, 0, 1, 0, 5, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1},
    {1, 0, 3, 0, 3, 1, 1, 0, 1, 1, 1, 0, 1, 0, 3, 5, 1},
    {1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0},
    {1, 0, 1, 1, 1, 5, 1, 3, 5, 1, 3, 0, 3, 1, 5, 5, 5},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
    {1, 1, 1, 5, 1, 1, 1, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

// Copia o labirinto do nível escolhido para a matriz "labirinto" e ajusta tam
void carregarLabirinto(int nivel, int labirinto[][TAM])
{
    if(nivel == '1')
    {
        tam = 8;
        nomeNivel = "Fácil (8x8)";
        for(int i = 0; i < tam; i++)
            for(int j = 0; j < tam; j++)
                labirinto[i][j] = mapaFacil[i][j];
    }
    else if(nivel == '2')
    {
        tam = 12;
        nomeNivel = "Médio (12x12)";
        for(int i = 0; i < tam; i++)
            for(int j = 0; j < tam; j++)
                labirinto[i][j] = mapaMedio[i][j];
    }
    else
    {
        tam = 17;
        nomeNivel = "Difícil (17x17)";
        for(int i = 0; i < tam; i++)
            for(int j = 0; j < tam; j++)
                labirinto[i][j] = mapaDificil[i][j];
    }
}

// Pergunta a dificuldade. Devolve '1', '2', '3' ou '0' (voltar).
int menuDificuldade()
{
    int tecla = 0;

    limparTela();
    desenharTitulo("Escolha a dificuldade");
    desenharCaixa(6, 5, 68, 11, 11);

    escreverEm(10, 7, 14, "DIFICULDADE");
    escreverEm(10, 9, 10, "1   Fácil     8x8    campo aberto com um lago");
    escreverEm(10, 10, 14, "2   Médio     12x12  corredor em zigue-zague");
    escreverEm(10, 11, 12, "3   Difícil   17x17  labirinto com bifurcações");
    escreverEm(10, 13, 15, "0   Voltar");

    while(tecla != '0' && tecla != '1' && tecla != '2' && tecla != '3')
    {
        tecla = lerTecla();
        if(tecla == 'Q')
        {
            tecla = '0';
        }
    }
    return tecla;
}

int main()
{
    int labirinto[TAM][TAM] = {0};
    int opcao = 0;
    int nivel;

    configurarConsole();

    while(opcao != '0')
    {
        opcao = menuPrincipal();

        if(opcao == '1' || opcao == '2')
        {
            nivel = menuDificuldade();
            if(nivel != '0')
            {
                carregarLabirinto(nivel, labirinto);
                if(opcao == '1')
                {
                    jogoManual(labirinto);
                }
                else
                {
                    modoBusca(labirinto);
                }
            }
        }
        else if(opcao == '4')
        {
            tabelaComparativa(labirinto);
        }
        else if(opcao == '3')
        {
            usarEmoji = !usarEmoji;
        }
        else if(opcao == 'Q')
        {
            opcao = '0';
        }
    }

    limparTela();
    mostrarCursor(1);
    irPara(0, 0);
    printf("Até a próxima!\n");
    return 0;
}
