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


int main(int n_args, char *ll_args[])
{
  int i,num_fantasma,retard, id_win,n_fil,n_col;
  int *p_lletres, id_lletres;
  void *p_win;

  if (n_args < 5)
  {   fprintf(stderr,"\tproces (%d): fanstasma3 num_fantasma retard id_win n_fil n_col\n",(int) getpid());
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

  win_set(p_win,n_fil,n_col);	/* crea acces a finestra oberta pel proces pare */



  int i =  num_fantasma;
  objecte seg;
  //int ret;
  int k, vk, nd, vd[3];
  
  //pthread_mutex_lock(&mutex);
  fantasmes[i].a = win_quincar(fantasmes[i].f,fantasmes[i].c);
  //pthread_mutex_unlock(&mutex);
  if (fantasmes[i].a == c_req) {
    fprintf(stderr,"  posicio inicial del fantasma damunt la pared del laberint\n"); /* error: fantasma sobre pared */
    exit(7);
  }
  //pthread_mutex_lock(&mutex);
  win_escricar(fantasmes[i].f,fantasmes[i].c,i+'0',NO_INV);
  //pthread_mutex_unlock(&mutex);
  do{
  //ret = 0; 
    nd = 0;
    for (k=-1; k<=1; k++)		/* provar direccio actual i dir. veines */
    {
      vk = (fantasmes[i].d + k) % 4;		/* direccio veina */
      if (vk < 0) vk += 4;		/* corregeix negatius */
      seg.f = fantasmes[i].f + df[vk]; /* calcular posicio en la nova dir.*/
      seg.c = fantasmes[i].c + dc[vk];
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
      fantasmes[i].d = (fantasmes[i].d + 2) % 4;		/* canvia totalment de sentit */
    }
    else
    { 
      if (nd == 1)			/* si nomes pot en una direccio */
      {
        fantasmes[i].d = vd[0];			/* li assigna aquesta */
      }
      else				/* altrament */
      {
        fantasmes[i].d = vd[rand() % nd];		/* segueix una dir. aleatoria */
      }

      seg.f = fantasmes[i].f + df[fantasmes[i].d];  /* calcular seguent posicio final */
      seg.c = fantasmes[i].c + dc[fantasmes[i].d];
      //pthread_mutex_lock(&mutex);
      seg.a = win_quincar(seg.f,seg.c);	/* calcular caracter seguent posicio */
      if ((seg.a==' ') || (seg.a=='.') || (seg.a=='0'))
      {
        win_escricar(fantasmes[i].f,fantasmes[i].c,fantasmes[i].a,NO_INV);	/* esborra posicio anterior */
        fantasmes[i].f = seg.f; fantasmes[i].c = seg.c; fantasmes[i].a = seg.a;	/* actualitza posicio */
        win_escricar(fantasmes[i].f,fantasmes[i].c,i+1+'0',NO_INV);		/* redibuixa fantasma */
        //pthread_mutex_unlock(&mutex);
        //pthread_mutex_lock(&mutex);
        if (fantasmes[i].a == '0') 
        {
          fi2 = 1;		/* ha capturat menjacocos */
        }
        //pthread_mutex_unlock(&mutex);
      }else{
        //pthread_mutex_unlock(&mutex);
      }
    }
    win_retard(retard*fantasmes[i].r);
  } while (!fi1 && !fi2);
  return(i);	/* retorna el numero de lletres que ha impres el proces */
}
