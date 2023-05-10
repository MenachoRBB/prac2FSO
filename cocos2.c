/*****************************************************************************/
/*									                                         */
/*				     cocos1.c				                                 */
/*									                                         */
/*  Arguments del programa:						                             */
/*     per controlar la posicio de tots els elements del joc, cal indicar    */
/*     el nom d'un fitxer de text que contindra la seguent informacio:	     */
/*		n_fil1 n_col fit_tauler creq				                         */
/*		mc_f mc_c mc_d mc_r						                             */
/*		f1_f f1_c f1_d f1_r						                             */
/*									                                         */
/*     on 'n_fil1', 'n_col' son les dimensions del taulell de joc, mes una   */
/*     fila pels missatges de text a l'ultima linia. "fit_tauler" es el nom  */
/*     d'un fitxer de text que contindra el dibuix del laberint, amb num. de */
/*     files igual a 'n_fil1'-1 i num. de columnes igual a 'n_col'. Dins     */
/*     d'aquest fitxer, hi hauran caracter ASCCII que es representaran en    */
/*     pantalla tal qual, excepte el caracters iguals a 'creq', que es visua-*/
/*     litzaran invertits per representar la paret.			                 */
/*     Els parametres 'mc_f', 'mc_c' indiquen la posicio inicial de fila i   */
/*     columna del menjacocos, aixi com la direccio inicial de moviment      */
/*     (0 -> amunt, 1-> esquerra, 2-> avall, 3-> dreta). Els parametres	     */
/*     'f1_f', 'f1_c' i 'f1_d' corresponen a la mateixa informacio per al    */
/*     fantasma 1. El programa verifica que la primera posicio del menja-    */
/*     cocos o del fantasma no coincideixi amb un bloc de paret del laberint.*/
/*	   'mc_r' 'f1_r' son dos reals que multipliquen el retard del moviment.  */ 
/*     A mes, es podra afegir un segon argument opcional per indicar el      */
/*     retard de moviment del menjacocos i dels fantasmes (en ms);           */
/*     el valor per defecte d'aquest parametre es 100 (1 decima de segon).   */
/*									                                         */
/*  Compilar i executar:					  	                             */
/*     El programa invoca les funcions definides a 'winsuport.h', les        */
/*     quals proporcionen una interficie senzilla per crear una finestra     */
/*     de text on es poden escriure caracters en posicions especifiques de   */
/*     la pantalla (basada en CURSES); per tant, el programa necessita ser   */
/*     compilat amb la llibreria 'curses':				                     */
/*									                                         */
/*	   $ gcc -Wall cocos0.c winsuport.o -o cocos0 -lcurses		             */
/*	   $ ./cocos0 fit_param [retard]				                         */
/*									                                         */
/*  Codis de retorn:						  	                             */
/*     El programa retorna algun dels seguents codis al SO:		             */
/*	0  ==>  funcionament normal					                             */
/*	1  ==>  numero d'arguments incorrecte 				                     */
/*	2  ==>  fitxer de configuracio no accessible			                 */
/*	3  ==>  dimensions del taulell incorrectes			                     */
/*	4  ==>  parametres del menjacocos incorrectes			                 */
/*	5  ==>  parametres d'algun fantasma incorrectes			                 */
/*	6  ==>  no s'ha pogut crear el camp de joc			                     */
/*	7  ==>  no s'ha pogut inicialitzar el joc			                     */
/*****************************************************************************/


#include <stdint.h>		/* intptr_t for 64bits machines */
#include <stdio.h>		/* incloure definicions de funcions estandard */
#include <stdlib.h>		/* per exit() */
#include <unistd.h>		/* per getpid() */
#include <pthread.h>
#include "winsuport.h"		/* incloure definicions de funcions propies */



#define MIN_FIL 7		/* definir limits de variables globals */
#define MAX_FIL 25
#define MIN_COL 10
#define MAX_COL 80
#define MAX_THREADS 10
#define MAX_GHOSTS 9
#define LONGMISS	20

				/* definir estructures d'informacio */
typedef struct {		/* per un objecte (menjacocos o fantasma) */
	int f;				/* posicio actual: fila */
	int c;				/* posicio actual: columna */
	int d;				/* direccio actual: [0..3] */
    float r;            /* per indicar un retard relati */
	char a;				/* caracter anterior en pos. actual */
} objecte;


/* variables globals */
pthread_t tid[MAX_THREADS]; /* taula d'identificadors dels threads */
int fi1, fi2;         /* finalització del joc*/
int n_fil1, n_col;		/* dimensions del camp de joc */
char tauler[70];		/* nom del fitxer amb el laberint de joc */
char c_req;			    /* caracter de pared del laberint */

objecte mc;      		/* informacio del menjacocos */
objecte fantasmes[MAX_GHOSTS];			    /* informacio dels fantasmes */
int total_fantasmes;

int df[] = {-1, 0, 1, 0};	/* moviments de les 4 direccions possibles */
int dc[] = {0, -1, 0, 1};	/* dalt, esquerra, baix, dreta */

int cocos;			/* numero restant de cocos per menjar */
int retard;		    /* valor del retard de moviment, en mil.lisegons */
char strin[LONGMISS];			/* variable per a generar missatges de text */

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* crea un sem. Global*/




/* funcio per realitzar la carrega dels parametres de joc emmagatzemats */
/* dins d'un fitxer de text, el nom del qual es passa per referencia a  */
/* 'nom_fit'; si es detecta algun problema, la funcio avorta l'execucio */
/* enviant un missatge per la sortida d'error i retornant el codi per-	*/
/* tinent al SO (segons comentaris al principi del programa).		    */
void carrega_parametres(const char *nom_fit)
{
  total_fantasmes = 0;
  FILE *fit;
  fit = fopen(nom_fit,"rt");		/* intenta obrir fitxer */
  if (fit == NULL)
  {	
    fprintf(stderr,"No s'ha pogut obrir el fitxer \'%s\'\n",nom_fit);
  	exit(2);
  }

  if (!feof(fit)) fscanf(fit,"%d %d %s %c\n",&n_fil1,&n_col,tauler,&c_req);
  else {
    fprintf(stderr,"Falten parametres al fitxer \'%s\'\n",nom_fit);
    fclose(fit);
    exit(2);
	}
  if ((n_fil1 < MIN_FIL) || (n_fil1 > MAX_FIL) ||
	(n_col < MIN_COL) || (n_col > MAX_COL))
  {
    fprintf(stderr,"Error: dimensions del camp de joc incorrectes:\n");
    fprintf(stderr,"\t%d =< n_fil1 (%d) =< %d\n",MIN_FIL,n_fil1,MAX_FIL);
    fprintf(stderr,"\t%d =< n_col (%d) =< %d\n",MIN_COL,n_col,MAX_COL);
    fclose(fit);
    exit(3);
  }

  if (!feof(fit)) fscanf(fit,"%d %d %d %f\n",&mc.f,&mc.c,&mc.d,&mc.r);
  else {
    fprintf(stderr,"Falten parametres al fitxer \'%s\'\n",nom_fit);
    fclose(fit);
    exit(2);
	}
  if ((mc.f < 1) || (mc.f > n_fil1-3) ||
	(mc.c < 1) || (mc.c > n_col-2) ||
	(mc.d < 0) || (mc.d > 3))
  {
    fprintf(stderr,"Error: parametres menjacocos incorrectes:\n");
    fprintf(stderr,"\t1 =< mc.f (%d) =< n_fil1-3 (%d)\n",mc.f,(n_fil1-3));
    fprintf(stderr,"\t1 =< mc.c (%d) =< n_col-2 (%d)\n",mc.c,(n_col-2));
    fprintf(stderr,"\t0 =< mc.d (%d) =< 3\n",mc.d);
    fclose(fit);
    exit(4);
  }

  
  if (!feof(fit) && total_fantasmes < MAX_GHOSTS) 
  {
    while(!feof(fit) && total_fantasmes < MAX_GHOSTS){
      fscanf(fit,"%d %d %d %f\n",&fantasmes[total_fantasmes].f,&fantasmes[total_fantasmes].c,&fantasmes[total_fantasmes].d,&fantasmes[total_fantasmes].r);
      total_fantasmes++;
    }
  }
  else 
  {
    fprintf(stderr,"Falten parametres al fitxer \'%s\'\n",nom_fit);
    fclose(fit);
    exit(2);
	}
  for(int i = 0; i<total_fantasmes; i++){
    if ((fantasmes[i].f < 1) || (fantasmes[i].f > n_fil1-3) ||
    (fantasmes[i].c < 1) || (fantasmes[i].c > n_col-2) ||
    (fantasmes[i].d < 0) || (fantasmes[i].d > 3))
    {
      fprintf(stderr,"Error: parametres fantasma %d incorrectes:\n",i+1);
      fprintf(stderr,"\t1 =< fantasma[%d].f (%d) =< n_fil1-3 (%d)\n",i,fantasmes[i].f,(n_fil1-3));
      fprintf(stderr,"\t1 =< fantasma[%d].c (%d) =< n_col-2 (%d)\n",i,fantasmes[i].c,(n_col-2));
      fprintf(stderr,"\t0 =< fantasma[%d].d (%d) =< 3\n",i,fantasmes[i].d);
      fclose(fit);
      exit(5);
    }
  }
  fclose(fit);			/* fitxer carregat: tot OK! */
  printf("Joc del MenjaCocos\n\tTecles: \'%c\', \'%c\', \'%c\', \'%c\', RETURN-> sortir\n",
		TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
  printf("prem una tecla per continuar:\n");
  getchar();
}




/* funcio per inicialitar les variables i visualitzar l'estat inicial del joc */
void inicialitza_joc(void)
{
  int r,i,j;
  //char strin[12];

  r = win_carregatauler(tauler,n_fil1-1,n_col,c_req);
  if (r == 0)
  {
    mc.a = win_quincar(mc.f,mc.c);
    if (mc.a == c_req) r = -6;		/* error: menjacocos sobre pared */
    else
    {
      //for(int n = 0; n<total_fantasmes;n++){
        
        cocos = 0;			/* compta el numero total de cocos */
        for (i=0; i<n_fil1-1; i++)
          for (j=0; j<n_col; j++)
            if (win_quincar(i,j)=='.') cocos++;
            

        
      //}
      win_escricar(mc.f,mc.c,'0',NO_INV);
      //if (mc.a == '.') cocos--;	/* menja primer coco */

	  //sprintf(strin,"Cocos: %d", cocos); win_escristr(strin);
    }
  }
  if (r != 0)
  {	win_fi();
	fprintf(stderr,"Error: no s'ha pogut inicialitzar el joc:\n");
	switch (r)
	{ case -1: fprintf(stderr,"  nom de fitxer erroni\n"); break;
	  case -2: fprintf(stderr,"  numero de columnes d'alguna fila no coincideix amb l'amplada del tauler de joc\n"); break;
	  case -3: fprintf(stderr,"  numero de columnes del laberint incorrecte\n"); break;
	  case -4: fprintf(stderr,"  numero de files del laberint incorrecte\n"); break;
	  case -5: fprintf(stderr,"  finestra de camp de joc no oberta\n"); break;
	  case -6: fprintf(stderr,"  posicio inicial del menjacocos damunt la pared del laberint\n"); break;
	  case -7: fprintf(stderr,"  posicio inicial del fantasma damunt la pared del laberint\n"); break;
	}
	exit(7);
  }
}




/* funcio per moure un fantasma una posicio; retorna 1 si el fantasma   */
/* captura al menjacocos, 0 altrament					*/
void * mou_fantasma(void * index)
{
  int i = (intptr_t) index;
  objecte seg;
  //int ret;
  int k, vk, nd, vd[3];
  
  pthread_mutex_lock(&mutex);
  fantasmes[i].a = win_quincar(fantasmes[i].f,fantasmes[i].c);
  pthread_mutex_unlock(&mutex);
  if (fantasmes[i].a == c_req) {
    fprintf(stderr,"  posicio inicial del fantasma damunt la pared del laberint\n"); /* error: fantasma sobre pared */
    exit(7);
  }
  pthread_mutex_lock(&mutex);
  win_escricar(fantasmes[i].f,fantasmes[i].c,i+'0',NO_INV);
  pthread_mutex_unlock(&mutex);
  do{
  //ret = 0; 
    nd = 0;
    for (k=-1; k<=1; k++)		/* provar direccio actual i dir. veines */
    {
      vk = (fantasmes[i].d + k) % 4;		/* direccio veina */
      if (vk < 0) vk += 4;		/* corregeix negatius */
      seg.f = fantasmes[i].f + df[vk]; /* calcular posicio en la nova dir.*/
      seg.c = fantasmes[i].c + dc[vk];
      pthread_mutex_lock(&mutex);
      seg.a = win_quincar(seg.f,seg.c);	/* calcular caracter seguent posicio */
      pthread_mutex_unlock(&mutex);
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
      pthread_mutex_lock(&mutex);
      seg.a = win_quincar(seg.f,seg.c);	/* calcular caracter seguent posicio */
      if ((seg.a==' ') || (seg.a=='.') || (seg.a=='0'))
      {
        win_escricar(fantasmes[i].f,fantasmes[i].c,fantasmes[i].a,NO_INV);	/* esborra posicio anterior */
        fantasmes[i].f = seg.f; fantasmes[i].c = seg.c; fantasmes[i].a = seg.a;	/* actualitza posicio */
        win_escricar(fantasmes[i].f,fantasmes[i].c,i+1+'0',NO_INV);		/* redibuixa fantasma */
        pthread_mutex_unlock(&mutex);
        pthread_mutex_lock(&mutex);
        if (fantasmes[i].a == '0') 
        {
          fi2 = 1;		/* ha capturat menjacocos */
        }
        pthread_mutex_unlock(&mutex);
      }else{
        pthread_mutex_unlock(&mutex);
      }
    }
    win_retard(retard*fantasmes[i].r);
  } while (!fi1 && !fi2);

  
  //return(ret);
}




/* funcio per moure el menjacocos una posicio, en funcio de la direccio de   */
/* moviment actual; retorna -1 si s'ha premut RETURN, 1 si s'ha menjat tots  */
/* els cocos, i 0 altrament */
void * mou_menjacocos(void * null)
{
  //char string[12];
  int status;
  objecte seg;
  int tec;//, ret;
  
  //ret = 0;
  do
  {
    pthread_mutex_lock(&mutex);
    tec = win_gettec();
    pthread_mutex_unlock(&mutex);
    if (tec != 0)
    {
      switch (tec)		/* modificar direccio menjacocos segons tecla */
      {
        case TEC_AMUNT:	  mc.d = 0; break;
        case TEC_ESQUER:  mc.d = 1; break;
        case TEC_AVALL:	  mc.d = 2; break;
        case TEC_DRETA:	  mc.d = 3; break;
        case TEC_RETURN:  fi1 = -1; break;
      }
    }

    seg.f = mc.f + df[mc.d];	/* calcular seguent posicio */
    seg.c = mc.c + dc[mc.d];
    pthread_mutex_lock(&mutex);
    seg.a = win_quincar(seg.f,seg.c);	/* calcular caracter seguent posicio */
    if ((seg.a == ' ') || (seg.a == '.'))
    {
      win_escricar(mc.f,mc.c,' ',NO_INV);		/* esborra posicio anterior */
      mc.f = seg.f; mc.c = seg.c;			/* actualitza posicio */
      win_escricar(mc.f,mc.c,'0',NO_INV);		/* redibuixa menjacocos */
      pthread_mutex_unlock(&mutex);
      if (seg.a == '.')
      {
        pthread_mutex_lock(&mutex);
        cocos--;
        if (cocos == 1) //Error continuava el joc amb 0 cocos, per això l'1 
        {
          fi1 = 1;
        }
        pthread_mutex_unlock(&mutex);
      }
    }else{
      pthread_mutex_unlock(&mutex);
    }
    win_retard(retard*mc.r);
  } while (!fi1 && !fi2);
  //return(ret);
}







/* programa principal				    */
int main(int n_args, const char *ll_args[])
{
  int rc, p, n, t_seg, min, seg, status;		/* variables locals */

  srand(getpid());		/* inicialitza numeros aleatoris */

  if ((n_args != 2) && (n_args !=3))
  {	
    fprintf(stderr,"Comanda: cocos0 fit_param [retard]\n");
  	exit(1);
  }
  carrega_parametres(ll_args[1]);

  if (n_args == 3) retard = atoi(ll_args[2]);
  else retard = 100;

  rc = win_ini(&n_fil1,&n_col,'+',INVERS);	/* intenta crear taulell */
  if (rc == 0)		/* si aconsegueix accedir a l'entorn CURSES */
  {
    inicialitza_joc();
    pthread_mutex_init(&mutex, NULL); /* inicialitza el semafor */
    p = 0; n = 0;
    if(pthread_create(&tid[n], NULL, mou_menjacocos, NULL) == 0)
      n++;

    for(int i = 0; i<total_fantasmes; i++){
      if(pthread_create(&tid[n], NULL, mou_fantasma, (void*)(intptr_t) i) == 0)
      {
        n++;
      }
    }

    t_seg = 0; min = 0; seg = 0;

    do			/********** bucle principal del joc **********/
    { 
      win_retard(1000);
	    seg=seg+1;
      min=seg/60;
      t_seg=seg%60;
      pthread_mutex_lock(&mutex);
      sprintf(strin,
          "%d:%2.d, Cocos: %d\n",
          min,t_seg,cocos-1); //Error continuava el joc, per això el -1
      win_escristr(strin);	
      pthread_mutex_unlock(&mutex);
      
    } while (!fi1 && !fi2);

    for (int i=0;i<=n;i++){
      pthread_join(tid[i], (void **) &status);
    }

    pthread_mutex_destroy(&mutex); /* destrueix el semafor */

    win_fi();

    
    if (fi1 == -1) printf("S'ha aturat el joc amb tecla RETURN!\n");
    else 
    { 
      if (fi1) printf("Ha guanyat l'usuari!\n");
	    else printf("Ha guanyat l'ordinador!\n"); 
    }
  }
  else
  {	
    fprintf(stderr,"Error: no s'ha pogut crear el taulell:\n");
    switch (rc)
    { case -1: fprintf(stderr,"camp de joc ja creat!\n");
        break;
      case -2: fprintf(stderr,"no s'ha pogut inicialitzar l'entorn de curses!\n");
        break;
      case -3: fprintf(stderr,"les mides del camp demanades son massa grans!\n");
        break;
      case -4: fprintf(stderr,"no s'ha pogut crear la finestra!\n");
        break;
    }
	  exit(6);
  }
  return(0);
}
