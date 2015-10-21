#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONST 40 //constante para tamanho de arquivo
#define IP 15 //constante do ip
#define MAX_PARENT 100 //vetor de pais tamanho fixo
#define MAXFILA 50 //tamanho máximo da fila 
#define MAX_TENTATIVAS 3 //número max de tentativas após timeout
#define TIMEOUT 2 //temporizador
#define INFINITO 999 //representacao do infinito

//tabela de roteamento

/**
 * @struct tabela_t - representa a tabela de roteamento.
 * idVizinho - vetor contendo o id de cada roteador do grafo;
 * custo - vetor de custo mínimo para cada roteador;
 * idImediato - o próximo roteador no caminho até o destino.
 * 
 **/
typedef struct tab{
  
  int *idVizinho; //vertice
  int *custo;   //custo minimo
  int *idImediato; //proximo vertice no caminho até idVizinho
}tabela_t;


/**
 * @struct msg_t - representa o pacote a ser enviado, juntamente com a mensagem do usuário.
 * idMsg - identificador da mensagem;
 * origem - Roteador que enviou a mensagem;
 * destino - Roteador destino para a msg;
 * nextH - Próximo roteador no caminho até o destino;
 * ip - ip do rot. origem;
 * text - a mensagem em si (Max. 100 bytes);
 * pSize - contador do vetor de parent;
 * ack - flag de confirmação de pacote;
 * parent - vetor estático que conta os roteador pelo caminho.
 *
 **/

typedef struct mensagem{
  int idMsg; //identificador da msg
  int origem; //quem enviou
  int destino; //para onde vai
  int nextH; //próximo roteador no caminho
  char ip[IP]; //ip de quem enviou
  char text[105]; //mensagem
  int pSize;
  int ack; //flag para confirmacao
  int parent[MAX_PARENT]; //vetor de parent para o caso de info
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
