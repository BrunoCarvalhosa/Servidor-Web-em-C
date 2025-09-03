#include <io.h>
#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <windows.h>
//#include <locale.h>

#pragma comment(lib,"ws2_32.lib")
#define PORTA 80
#define TAMANHOMENSAGEM 10000
#define MAXSOCKETS 10
#define TAMANHOARQUIVO 255
#define TAMANHOCAMINHO 1024
//  mudar o caminho (pasta onde os arquivos estão)
#define TAMANHOCAMINHOLOCAL 1024
#define DIRETORIORAIZ "D:\\www\\"
#define ARQUIVOPADRAO "index.html"

const char *tiposmime[] =
{
    "text/html",
    "image/jpeg",
    "image/png",
    "image/gif",
    "text/css",
    "image/x-icon",
    "application/octet-stream",
};

SOCKET novoSocket[MAXSOCKETS];
int qtdSockets = 0;

int obterPalavra(char *frase, char *palavra, int i)
{
    int j=0;
    if (frase != NULL)
    {
        while(frase[i] != ' ')
        {
            palavra[j] = frase[i];
            i++;
            j++;
        }
        palavra[j] = '\0';
    }
    else
        palavra[0]='\0';
    return i+1;
}

//como é só o GET que é pra trabalhar, sempre vai ser da posicao 0 a 3
char *pegarGET(char *requisicao)
{
    static char get[4];

    if (strncmp(requisicao, "GET", 3) == 0)
    {
        strcpy(get, "GET");
    }
    else
    {
        strcpy(get, "");
    }

    return get;
}

void obterCaminho(char *requisicao, char *caminho, int i)
{
    obterPalavra(requisicao, caminho, 4);
}

char *obterFormatoArquivo(char *nomeArquivo) {
    int len = strlen(nomeArquivo);
    int i, j, k = 0;
    char *formato;

    // Percorre a string de trás para frente procurando o '.'
    for (i = len - 1; i >= 0; i--) {
        if (nomeArquivo[i] == '.') {
            // Calcula o tamanho da extensão e aloca memória para ela
            formato = (char *)malloc((len - i) * sizeof(char));

            // Copia os caracteres após o ponto para formato
            for (j = i + 1; j < len; j++) {
                formato[k] = nomeArquivo[j];
                k++;
            }
            formato[k] = '\0';
            return formato;
        }
    }

    return NULL;
}


void obterHorarioFormatado(char *ft) {
    SYSTEMTIME stUTC, stLocal;
    TIME_ZONE_INFORMATION tzInfo;

    // Arrays de abreviações de dias da semana e meses
    const char *diasSemana[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char *meses[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    // Obtém o horário atual em UTC
    GetSystemTime(&stUTC);

    // Converte o horário UTC para o fuso horário local
    GetTimeZoneInformation(&tzInfo);
    SystemTimeToTzSpecificLocalTime(&tzInfo, &stUTC, &stLocal);

    // Formata o horário no estilo desejado (ex: "Mon, 04 Sep 2024 13:25:36 GMT")
    sprintf(ft, "%s, %02d %s %04d %02d:%02d:%02d GMT",
            diasSemana[stUTC.wDayOfWeek],   // Dia da semana
            stUTC.wDay,                     // Dia do mês
            meses[stUTC.wMonth - 1],        // Mês (wMonth é de 1 a 12, então -1 para acessar o array)
            stUTC.wYear,                    // Ano
            stUTC.wHour,                    // Hora
            stUTC.wMinute,                  // Minuto
            stUTC.wSecond);                 // Segundo
}


int obterTipoMime(char *formatoArquivo)
{
    if (strcmp(formatoArquivo,"html")==0)
        return 0;
    if (strcmp(formatoArquivo,"jpg")==0)
        return 1;
    if (strcmp(formatoArquivo,"jpeg")==0)
        return 1;
    if (strcmp(formatoArquivo,"png")==0)
        return 2;
    if (strcmp(formatoArquivo,"gif")==0)
        return 3;
    if (strcmp(formatoArquivo,"css")==0)
        return 4;
    if (strcmp(formatoArquivo,"ico")==0)
        return 5;
    return 6;
}

void tratarRequisicao(int pos)
{
    int len, i, tamanhoArquivo;
    char requisicao[TAMANHOMENSAGEM];
    char cabecalho[TAMANHOMENSAGEM];
    char *metodo;
    char nomeArquivo[TAMANHOARQUIVO];
    char *formatoArquivo;
    int tipoMime;
    char caminho[TAMANHOCAMINHO];
    char caminhoLocal[TAMANHOCAMINHOLOCAL];
    char ft[80];
    char *objetoHttp;
    FILE *fp;

    requisicao[0]='\0';

    len=recv(novoSocket[pos],requisicao,TAMANHOMENSAGEM,0);
    if(len>0)
    {

        printf("\n\n***** REQUISICAO (%d caracteres) *****\n%s",len,requisicao);
        metodo = pegarGET(requisicao);
        printf("Metodo requisitado: %s",metodo);

        if (strncmp("GET",metodo,4) == 0)
        {

            obterCaminho(requisicao, caminho, i);
            printf("\nCaminho do arquivo: %s",caminho);

            if (strncmp("/",caminho,2) == 0){
                strcpy(nomeArquivo,ARQUIVOPADRAO);
            } else {
                strcpy(nomeArquivo,caminho+1);
            }


            printf("\nNome do arquivo: %s", nomeArquivo);

            formatoArquivo = obterFormatoArquivo(nomeArquivo);
            printf("\nFormato do arquivo: %s",formatoArquivo);

            tipoMime = obterTipoMime(formatoArquivo);
            printf("\nTipo mime: %s",tiposmime[tipoMime]);

            strcpy(caminhoLocal,DIRETORIORAIZ);
            strcat(caminhoLocal,nomeArquivo);
            printf("\nCaminho local: %s",caminhoLocal);

            obterHorarioFormatado(ft);

            fp = fopen(caminhoLocal,"r");
            if (fp)
            {
                fseek(fp, 0, SEEK_END);
                tamanhoArquivo = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                objetoHttp = malloc(tamanhoArquivo);
                if(objetoHttp)
                {
                    fread(objetoHttp, 1, tamanhoArquivo, fp);
                    printf("\n\n***** RESPOSTA 200 *****\n");
                    sprintf(cabecalho,"HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Length: %d\r\nContent-Type: %s; charset=UTF-8\r\n\r\n",ft,tamanhoArquivo,tiposmime[tipoMime]);
                }
                fclose(fp);
            }
            else
            {
                printf("\n\n***** RESPOSTA 404 *****\n");
                objetoHttp = (char *) malloc(sizeof(char)*1000);
                strcpy(objetoHttp,"<html><head><title>404 Não encontrado</title></head><body><h1>404 - Não encontrado</h1><p>a solicitação não pôde ser concluída porque o documento HTML não foi encontrado.</p></body></html>");
                tamanhoArquivo = strlen(objetoHttp);
                sprintf(cabecalho,"HTTP/1.1 404 Not Found\r\nDate: %s\r\nContent-Length: %d\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n",ft,tamanhoArquivo);
            }
        }
        else
        {
            printf("\n\n***** RESPOSTA 501 *****\n");
            objetoHttp = (char *) malloc(sizeof(char)*100);
            sprintf(objetoHttp,"<html><head><title>501 Not Implemented</title></head><body><h1>501 - Método não implementado</h1><p>O método %s não foi implementado.</p></body></html>",metodo);
            tamanhoArquivo = strlen(objetoHttp);
            sprintf(cabecalho,"HTTP/1.1 501 Not Implemented\r\nDate: %s\r\nContent-Length: %d\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n",ft,tamanhoArquivo);
        }

        send(novoSocket[pos], cabecalho, strlen(cabecalho), 0);
        send(novoSocket[pos], objetoHttp, tamanhoArquivo, 0);
        puts(cabecalho);
        puts(objetoHttp);

        free(objetoHttp);
    }
    closesocket(novoSocket[pos]);
    qtdSockets--;
}

int main(int argc, char *argv[])
{
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in servidor, cliente;
    int c, pos=0, proximaPos=0;

    //setlocale(LC_ALL, "Portuguese";
    system("color 02");

    //Inicializa a bilioteca
    WSAStartup(MAKEWORD(2,2),&wsa);

    //Inicializa o descritor do socket
    s = socket(AF_INET, SOCK_STREAM, 0 );

    //Preparar a estrutura sockaddr_in
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY;
    servidor.sin_port = htons(PORTA);

    bind(s,(struct sockaddr *)&servidor, sizeof(servidor));
    listen(s,3);

    printf("SERVIDOR DO BRUNO RODANDO\n");
    printf("aceitando conexao: ");


    c = sizeof(struct sockaddr_in);

    while(1)
    {
        novoSocket[pos] = accept(s, (struct sockaddr *)&cliente, &c);
        printf("\nConexao recebida");
        if (qtdSockets<MAXSOCKETS)
        {
            qtdSockets++;
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)tratarRequisicao, (void*) pos, 0, NULL);
            pos=++proximaPos;
            if (proximaPos == MAXSOCKETS)
                proximaPos = 0;
        }
        else
        {
            printf("\nNúmero máximo de sockets ativos\n");
            closesocket(novoSocket[pos]);
        }
    }

    closesocket(s);
    WSACleanup();

    return 0;
}

