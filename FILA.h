#ifndef FILA_H_INCLUDED
#define FILA_H_INCLUDED
#include <stdio.h>
#include <stdlib.h>
/* FUNCOES DE MANIPULACAO DE FILA (historico de posicoes do jogador)

Fila* CriaFila()                                   CRIA A FILA

void InserFila(Fila *F, int l, int c)              INSERE A POSICAO (l, c) NO FIM

void RemoveFila(Fila *F, int *linha, int *coluna)  REMOVE DO INICIO, DEVOLVENDO LINHA E COLUNA

Fila* LiberaFila(Fila *F)                          LIBERA A FILA

void ImprimeFila(Fila *F)                          IMPRIME A FILA

int VaziaFila(Fila *F)                             RETORNA 1 SE A FILA ESTA VAZIA

*/

typedef struct no
{
  int linha;
  int coluna;
  struct no *prox;
} No;

typedef struct fila {
  No *Ini,*Fim;
} Fila;

Fila* CriaFila()
{
  Fila *aux;
  aux = (Fila *)malloc(sizeof(Fila));
  if (aux == NULL)
  {
    printf("Erro de memoria");
    exit(0);
  }
  aux->Ini = NULL;
  aux->Fim = NULL;
  return aux;
}

int VaziaFila(Fila *F)
{
    if(F->Ini==NULL)
    {
        return 1;
    }
    return 0;
}

void RemoveFila(Fila *F, int *linha, int *coluna)
{
  No *aux;
  aux = F->Ini;
  if (aux == NULL)
  {
    printf("Erro Fila vazia");
    exit(0);
  }
  else
  {
    *linha = aux->linha;
    *coluna = aux->coluna;
    F->Ini = aux->prox;
    if(F->Ini == NULL)
    {
        F->Fim = NULL;
    }
    free(aux);
  }
}

void InserFila(Fila *F, int l, int c)
{
    No *novo = (No*)malloc(sizeof(No));
    if (novo != NULL)
    {
        novo -> linha = l;
        novo -> coluna = c;
        novo-> prox=NULL;
        if(F->Fim!=NULL)
       {
           F->Fim->prox=novo;
       }
       else
       {
           F->Ini=novo;
       }
        F->Fim=novo;
    }
    else
    {
        printf("Erro de memoria");
        exit(0);
    }
}

void ImprimeFila(Fila *F)
{
    if(!VaziaFila(F))
    {
        No *aux=F->Ini;
        printf("\n\n\tFila:");
        for(;aux!=NULL;aux=aux->prox)
        {
            printf(" (%d, %d)", aux->linha, aux->coluna);
        }
        printf("\n");
    }
    else
    {
         printf("\n\n\tFila VAZIA\n");
    }
}


Fila* LiberaFila(Fila *F)
{
    No *apag, *aux;
    aux =F -> Ini;
    while (aux != NULL)
    {
        apag = aux;
        aux = aux -> prox;
        free(apag);
    }
    free(F);
    return NULL;
}
#endif // FILA_H_INCLUDED
