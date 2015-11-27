/**
 * Compilado com a versão:
 * 		-gcc version 4.6.3 (Ubuntu/Linaro 4.6.3-1ubuntu5)
 * Linux Ubuntu 12.04LTS
 * 
 * Dijktra code
 *   -http://www.vivaolinux.com.br/script/Algoritmo-de-Dijkstra
 *
 **/

#include <math.h>
#include "funcs.h"

#define CONST 40
#define MAX 200


//vetor de custos e adjacencias
int *cost;


int countIn(char rot[CONST]){
  //Conta a quantidade de vértices 
  FILE *arq;
  int i,adj;
  int count = 0;
  char ip[CONST];
  
  if(!(arq = fopen(rot,"r"))){
    printf("Arquivo roteador.config nao foi encontrado\n");
    return 0;
  }
  while(fscanf(arq,"%d %d %s", &i, &adj, ip)!=EOF)count++;
	
  fclose(arq);
  return count;

}

//Esta função contabilidade para cada vertice, o caminho mínimo para todos os demais
tabela_t *leEnlaces( char enl[CONST], int count, int myId){

  tabela_t *myConnect;
  FILE *arq;
  int i,adj,custo;

  if(!(myConnect = (tabela_t *)malloc(sizeof(tabela_t))))
		return NULL;

  
  if(!(myConnect->idVizinho = (int *)malloc(sizeof(int)*(count))) ||
     (!(myConnect->custo = (int *)malloc(sizeof(int)*(count)))) ||
     (!(myConnect->idImediato = (int *)malloc(sizeof(int)*(count))))||
     (!(myConnect->enlace = (int *)malloc(sizeof(int)*(count)))))
      return NULL;
  
  
  //if(!(cost = (int *)malloc(sizeof(int)*(count*count))))
	//	return NULL;

  if(!(arq = fopen(enl,"r"))) return NULL;

   //inicializa a tabela de roteamento
  for(i=0; i< count;i++){
	  myConnect->idVizinho[i] = i+1;
	  myConnect->custo[i] = INFINITO;
	  myConnect->idImediato[i] = i+1;
	  myConnect->enlace[i] = 0;
	  
  }
  //custo para si mesmo é zero    
  myConnect->idVizinho[myId-1] = myId; 
  myConnect->custo[myId-1] = 0;
  myConnect->idImediato[myId-1] = myId;
  myConnect->enlace[myId-1] = 2;
  myConnect->alterado = 0;
  //lê arestas com respectivos custos
  while(fscanf(arq,"%d %d %d", &i, &adj, &custo) != EOF){
		if(i == myId && adj != myId){
			myConnect->idVizinho[adj-1] = adj;
			myConnect->custo[adj-1] = custo;
			myConnect->idImediato[adj-1] = adj;
			myConnect->enlace[adj-1] = 1;
		}
		else if( i != myId && adj == myId){
			myConnect->idVizinho[i-1] = i;
			myConnect->custo[i-1] = custo;
			myConnect->idImediato[i-1] = i;
			myConnect->enlace[i-1] = 1;
		}
  }
  fclose(arq);
  return myConnect;
}


//Devolve as informações sobre o roteador X, isto é (id, porta, ip)
router_t *leInfos(char rout[CONST], int id){

	FILE *arq = fopen(rout, "r");
	router_t *myRouter = (router_t *)malloc(sizeof(router_t)*1);

	if(!myRouter)return NULL;

	if(!arq)return NULL;

	while(fscanf(arq,"%d %d %s", &(myRouter->id), &(myRouter->port), myRouter->ip)!=EOF){
		if(id == myRouter->id)break;
	}

	if(id != myRouter->id){return NULL;}

	fclose(arq);
	return myRouter;
}




/*void dijkstra(int vertices,int origem,int destino)
{
  int i,v, cont = 0;
  int *ant, *tmp;
  int *z;     //vertices para os quais se conhece o caminho minimo 
  double min;
double dist[vertices]; // vetor com os custos dos caminhos 


  // aloca as linhas da matriz                                                                                                         
  ant = calloc (vertices, sizeof(int *));                                                                                                 
  tmp = calloc (vertices, sizeof(int *));                                                                                                 
  if (ant == NULL) {                                                                                                                      
    printf ("** Erro: Memoria Insuficiente **");                                                                                    
    exit(-1);                                                                                                                       
  }                                                                                                                                       
                                                                                                                                                
  z = calloc (vertices, sizeof(int *));                                                                                                   
  if (z == NULL) {                                                                                                                        
    printf ("** Erro: Memoria Insuficiente **");                                                                                    
    exit(-1);                                                                                                                       
  }                                                                                                                                       
                                                                                                                                                
  for (i = 0; i < vertices; i++) {                                                                                                        
    if (cost[(origem - 1) * vertices + i] !=- 1) {                                                                                
      ant[i] = origem - 1;                                                                                                    
      dist[i] = cost[(origem-1)*vertices+i];                                                                                
    }                                                                                                                               
    else {                                                                                                                          
      ant[i]= -1;                                                                                                             
      dist[i] = HUGE_VAL;                                                                                                     
    }                                                                                                                               
    z[i]=0;                                                                                                                         
  }
  z[origem-1] = 1;
  dist[origem-1] = 0;

  // Laco principal 
  do {

    // Encontrando o vertice que deve entrar em z                                                                                 
    min = HUGE_VAL;                                                                                                                 
    for (i=0;i<vertices;i++)                                                                                                        
      if (!z[i])                                                                                                              
	if (dist[i]>=0 && dist[i]<min) {                                                                                
	  min=dist[i];v=i;                                                                                        
	}                                                                                                               
                                                                                                                                                
	// Calculando as distancias dos novos vizinhos de z                                                                           
    if (min != HUGE_VAL && v != destino - 1) {                                                                                      
      z[v] = 1;                                                                                                               
      for (i = 0; i < vertices; i++)                                                                                          
	if (!z[i]) {                                                                                                    
	  if (cost[v*vertices+i] != -1 && dist[v] + cost[v*vertices+i] < dist[i]) {                           
	    dist[i] = dist[v] + cost[v*vertices+i];                                                       
	    ant[i] =v;                                                                                      
	  }                                                                                                       
	}                                                                                                                       
    }                                                                                                                               
  } while (v != destino - 1 && min != HUGE_VAL);

  printf("\tDe %d para %d: \t", origem, destino);
  if (min == HUGE_VAL) {
    printf("Nao Existe\n");
    printf("\tCusto: \t- \n");
  }
  else {
    i = destino;
    i = ant[i-1];
    while (i != -1) {
      //      printf("<-%d",i+1);                                                                                                     
      tmp[cont] = i+1;
      cont++;
      i = ant[i];
    }

    for (i = cont; i > 0 ; i--) {
      printf("%d -> ", tmp[i-1]);                                                                                             
    }                                                                                                                               
    printf("%d", destino);                                                                                                          
                                                                                                                                                
    printf("\n\tCusto:  %d\n",(int) dist[destino-1]);                                                                               
  }
}

*/
