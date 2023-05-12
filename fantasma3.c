/************************************************************************/
/*				mp_car.c				*/
/*									*/
/*	Compilar:							*/
/*		$ gcc  -Wall mp_car.c memoria.o -o mp_car1		*/
/*									*/
/*	Escriu per la sortida estandard un caracter identificatiu	*/
/*      segons l'argument 'n_car' (1 -> 'a', 2 -> 'b', ...), tantes	*/
/*	vegades com indica el segon argument 'n_vegades', esperant	*/
/*	un temps aleatori entre dues visualitzacions; el programa	*/
/*	acaba quan s'han escrit tantes lletres com indiqui el		*/
/*	contingut de la zona de memoria compartida identificada pel	*/
/*	tercer argument 'id_lletres'.					*/
/*									*/
/*	Aquest programa s'executara com un proces fill del programa	*/
/*	'multiproc' (veure fitxer 'multiproc.c').			*/
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "memoria.h"
#include "winsuport2.h"

typedef struct {		/* per un objecte (menjacocos o fantasma) */
	int f;				/* posicio actual: fila */
	int c;				/* posicio actual: columna */
	int d;				/* direccio actual: [0..3] */
  float r;            /* per indicar un retard relati */
	char a;				/* caracter anterior en pos. actual */
} objecte;

char c_req;			    /* caracter de pared del laberint */

int df[] = {-1, 0, 1, 0};	/* moviments de les 4 direccions possibles */
int dc[] = {0, -1, 0, 1};	/* dalt, esquerra, baix, dreta */

int main(int n_args, char *ll_args[])
{
  objecte fantasma;
  int i,num_fantasma,retard, id_win,n_fil,n_col,fi1,fi2;
  int *p_lletres, id_lletres;
  void *p_win;

  if (n_args < 5)
  {   fprintf(stderr,"\tproces (%d): fanstasma3 num_fantasma retard id_win n_fil n_col fil_fantasma col_fantasma direccio_fantasma char_anterior retard_fantasma\n",(int) getpid());
	exit(0);
  }

  num_fantasma = atoi(ll_args[1]);
  retard = atoi(ll_args[2]);

  /*id_lletres = atoi(ll_args[3]);
  p_lletres = map_mem(id_lletres);	/* obtenir adres. de mem. compartida */
  /*if (p_lletres == (int*) -1)
  {   fprintf(stderr,"proces (%d): error en identificador de memoria\n",(int)getpid());
	exit(0);
  }*/

  id_win = atoi(ll_args[3]);
  p_win = map_mem(id_win);
  if (p_win == (int*) -1)
  {   fprintf(stderr,"proces (%d): error en identificador de finestra\n",(int)getpid());
	exit(0);
  }
  n_fil = atoi(ll_args[4]);		/* obtenir dimensions del camp de joc */
  n_col = atoi(ll_args[5]);
  fantasma.f = atoi(ll_args[6]);
  fantasma.c = atoi(ll_args[7]);
  fantasma.d = atoi(ll_args[8]);
  fantasma.a = atoi(ll_args[9]);
  fantasma.r = atoi(ll_args[10]);
  fi1 = atoi(ll_args[11]);
  fi2 = atoi(ll_args[12]);
  win_set(p_win,n_fil,n_col);	/* crea acces a finestra oberta pel proces pare */


  /*############INICIO FUNCION MOU_FANTASMA############*/
  i = num_fantasma;
  objecte seg;
  //int ret;
  int k, vk, nd, vd[3];
  
  //pthread_mutex_lock(&mutex);
  fantasma.a = win_quincar(fantasma.f,fantasma.c);
  //pthread_mutex_unlock(&mutex);
  if (fantasma.a == c_req) {
    fprintf(stderr,"  posicio inicial del fantasma damunt la pared del laberint\n"); /* error: fantasma sobre pared */
    exit(7);
  }
  //pthread_mutex_lock(&mutex);
  win_escricar(fantasma.f,fantasma.c,i+'0',NO_INV);
  //pthread_mutex_unlock(&mutex);
  do{
  //ret = 0; 
    nd = 0;
    for (k=-1; k<=1; k++)		/* provar direccio actual i dir. veines */
    {
      vk = (fantasma.d + k) % 4;		/* direccio veina */
      if (vk < 0) vk += 4;		/* corregeix negatius */
      seg.f = fantasma.f + df[vk]; /* calcular posicio en la nova dir.*/
      seg.c = fantasma.c + dc[vk];
      //pthread_mutex_lock(&mutex);
      seg.a = win_quincar(seg.f,seg.c);	/* calcular caracter seguent posicio */
      //pthread_mutex_unlock(&mutex);
      if ((seg.a==' ') || (seg.a=='.') || (seg.a=='0'))
      { 
        vd[nd] = vk;			/* memoritza com a direccio possible */
        nd++;
      }
    }
    if (nd == 0)				/* si no pot continuar, */
    {
      fantasma.d = (fantasma.d + 2) % 4;		/* canvia totalment de sentit */
    }
    else
    { 
      if (nd == 1)			/* si nomes pot en una direccio */
      {
        fantasma.d = vd[0];			/* li assigna aquesta */
      }
      else				/* altrament */
      {
        fantasma.d = vd[rand() % nd];		/* segueix una dir. aleatoria */
      }

      seg.f = fantasma.f + df[fantasma.d];  /* calcular seguent posicio final */
      seg.c = fantasma.c + dc[fantasma.d];
      //pthread_mutex_lock(&mutex);
      seg.a = win_quincar(seg.f,seg.c);	/* calcular caracter seguent posicio */
      if ((seg.a==' ') || (seg.a=='.') || (seg.a=='0'))
      {
        win_escricar(fantasma.f,fantasma.c,fantasma.a,NO_INV);	/* esborra posicio anterior */
        fantasma.f = seg.f; fantasma.c = seg.c; fantasma.a = seg.a;	/* actualitza posicio */
        win_escricar(fantasma.f,fantasma.c,i+1+'0',NO_INV);		/* redibuixa fantasma */
        //pthread_mutex_unlock(&mutex);
        //pthread_mutex_lock(&mutex);
        if (fantasma.a == '0') 
        {
          fi2 = 1;		/* ha capturat menjacocos */
        }
        //pthread_mutex_unlock(&mutex);
      }else{
        //pthread_mutex_unlock(&mutex);
      }
    }
    win_retard(retard*fantasma.r);
  } while (!fi1 && !fi2);
  /*############FINAL FUNCION MOU_FANTASMA############*/
  return(i);	/* retorna el numero de lletres que ha impres el proces */
}
