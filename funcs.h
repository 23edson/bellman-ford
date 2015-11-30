#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONST 40 //constante para tamanho de arquivo
#define IP 15 //constante do ip
#define MAX_PARENT 20 //vetor de pais tamanho fixo
#define MAXFILA 100//tamanho máximo da fila 
#define MAX_TENTATIVAS 3 //número max de tentativas após timeout
#define TIMEOUT 2 //temporizador
#define INFINITO 999 //representacao do infinito
#define MAX_TIME_DV 4 //tempo de envio do DV
#define CONSTANTE_DV 20 //numero de roteadores no vetor de distancia 

//tabela de roteamento

/**
 * @struct tabela_t - representa a tabela de roteamento.
 * 
 * alterado - flag para o caso de tiver atualizações na tabela
 * enlace - flag para cada roteador ( identificar as adjacencias após inicializar)
 * idVizinho - vetor contendo o id de cada roteador do grafo;
 * custo - vetor de custo mínimo para cada roteador;
 * idImediato - o próximo roteador no caminho até o destino.
 * 
 **/
typedef struct tab{
  
  int alterado;
  int *enlace;
  int *idVizinho; //vertice
  int *custo;   //custo minimo
  int *idImediato; //proximo vertice no caminho até idVizinho
}tabela_t;


/**
 * @struct DistVector_t - Estrutura do vetor de distancia
 * 
 * router - vetor que com id's de cada roteador
 * dist - a distância estimada 
 **/

typedef struct dv{
	//int exists;
	int router[MAX_PARENT];
	int dist[MAX_PARENT];
	
}DistVector_t;

/**
 * @struct msg_t - representa o pacote a ser enviado, juntamente com a mensagem do usuário.
 * tipo - tipo da msg ( 1. Usuário; 2. Vetor de distancia);
 * idMsg - identificador da mensagem;
 * origem - Roteador que enviou a mensagem;
 * destino - Roteador destino para a msg;
 * nextH - Próximo roteador no caminho até o destino;
 * ip - ip do rot. origem;
 * text - a mensagem em si (Max. 100 bytes). (nao usado para msg do tipo 2);
 * pSize - contador do vetor de parent. (nao usado para msg do tipo 2);
 * ack - flag de confirmação de pacote. (nao usado para msg do tipo 2);
 * parent - vetor estático que conta os roteador pelo caminho. (nao usado para msg do tipo 2);
 * DV - vetor de distancia para ser enviado ( nao usado para msn do tipo 1);
 *
 **/

typedef struct mensagem{
  int tipo; //tipo da mensagem
  int idMsg; //identificador da msg
  int origem; //quem enviou
  int destino; //para onde vai
  int nextH; //próximo roteador no caminho
  char ip[IP]; //ip de quem enviou
  char text[105]; //mensagem
  int pSize;
  int ack; //flag para confirmacao
  int parent[MAX_PARENT]; //vetor de parent para o caso de info
  //DistVector_t *DV;
  DistVector_t DV;
}msg_t;


/**
 * @struct router_t - Representa as informações sobre um roteador.
 * id - Identificador do roteador;
 * port - porta que este roteador escuta;
 * ip - Endereço ip.
 * 
 **/
typedef struct rt{
  
  int id;  //id do roteador
  int port; //porta associada ao socket
  char ip[IP]; //ip
}router_t;



tabela_t *leEnlaces( char enl[CONST], int count, int myId); //lê tabela de roteamento
router_t *leInfos(char rout[CONST], int id); //lê info sobre roteador
int countIn(char rot[CONST]); //conta vértices do grafo
void enviarMsg(void); //Msg do usuário
void server(void); //server que recebe mensagens
void serverControl(void); //controle das filas
msg_t initDV(msg_t me, int who, int id);//inicia o DV
void SendDV(void); //thread que controla o DV
