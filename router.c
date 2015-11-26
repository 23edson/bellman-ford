
/**
 * Compilado com a versão:
 * 		-gcc version 4.6.3 (Ubuntu/Linaro 4.6.3-1ubuntu5)
 * Linux Ubuntu 12.04LTS
 *
 * Este programa representa um roteador, onde o seu objetivo é 'rotear' pacotes
 * UDP da origem até o destino. O programa recebe  um argumenta da linha de comandos, sendo
 * um número inteiro que representa o ID do roteador sendo instanciado.
 *
 * Primeiramente, é lido a partir de dois arquivos informações sobre os enlaces
 * e os roteadores, sendo eles "enlaces.config" e "roteador.config" respectivamente.Para
 * fins de simplicidade, é assumido que ambos os arquivos estão no diretório deste
 * código-fonte. A topologia da rede é estática, ou seja, não sofre alterações durante 
 * o seu ciclo de vida. 
 *
 * O processamento dos pacotes é realizado com o uso de uma fila simples, onde os pacotes
 * são dispostos na fila de modo que o primeiro da fila é o primeiro a ser atendido (FIFO),
 * com excessão de mensagens de confirmação(ACK), estas são removidas da fila assim que
 * recebidas. Este escalonamento refere-se a quaisquer tipo de mensagem, seja ela de
 * encaminhamento, ou seja, para o caso de o roteador que recebeu o pacote não ser o
 * destino final, outra possibilidade é quando o roteador é o destino, e a última para
 * o caso de ser uma mensagem de confirmação.
 * 
 * Para o caso de envio de pacotes, é necessário assegurar um tempo finito de tentativas,
 * deste modo, quando uma mensagem é escalonada para envio, é definido por padrão um tempo
 * de espera(TIMEOUT) de 2 segundos. Contudo, também é definido uma quantidade finita de 
 * tentativas, neste caso são 3 tentativas; isto nos da um tempo total de 6 segundos para
 * cada pacote.
 *
 * Cada roteador dispõe de uma tabela de roteamento, nela estão colocadas informações 
 * sobre todos os roteadores, assim, para cada um deles está definido o próximo salto
 * (NextHop), seguido pelo custo mínimo para o destino a partir da origem calculada
 * por uma função de Dijkstra.
 *
 * A implementação de mensagens de confirmação funciona da seguinte maneira: quando uma
 * mensagem é roteada da origem até o destino, cada roteador do caminho é marcado em um
 * vetor de parent, assim a confirmação é propagada do destino até a origem.
 *
 * A aplicação permite o envio de mensagens, para isto cada roteador implementa uma
 * interface de interação com o usuário, isto é através de uma thread são recebidos
 * um destino e uma mensagem lida do teclado. Posteriormente esta mensagem é posta
 * na fila para o escalonamento.
 * 
 **/


#include "funcs.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#define GETFILE 105 //constante para mensagem do usuário
//#define MAXFILA 100 //constante da fila de pacotes do roteador
#define ALOCACAO_ERRO 0
//gcc funcs.h readFiles.c router.c -D_REENTRANT -lpthread -o m

/**
 * @struct fila_t - Descreve a fila de pacotes do roteador X.
 * time_t timestamp - Armazena o tempo decorrido na fila do roteador; 
 * int tentativas - Quantidade de retransmissões após timeout;
 *  msg *mesfg - Estrutura do tipo msg. Representa um pacote;
 **/

typedef struct fila{
	time_t timestamp;
	int tentativas;
	int id;
	msg_t *mesg;
}fila_t;



//Prototipação
void remove_f();
int iniciaFila();
void insereFila(msg_t *buf);
void remove_fix(int i);
void insere_fix(msg_t *nova, int save, int saveID, time_t back);
//msg_t *copyStatic(msg_t *new, msg_t buf);
msg_t *copyData(msg_t *new, msg_t *buf);
DistVector_t *copyVector(DistVector_t *new, DistVector_t *buf);
//Variáveis compartilhadas entre as threads

int vertices,vizinhos=0; // Número de vértices do grafo;
int tamanho = 0;//controle da fila, inicialmente vazia.
tabela_t *myConnect = NULL; //lista de roteamento
router_t *myRouter = NULL; //informações do roteador
fila_t *filas = NULL;
DistVector_t *vetor = NULL;
msg_t mensger[MAX_PARENT];
time_t *nodeTime;

char rout_u[20] = "roteador.config"; //Arquivo de configuração dos roteadores;
char link_u[20] = "enlaces.config"; //Arquivo de conf. dos enlaces;

pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER; //mutex para controlar o acesso as variáveis globais

void printTime(){
	
	int i;
    time_t ltime; /* calendar time */
    ltime=time(NULL); /* get current time */
    char *tempo = (char *)malloc(sizeof(char)*30);
    //strcpy(tempo, "Time : ");
    tempo = asctime( localtime(&ltime) );
    for(i = 11; i < 20; i++){
		if( i == 11)
			printf("  Time : ");
		printf("%c",tempo[i]);
	}
	printf("\n");
	//free(tempo);
	
}


DistVector_t *atualizaDV(DistVector_t *vector){
	
	int i;
	for(i = 0; i < vertices; i++){
		vector->dist[i] = myConnect->custo[i];
		vector->router[i] = myConnect->idVizinho[i];
			
	}
		
	myConnect->alterado = 0;
	
	return vector;
}


DistVector_t *initDV2(DistVector_t *vetor){
	
	int i;
	
	for(i = 0; i < vertices; i++){
		vetor->dist[i] = INFINITO;
		vetor->router[i] = INFINITO;
	}
	return vetor;
}
msg_t initDV(msg_t me, int who,int id){
	
	router_t *vis = NULL;
	//printf("valor %d  edma", who);
	//int i, j;
	//char ip[IP];
	vis = leInfos(rout_u, who);
	//puts("Oii");puts("tt");
	strcpy(me.ip, vis->ip);
	free(vis);
	me.tipo = 2;
	me.ack = 0;
	me.idMsg = id;
	me.origem = myRouter->id;
	me.destino = who;
	me.nextH = who;
	//strcpy(me->ip, ip);
	
	//me.DV = (DistVector_t *)malloc(sizeof(DistVector_t));
	//if(!me.DV){
	//	me.tipo = ALOCACAO_ERRO;
		
	//}
	
	
	//me->DV->exists = (int *)malloc(sizeof(int)*vertices);
	//me.DV->dist = (int *)malloc(sizeof(int)*vertices);
	//me.DV->router = (int *)malloc(sizeof(int)*vertices);
	
	return me;
}
/*msg_t *structureDV(msg_t mensg){
	
	int *vet;
	
	vetor = (DistVector_t *)malloc(sizeof(DistVector_t));
	vetor->dist = (int *)malloc(sizeof(int)*vertices);
    vetor->router = (int *)malloc(sizeof(int)*vertices);
    vetor = initDV2(vetor); //inicializa todo mundo com infinito
    vetor->dist[myRouter->id-1] = 0;
    vetor->router[myRouter->id-1] = myRouter->id;//distancia para mim mesmo é zero
    
    vet = (int *)malloc(sizeof(int)*vertices);
    
    
    for(i = 0; i < vertices; i++){
	  //vetor->dist[i] = INFINITO;
	  //vetor->router[i] = INFINITO;
	  if((myConnect->idVizinho[i] != myRouter->id) && (myConnect->custo[i] != INFINITO) && (myConnect->idImediato[i] == myConnect->idVizinho[i])){
			vet[vizinhos] = myConnect->idVizinho[i];
			vetor->dist[i] = myConnect->custo[i];
			vetor->router[i] = myConnect->idImediato[i];
			vizinhos++;
			
		}
  }
  
  mensg = (msg_t *)malloc(sizeof(msg_t)*vizinhos);
  
  for( i = 0; i < vizinhos; i++){
	  mensg[i] = initDV(mensg[i], vet[i],id++);
	  mensg[i].DV = vetor;
   }
}*/

void imprimeTabela(){
	
	int i;
	
	printf("----DV UPDATE---------\n");
	printTime();
    printf("     Rot : %d\n", myConnect->idVizinho[myRouter->id-1]);
	printf("----------------------\n");
	printf(" ROT.|   CUSTO\n");
	printf("     ------------------\n");
		for(i = 0 ; i < vertices; i++){
			  
			if(myConnect->idVizinho[i] != myRouter->id){
				if(myConnect->custo[i] == INFINITO)
					printf("   %d |   INF \n", myConnect->idVizinho[i]);
				else printf("   %d |    %d \n", myConnect->idVizinho[i], myConnect->custo[i]);

			  }
			  
		}
	printf("-----------------------\n");
}

void SendDV(void){

  time_t timest;
 
  //router_t *vis = NULL;
  
  //msg_t aux;
 
  //int vizinhos = 0;
  int i,j,k;
  int *vet;
  int id = 1;
  vetor = (DistVector_t *)malloc(sizeof(DistVector_t));
  //vetor->dist = (int *)malloc(sizeof(int)*vertices);
  //vetor->router = (int *)malloc(sizeof(int)*vertices);
  vetor = initDV2(vetor); //inicializa todo mundo com infinito
  vetor->dist[myRouter->id-1] = 0;
  vetor->router[myRouter->id-1] = myRouter->id;//distancia para mim mesmo é zero
  
  
  vet = (int *)malloc(sizeof(int)*vertices);
  
  
  for(i = 0; i < vertices; i++){
	  //vetor->dist[i] = INFINITO;
	  //vetor->router[i] = INFINITO;
	  if((myConnect->idVizinho[i] != myRouter->id) && (myConnect->custo[i] != INFINITO) && (myConnect->idImediato[i] == myConnect->idVizinho[i])){
			vet[vizinhos] = myConnect->idVizinho[i];
			vetor->dist[i] = myConnect->custo[i];
			vetor->router[i] = myConnect->idImediato[i];
			vizinhos++;
			
		}
  }
  nodeTime = (time_t *)malloc(sizeof(time_t)*vizinhos);
  //myConnect->alterado = 0;
  //printf("vv: %d %d %d ~  \n", vet[0], myConnect->idVizinho[0],vizinhos);
  //mensg = (msg_t *)malloc(sizeof(msg_t)*vizinhos);
  //puts("oi");
  
  for(i = 0; i < vizinhos; i++){
	//DistVector_t *vector = (DistVector_t *)malloc(sizeof(DistVector_t));
	for(j = 0; j < vertices; j++){
		mensger[i].DV.dist[j] = vetor->dist[j];
		mensger[i].DV.router[j] = vetor->router[j];
	}
	mensger[i] = initDV(mensger[i], vet[i],id++);
	nodeTime[i]= time(0);
	//mensger[i].ack = (int)time(0);
	 
 }
  free(vet);
   
	//printf("id %d dest %d e %d\n",mensg[2].origem,mensg[2].destino,mensg[2].DV->dist[1]);
	//printf("cheguei");
  
	//int cc = 0;
	//printf("id1 %d des1 %d\n",mensg[1].origem,mensg[1].destino);
  //usleep(300000);
  //timestamp = time(0);
  
  //printf("tempo %f", timestamp);
  
  //usleep(200000);
  //printf("vis %d ", vizinhos);
  timest = time(0);
  while(1){
	  usleep(500000);//500ms
	  //msg_t *conf = (msg_t *)malloc(sizeof(msg_t));
	  //timestamp = time(0);]
	  //printf("ddddd %d",cc++);
	  pthread_mutex_lock(&count_mutex);
	  //printf("ddddd %d",cc++);
	  //double tempo = difftime(time(0), timest);
	  double tempo = difftime(time(0), timest);//printf("%lf ", tempo);
	  if(myConnect->alterado == 1){//printf("dddddx");
		 // puts("entreeei");
		  vetor = atualizaDV(vetor);
		  if(!filas)
			if(!iniciaFila())return;
		    
		  for( i = 0; i < vizinhos; i++){
			  if(mensger[i].tipo == 2){
					for(j = 0; j < vertices; j++){
						mensger[i].DV.dist[j] = vetor->dist[j];
						mensger[i].DV.router[j] = vetor->router[j];
					}		
					mensger[i].idMsg = id++;
					//nodeTime[i] = time(0);
					insereFila(&mensger[i]);
					printf("Roteador : %d enviando pacote DV #%d para roteador %d\n", myRouter->id,mensger[i].idMsg, mensger[i].destino);
			  }
		  }
		  timest = time(0);
		  imprimeTabela();
		  
	
	  }
	  
	 
	  
	  else if(tempo >= MAX_TIME_DV){//printf("rrr");
		    
			if(!filas)
				if(!iniciaFila())return;
				
			//if(filas[i].exists 
		/*	for( i = 0; i < vizinhos; i++){
				mensger[i].idMsg = id++;
				insereFila(&mensger[i]);
			}
			timest= time(0);
	}*/
			for( i = 0; i < vizinhos; i++){
				
				//time_t aux = (time_t)mensger[i].ack;
				tempo = difftime(time(0), nodeTime[i]);
				if( tempo >= MAX_TIME_DV*MAX_TENTATIVAS){
					mensger[i].tipo = 0;
					myConnect->custo[mensger[i].destino-1] = INFINITO;
					//nodeTime[i] = time(0);
					//mensger[i].ack = (int)time(0);
					for(j = 0; j < vizinhos; j++){
						mensger[j].DV.dist[mensger[i].destino-1] = INFINITO;
						mensger[j].DV.router[mensger[i].destino-1] = INFINITO; 
					}
				}
				//conf = copyData(conf,&mensg[i]);
				else{
					mensger[i].idMsg = id++;
				insereFila(&mensger[i]);}
				
				//printf("des :%d e %d \n", filas[tamanho-1].mesg->destino, tamanho);
				//puts("ok");
				//printf("%d e %d %d\n", mensg[i].DV->router[0], mensg[i].DV->dist[0],mensg[i].destino);
				//printf("des :%d e %d ", mensg[i].destino, tamanho);
				//if(tamanho == 3){
					//printf("des :%d e %d \n", filas[0].mesg->destino, tamanho);
					//printf("des :%d e %d \n", filas[1].mesg->destino, tamanho);
					//printf("des :%d e %d \n", filas[2].mesg->destino, tamanho);}
			}
			
			
			//insereFila(&mensg);
			timest= time(0);
			// printf("teste :%d", teste++);
			
	  }
	 
	  pthread_mutex_unlock(&count_mutex);
  }
  //close(s);
  pthread_exit(NULL);


}



/**
 * @function enviarMSg - A função representa a interface de envios de mensagem através do socket
 * 		para um roteador específico; 
 * 
 * 	Lê da teclado um destino válido na topologia da rede e uma mensagem, então faz o encaminhamento
 *  correto até o destino. Esta função executa simultaneamente com as demais funções, controlada
 *  através de uma thread.
 * 
 **/
void enviarMsg(void){
	int s, i;
	char message[GETFILE];
	int destino;
	msg_t mensg;
	int id = 1;

	router_t *destRouter = NULL;
	usleep(300000);//espera 300ms
	while(1){
		printf("\n");
		printf("ROTEADOR NUMERO %d\n\n", myRouter->id);
		//usleep(100000); //espera mais 100ms para 
		printf("Enviar msg para roteador destino:");
			scanf("%d", &destino);

		fgetc(stdin);//pula \n do scanf
		//pthread_mutex_lock(&count_mutex);
		if(destino == myRouter->id){
			printf("\nEnviar para mim mesmo?\n");
			continue;
		}
		//pthread_mutex_unlock(&count_mutex);
		if(!(destRouter = leInfos(rout_u, destino))){
			//nao destino na topologia
			printf("Destino invalido\n");
		}

		else{
		
			for( i = 0; i < vertices; i++){ //inicializa a estrutura do pacote
				if(myConnect[myRouter->id-1].idVizinho[i] == destino){
					mensg.tipo = 1;
					mensg.origem = myRouter->id;
					mensg.destino = destino;
					mensg.nextH = myConnect->idImediato[i];
					mensg.ack = 0;
					mensg.pSize = 0;
					//mensg.DV = NULL;
					strcpy(mensg.ip, destRouter->ip);
					break;
				}
			
			}
			printf("Msg: ");
			fgets(message, 100,stdin);//100bytes no máximo
			strcpy(mensg.text, message);
		 
			pthread_mutex_lock(&count_mutex);//usa mutex para iniciar a fila, se necessário
			if(!filas)
				if(!iniciaFila())return;
		
			mensg.idMsg = id++;//identificador da mensagem
			mensg.parent[mensg.pSize] = myRouter->id;//caminho feito é registrado no vetor parent
			mensg.pSize++;
			
			insereFila(&mensg);//insere a mensagem preparada no final da fila
			//printf("fila %d\n", tamanho);
			pthread_mutex_unlock(&count_mutex);//libera para outras thread usarem
		
		}
	}

		close(s);
		pthread_exit(NULL);

}

/**
 * @function encaminhaMSg - Responsável por enviar o pacote no socket para algum roteador específico.
 * @param int s - Socket associado;
 * @param struct sockaddr_in *etc - Estrutura do socket associado;
 * @param msg *buf - A mensagem a ser enviada.
 * 
 * A função envia um pacote para um roteador, controlando se é uma mensagem de confirmação ou não. Esta é uma
 * função auxiliar do controle da fila.
 * 
 * 
 **/
int encaminhaMsg(int s, struct sockaddr_in *etc, msg_t *buf, int flag){ 
	
	int i;
	int t = sizeof(*etc);
	router_t *destRouter = NULL;
	
	if(buf->ack == 1 && buf->tipo == 1){ //se for mensagem de confirmação (ack)
		for(i = 0; i < vertices; i++){ //procura o roteador vizinho que pertence ao caminho mínimo do destino até a origem
			if(myConnect->idVizinho[i] == buf->parent[buf->pSize]){
				buf->nextH = myConnect->idImediato[i];
			
				if(!(destRouter = leInfos(rout_u, buf->nextH))){
					printf("Destino nao e valido\n");return 0;
				}
				break;
			}	
			
			
		}
	}
	else if(buf->tipo == 2){
		//printf("destinoo :%d \n", buf->destino);
		if(!(destRouter = leInfos(rout_u, buf->destino))){
			printf("Destino nao e valido\n");return 0;
		}
	}
	
	else{
		for( i = 0; i < vertices; i++){//procura o roteador vizinho que pertence ao caminho mínimo da origem até o destino
			if(myConnect->idVizinho[i] == buf->destino){
				buf->nextH = myConnect->idImediato[i];
				if(!(destRouter = leInfos(rout_u, buf->nextH))){
					printf("Destino nao e valido\n");return 0;
				}
				break;
			}
		}
	} 
	//printf("porrt %d", destRouter->port);
	
	etc->sin_port = htons(destRouter->port); //especifica a porta do socket
	
	if (inet_aton(destRouter->ip , &etc->sin_addr) == 0) {//Associa o IP
		fprintf(stderr, "inet_aton() error\n");
		return 0;
	}
	//printf("rrrrrr : %d", buf->DV->router[3]);
	if (sendto(s, buf, sizeof(msg_t), 0, (struct sockaddr *)etc, t) == -1) { //tenta enviar
		printf("Nao foi possivel encaminhar a mensagem()...\n");
		return 0;
	}
	if(!buf->ack && buf->tipo == 1) //define os formatos de msg de apresentação para o usuário
		printf("\nRoteador : %d encaminhando msg #%d de %d bytes para roteador : %d\n", myRouter->id,buf->idMsg, (int )strlen(buf->text),destRouter->id); 
	//else if(!buf->ack && buf->tipo == 2 && flag == 0)
		//printf("Roteador : %d enviando pacote DV #%d para roteador %d\n", myRouter->id,buf->idMsg, destRouter->id); 
	//else if(flag == 0)
		//printf("\nRoteador : %d enviando confirmacao de pacote #%d para rot. %d\n", myRouter->id, buf->idMsg, destRouter->id);
	//puts("aq14");
	free(destRouter);
	return 1;
	
}



/**
 * @function server - Representa o servidor responsável por receber os pacotes enviaos atráves do socket.
 * 
 * Essa função é controlada por uma thread particular, onde a mesma é criada por primeiro. Assim, é definido
 * o tipo do socket(neste caso UDP), a porta de conexão, variáveis internas de controle, etc. Como a função faz
 * uso da fila (global), então se faz necessário o uso de mutex para sincronizar os acessos.
 * 
 * A chamada à função recvfrom é bloqueante, ou seja, neste ponto a thread fica bloqueada enquanto não receber algum
 * pacote. Após receber algum pacote, é verificado se o mesmo chegou ao destino ou se ainda é preciso reencaminhar 
 * para um próximo roteador. Também é atráves da função recvfrom que as confirmações de pacotes são recebidas.
 * 
 **/

void server(void){ //Para receber as mensagens
	
	int s,temp, recv_len,i,j,flag = 0;
	struct sockaddr_in si_me, si_other;
	socklen_t len;
	msg_t mensg;
	
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		printf("Problema com Socket do servidor .\n");
		exit(1);
	}

	memset((char *) &si_me, 0, sizeof(si_me));
	
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(myRouter->port);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if (bind(s , (struct sockaddr*)&si_me, sizeof(si_me)) == -1) {
		printf("bind(). Porta ocupada?.\n");
		exit(1);
	}
	pthread_mutex_lock(&count_mutex); //Acesso a fila, então o uso de mutex
	
	if(!iniciaFila()){
		printf("Impossivel alocar a fila\n");
		exit(1);
	}
	pthread_mutex_unlock(&count_mutex);
	while(1){
		
		fflush(stdout);
		memset((char *)&mensg, 0, sizeof(msg_t));
		//mensg.DV = NULL;
		
		if ((recv_len = recvfrom(s, &mensg, sizeof(msg_t), 0, (struct sockaddr *)&si_other, &len)) == -1) {
			printf("recvfrom() ");
			exit(1);
		}
		//printf("au mmmm %d %d %d --- \n" , mensg.origem, mensg.destino, mensg.nextH);
		
		pthread_mutex_lock(&count_mutex); //Acesso único também a partir deste ponto
		//puts("oi");
		
		if(mensg.tipo == 2){ //mensagem de dv
			
				printf("\n");
					for(i = 0; i < vertices; i++){
						printf("%d ", mensg.DV.router[i]);
						printf("%d \n", mensg.DV.dist[i]);
					}
			
				//msg_t *conf = (msg_t *)malloc(sizeof(msg_t));
				
				for(i = 0 ; i < vertices; i++){
					//printf("%d %d %d %d ..\n", mensg.DV.router[i], mensg.DV.dist[i], mensg.DV.dist[myRouter->id-1],myConnect->custo[i]);
					if(mensg.DV.router[i] != INFINITO && mensg.DV.router[i]!= myRouter->id) {
						temp = mensg.DV.dist[i] + mensg.DV.dist[myRouter->id-1];
						
						for(j = 0; j < vizinhos; j++){
							if(mensger[j].destino == mensg.origem){
								nodeTime[j] = time(0);
								break;
							}
						}
						if(j < vizinhos)
							if(mensg.DV.router[i] != INFINITO && mensger[j].tipo == 0)
								mensger[j].tipo = 2;
						
						if(mensg.DV.router[i] == myConnect->idVizinho[i] && myConnect->idImediato[i] == mensg.origem){
							if(myConnect->custo[i] != temp){
								myConnect->custo[i] = temp;
								//myConnect->idImediato[i] = mensg.origem;
								myConnect->alterado = 1;
							}
						}
						
						else if( (temp  < myConnect->custo[i])){
							myConnect->custo[i] = temp;
							myConnect->idImediato[i] = mensg.origem;
							myConnect->alterado = 1;
						} 
					}
		
				}
				if(myConnect->alterado == 1 )
					printf("Pacote DV #%d recebido do rot. %d\n", mensg.idMsg, mensg.origem);
					
				//printf("Pacote DV #%d recebido do rot. %d\n", mensg.idMsg, mensg.origem);
				//mensg.ack = 1;
				//mensg.nextH = mensg.origem;
				//mensg.origem = mensg.destino;
				//conf = copyData(conf, &mensg);
				//insereFila(conf);
			
		}
		
		else{
			if((mensg.destino != myRouter->id) && (mensg.ack == 0)){ //caso este nao for o destino
				
				for(i = 0; i < tamanho; i++){ //ignorar pacotes ja recebidos no caso de retransmissoes
					
					if(mensg.idMsg == filas[i].mesg->idMsg && mensg.nextH == myRouter->id){
						flag = 1;
						break;
					}
					
				}
				if(!flag){//se a mensagem nao é repetida
					msg_t *confr = (msg_t *)malloc(sizeof(msg_t));
				
					mensg.parent[mensg.pSize++] = myRouter->id; //coloca no vetor parent este roteador
					confr = copyData(confr,&mensg); //copia a mensagem para uma estrutura dinâmica
					insereFila(confr); //coloca no final da fila
				}
				flag=0;//flag de controle interno
			}
			else{ //caso este for o destino
			
				msg_t *conf = (msg_t *)malloc(sizeof(msg_t));
				if(!mensg.ack){ //se nao for msg de confirmacao
					mensg.ack = 1; //flag de confirmacao
					mensg.pSize--; //retira este roteador do caminho em parent
					conf = copyData(conf, &mensg);
					
					printf("\nRoteador : %d recebeu a msg de %d\n", myRouter->id, mensg.origem);
					printf("Msg: %s\n", mensg.text);
						
					insereFila(conf); //coloca na fila para enviar o ack
				}
				else{//caso recebe a confirmacao
					
					for(i = 0; i < tamanho; i++){//remove da fila o pacote 
						if(mensg.parent[mensg.pSize] == myRouter->id && filas[i].mesg->origem == mensg.origem){
							remove_fix(i);
							break;
						}
					}
					
					if(mensg.origem == myRouter->id){ //se este for o roteador que enviou a mensagem, neste ponto a mesma é confirmada
						printf("\nMensagem confirmada!!\n");
						
					}
					else{//senao apenas propaga a confirmaçao para o vizinho no caminho parent
						mensg.pSize--;
						conf = copyData(conf, &mensg);
						insereFila(conf);
					}
				}
			}
		}
		pthread_mutex_unlock(&count_mutex);
	}
}

/**
 * function serverControl - Controla a fila do roteador
 * 
 * A função cria um socket para controlar o envio dos pacotes aos demais roteadores. Esta função executa em uma thread separada.
 * Evidentemente usa mutex para o acesso aos elementos da fila. dentro do laço principal existe um tempo de espera de 500ms, para
 * eventuais sincronizações que possam ocorrer. A função basicamente controla quando enviar e reinviar os pacotes, ela controla o
 * timeout. O temporizador está definido como aproximadamente 2segundos, porém ele faz no máximo 3 tentativas, então aproxidamente
 * 6s entre todas as possíveis retransmissões.
 * 
 * A função precisa decidir o estado do pacote, isto é, se é pacote de confirmação, se é novo(vindo do usuário), ou se ele está
 * sendo retransmitido. A contagem de tempo é feito por uma variável do tipo time_t, onde é comparado a hora do sistema em segundos
 * menos a hora que foi colocado o pacote na fila.
 * 
 * 
 **/
	


void serverControl(){

	struct sockaddr_in controle;
	time_t back;
	int save,saveId;
	int s,i;

	pthread_mutex_lock(&count_mutex);//precisa exclusividade, pois usa a fila 
	if(!iniciaFila())return;

	pthread_mutex_unlock(&count_mutex);
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) { //cria socket UDP
	printf("Socket de controle.\n");
	return;
	}

	controle.sin_family = AF_INET; 
	memset((char *) &controle, 0, sizeof(controle));


	while(1){
		usleep(500000);//espera 500ms

		if(tamanho == 0)continue; //fila vazia
	//puts("umm");printf("r  u %d ", tamanho);
	
		pthread_mutex_lock(&count_mutex);
		for(i = 0; i < tamanho; i++){
			

			msg_t *conf = (msg_t *)malloc(sizeof(msg_t));
			if(filas[i].mesg->ack){ //caso recebeu o pacote
				encaminhaMsg(s,&controle,filas[0].mesg,filas[0].tentativas); //manda ack e remove da fila
				remove_f();
			}
			else if(filas[0].tentativas == 0){ //se a mensagem nao foi enviada, ou seja, veio do usuario
				int val = encaminhaMsg(s,&controle,filas[0].mesg,filas[0].tentativas);
				if(!val || filas[0].mesg->tipo == 2){
					
					remove_f();//caso deu erro, apenas remove
					
				}
				else{
					
					filas[0].tentativas++; //incrementa o numero de tentativas
					filas[0].timestamp = time(0); //conta o tempo a partir de agora
					save = filas[0].tentativas;
					saveId = filas[0].id; //copia em variaveis auxiliares
					back = filas[0].timestamp;
					conf = copyData(conf, filas[0].mesg);//copia para estrutura dinâmica
					
					if(filas[0].mesg->tipo == 1){//se nao for mensagem do atualizado do DV
						remove_f();
						insere_fix(conf, save, saveId, back);//tem efeito de pegar o primeiro e colocar por último na fila, mantendo os atributos
					}
				}
			
			}
			else if(filas[0].tentativas > 0 && filas[0].tentativas < MAX_TENTATIVAS){
				double tempo = difftime(time(0), filas[0].timestamp);
			 
				 if(filas[0].tentativas < MAX_TENTATIVAS && tempo > TIMEOUT){ //max 3 tentativas
					if(filas[0].mesg->tipo == 1)
						printf("\nRetransmissao\n");
					if(encaminhaMsg(s,&controle, filas[0].mesg,filas[0].tentativas)==0){
						remove_f();
					}
					else{
						
						//filas[i].timestamp = time(0);
						filas[i].tentativas++;
						back = time(0);
						save = filas[0].tentativas;
						saveId = filas[0].id;
						conf = copyData(conf,filas[0].mesg);
						
						if(filas[0].mesg->tipo == 1){//se nao for mensagem de atualizacao DV
							remove_f();
							insere_fix(conf,save,saveId,back); //coloca no final da fila
						}
					 }
				 }
				else if(filas[0].tentativas < MAX_TENTATIVAS && tempo <= TIMEOUT){ //Aprox 2s
					// encaminhaMsg(s,&controle, filas[i].mesg);
					 conf = copyData(conf,filas[0].mesg);
					 save = filas[0].tentativas;
					 back = filas[0].timestamp;
					 saveId = filas[0].id;
					 
					 if(filas[0].mesg->tipo == 1){//se nao for msg de DV
						remove_f();
						insere_fix(conf,save,saveId,back); //coloca no final da fila
						//total aproximadamente de 6s para tentar enviar
					}
				 }
			}
			else{
				if(filas[0].mesg->tipo == 1)
					printf("\nNao foi possivel encaminhar a mensagem\n");
					//if(filas[i].mesg->DV)puts("DV OK");
				
				remove_f(); //Desiste de enviar
			}
			

		}
		pthread_mutex_unlock(&count_mutex);
	}
	close(s);
	pthread_exit(NULL);
}


/**
 * @function copyData - faz a copia de dois pacotes
 * 
 * A função copia a msg A para a msg B
 * 
 * 
 **/

msg_t *copyData(msg_t *new, msg_t *buf){
	
	int i;
	new->tipo = buf->tipo;
	new->origem = buf->origem;
	new->destino = buf->destino;
	new->nextH = buf->nextH;
	strcpy(new->ip, buf->ip);
	if(buf->tipo==1)strcpy(new->text, buf->text);
	
	new->ack = buf->ack;
	new->idMsg = buf->idMsg;
	new->pSize = buf->pSize;
	if(buf->tipo == 2){
		for(i = 0; i < vertices; i++){
			new->DV.dist[i] = buf->DV.dist[i];
			new->DV.router[i] = buf->DV.router[i];
		}
		
	}else
		for(i = 0 ; i < vertices; i++)
			new->parent[i] = buf->parent[i];


	
	return new;


}
	
	
//inicializa a fila de pacotes (fila simples)
int iniciaFila(){
	
	int i;
	filas = (fila_t *)malloc(sizeof(fila_t)*MAXFILA); //fila com tamanho fixo MAXFILA
	
	if(!filas){
		printf("Falha na alocacao\n");
		return 0;
	}
	for(i = 0; i < MAXFILA; i++){
		filas[i].mesg = NULL;
		//filas[i].mesg->DV = NULL;
		filas[i].id = 0;
		filas[i].timestamp = 0;
		filas[i].tentativas = 0;
	}
	return 1;
}


//A função realiza a inserção de um pacote no final da fila
void insereFila(msg_t *buf){
	//puts("eu");
	msg_t *nova = (msg_t *)malloc(sizeof(msg_t));
	//int i;
	
	if(!filas){
		printf("Impossivel inserir. Problema com a fila\n");
		return;
	}
	else if(tamanho >= MAXFILA){
		printf("Fila cheia. Roteador sobrecarregado.\n");
		return;
	}
	else{//printf("aaaaaaaaaaa");
		nova = copyData(nova,buf);
	
		//filas[tamanho].mesg = nova;
		/*if(nova->tipo == 2 && tamanho > 0){ //desloca uma posicao para direita(DV tem prioridade)
			
			for(i = tamanho -1; i >=0; i--){
				//puts("eu");
				filas[i+1].tentativas = filas[i].tentativas;
				filas[i+1].timestamp = filas[i].timestamp;
				filas[i+1].mesg = filas[i].mesg;
				
			//	puts("eulll");
			}
			
			filas[0].id = filas[tamanho-1].id+1; //id incremental a partir de 1
			filas[0].timestamp = filas[tamanho-1].tentativas =0;
			filas[0].mesg = nova;
			
		}*/
		//else{
			filas[tamanho].mesg = nova;
			if(tamanho == 0){
				filas[tamanho].id = 1; //primeiro pacote tem id=1 
				filas[tamanho].timestamp = filas[tamanho].tentativas =0;
				
			
			}
			else{
				filas[tamanho].id = filas[tamanho-1].id+1; //id incremental a partir de 1
				filas[tamanho].timestamp = filas[tamanho].tentativas =0;
			}
			//printf("dd;;>%d ", filas[tamanho].mesg->destino);
		//}
		tamanho++;//tamanho global da fila
		//teste = tamanho;
	}
}

/**
 * function insere_fix - insere um dado que já existia na fila
 * 
 * Ao remover um dado da fila, esta função é chamada para inseri-lo novamente,
 * porém no final dela. Mantendo alguns atributos anteriores.
 * 
 * @param msg *buf - pacote a ser inserido na fila;
 * @param int save - quantidade de tentativas;
 * @param int saveID - identificador anterior;
 * @param time_t back - timestamp anterior;
 * 
 * Antes de remover o pacote da fila, é necessário salvar estes três atributos, e então passados como parâmetros.
 * 
 * 
 **/
void insere_fix(msg_t *nova, int save, int saveID, time_t back){

		
	//msg_t *nova = (msg_t *)malloc(sizeof(msg_t));
	
	if(!filas){
		printf("Impossivel inserir. Problema com a fila\n");
		return;
	}
	else if(tamanho >= MAXFILA){
		printf("Fila cheia. Roteador sobrecarregado\n");
		return;
	}
	else{
		//nova = copyData(nova,buf);
		
		filas[tamanho].mesg = nova;
		filas[tamanho].id = saveID;
		filas[tamanho].tentativas = save;
		filas[tamanho].timestamp = back;
		
		tamanho++;
	}
	
}


//remove o primeiro da fila
void remove_f(){
//	puts("onde");
	int i;
	//if(filas[0].mesg->tipo == 2)
		//if(filas[0].mesg->DV)
		//	free(filas[0].mesg->DV);
		//puts("onde1");
		//printf("como %d %d %d\n", teste,tamanho,filas[0].mesg->destino);
		//puts("aq2");
	free(filas[0].mesg); //libera espaço
	//puts("aq23");
	filas[0].mesg = NULL; 
	//puts("onde2");
	for(i = 1; i < tamanho; i++){
		filas[i-1].mesg = filas[i].mesg;
		//filas[i-1].tentativas = filas[i].tentativas;
		//filas[i-1].timestamp = filas[i].timestamp;
	}
	tamanho--;
	//puts("onde3");
}

/**
 * @function remove_fix - Esta função remove um pacote intermediário.
 * 
 * Pacotes que já foram confirmados tem prioridade na fila, assim para evitar que 
 * eles sejam reinviados é necessário removê-los da fila.
 * 
 **/

void remove_fix(int i){
	
	int j;
	//puts("oi");
	//printf("f:%d", tamanho);
	if(i == 0){ //caso for o primeiro, remove normalmente
		remove_f();
		
	}		
	else if( i == tamanho - 1){ //se for o ultimo
		//puts("aqui");
		
		//free(filas[0].mesg);
		//puts("aq");
		free(filas[i].mesg); 
		filas[i].mesg = NULL;
		tamanho--;
		
	}
	else{ //caso for algum intermediario
		//puts("xomo");
		for(j = i; j < tamanho-1; j++){
			//puts("aq1");
			free(filas[j].mesg);
			filas[j].mesg = filas[j+1].mesg;
			
		}
		tamanho--;
	}

	return;
}


int main(int argc, char *arq[]){
  
  pthread_t tids[4];
  
  if(!arq[1]){printf("Problema com o id do roteador\n");return 0;}
  int  routerId = atoi(arq[1]);//Lê o ID do roteador da linha de comando
  
	
  //Lê a tabela de roteamento com os caminhos mínimos já computados
  if(!(myConnect = leEnlaces(link_u, vertices =countIn(rout_u),routerId)))return 0;
    
  //Lê informações sobre este roteador ( id, porta, ip)   
  if(!(myRouter = leInfos(rout_u, routerId))) return 0;
  
 int i;
  
  for(i = 0; i < vertices; i++){
	  printf("%d %d %d\n", myConnect->idVizinho[i],myConnect->custo[i],myConnect->idImediato[i]);
	  
  }
 

  imprimeTabela();
  	  
  
  //cria 3 threads para controlar o roteador
 //printf("%d", MAXFILA);
  pthread_create(&tids[0], NULL, (void *)server, NULL); //server que recebe os pacotes
  pthread_create(&tids[1], NULL, (void *)SendDV, NULL);
 // pthread_create(&tids[2], NULL, (void *)enviarMsg, NULL); //client que manda as mensagens
  pthread_create(&tids[3], NULL, (void *)serverControl, NULL); //controle da fila e encaminhamentos dos pacotes
  
 /* pthread_create(&tids[1], NULL, (void *)SendDV, NULL);
  pthread_create(&tids[2], NULL, (void *)enviarMsg, NULL); //client que manda as mensagens
  pthread_create(&tids[3], NULL, (void *)serverControl, NULL); //controle da fila e encaminhamentos dos pacotes
  
  pthread_join(tids[2], NULL);
  pthread_cancel(tids[0]);
  pthread_cancel(tids[2]);
  pthread_cancel(tids[3]);
  pthread_join(tids[0], NULL);
  pthread_join(tids[1], NULL);
  pthread_join(tids[3], NULL);*/
  
  //pthread_join(tids[2], NULL);
  pthread_join(tids[0], NULL);
  pthread_cancel(tids[1]);
  pthread_cancel(tids[3]);
  pthread_join(tids[1], NULL);
  //pthread_cancel(tids[0]);
  pthread_cancel(tids[3]);
  //pthread_join(tids[0], NULL);
  pthread_join(tids[3], NULL);
  //pthread_join(tids[2], NULL);
  //pthread_join(tids[3], NULL);
 // SendDV();
  return 0;
}
