/*
 * ===========================================================
 * COMPILADORES - PROJETO Fase 1: Análise Léxica e Sintática
 * Linguagem: definida nos exemplos do Anexo 1
 * Implementação em C
 *
 * Integrantes do grupo:
 *   1. Beatriz Silva Nóbrega - 10435789
 *   2. Felipe Marques Leite Martha - 10437877
 *
 * Compilar:  gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador
 * Executar:  .\compilador.exe teste_ok.txt
 * ===========================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ===========================================================
// ESTRUTURAS DE DADOS (Baseado rigorosamente no PDF)
// ===========================================================

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ID,           // Identificadores (variáveis, funções)
    TOKEN_NUM_INT,      // Números inteiros (Ex: 42)
    TOKEN_NUM_FLOAT,    // Números reais (Ex: 3.14)
    TOKEN_OP_REL,       // Operadores relacionais
    TOKEN_KEYWORD       // Palavras reservadas
} TokenNome;

typedef enum {
    OP_LT,  // < (Less Than)
    OP_LE,  // <= (Less or Equal)
    OP_EQ,  // == (Equal)
    OP_GT,  // > (Greater Than)
    OP_GE  // >= (Greater or Equal)
} OpRelType;

typedef struct {
    TokenNome type; // Nome do token
    int line;       // Para tratamento de erros

    // valor do atributo
    union {
        int table_index;    // índice para Tabela de Símbolos
        int int_value;      // Valor literal convertido
        double float_value; // Valor literal convertido
        OpRelType op_code;  // operador relacional específico
    } attribute;
} Token;

// ===========================================================
// VARIÁVEIS GLOBAIS E TABELA DE SÍMBOLOS
// ===========================================================

// tabela de simbolos: guarda o TEXTO de tudo que o scanner le (ids, palavras reservadas,
// strings, delimitadores e operadores). o parser consulta esse texto pra saber o que veio.
// atencao: o indice daqui NAO e o numero do ID que aparece na saida, isso fica mais embaixo
char tabela_simbolos[1000][100];
int total_simbolos = 0;

// Procura o texto na tabela de simbolos; se nao existir, insere.
// Retorna o indice do texto na tabela (usado como atributo do token).
int buscar_ou_inserir_simbolo(const char *nome) {
    for (int i = 0; i < total_simbolos; i++) {
        if (strcmp(tabela_simbolos[i], nome) == 0) {
            return i; 
        }
    }
    strcpy(tabela_simbolos[total_simbolos], nome);
    total_simbolos++;
    return total_simbolos - 1;
}

// --- numeracao dos ids (correcao pedida pela professora) ---
// o problema: antes a gente imprimia o proprio indice da tabela_simbolos como numero do id.
// so que a tabela tambem guardava palavras reservadas, strings, delimitadores e operadores,
// entao o primeiro id da saida podia aparecer como 3, 7, 43... e nao seguia contagem sequencial.
// a solucao: so quem e TOKEN_ID ganha numero de id, e esse numero e uma contagem propria
// (0, 1, 2, ...) na ordem em que cada id diferente aparece no fonte.
// o resto continua na tabela_simbolos so pro parser ler o texto, mas nao gasta numero.
int numero_id[1000];   // numero_id[i] = numero de id do simbolo i (so vale se eh_id[i] for 1)
int eh_id[1000];       // eh_id[i] = 1 se o simbolo i ja virou um id numerado, 0 se ainda nao
int total_ids = 0;     // quantos ids diferentes ja foram numerados (e o proximo numero livre)

// recebe a posicao do simbolo na tabela_simbolos e devolve o numero de id dele
int obter_numero_id(int indice_simbolo) {
    // primeira vez que esse id aparece: ele pega o proximo numero livre e a contagem anda
    if (!eh_id[indice_simbolo]) {
        eh_id[indice_simbolo] = 1;
        numero_id[indice_simbolo] = total_ids++;
    }
    // se ja tinha numero, so devolve o mesmo (a mesma variavel sempre sai com o mesmo numero)
    return numero_id[indice_simbolo];
}

const char *palavras_reservadas[] = {
    "algoritmo", "var", "inicio", "fimalgoritmo",
    "se", "entao", "senao", "fimse",
    "para", "de", "ate", "passo", "faca", "fimpara",
    "enquanto", "fimenquanto",
    "procedimento", "fimprocedimento",
    "funcao", "fimfuncao", "retorne",
    "vetor", "inteiro", "real", "caractere", "logico",
    "verdadeiro", "falso",
    "leia", "escreva", "escreval",
    "E", "OU", "MOD"
};
const int total_palavras_reservadas = 34;
// Retorna 1 se a palavra esta na lista de palavras reservadas, senao 0.
int eh_palavra_reservada(const char *palavra) {
    for (int i = 0; i < total_palavras_reservadas; i++) {
        if (strcmp(palavra, palavras_reservadas[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// ===========================================================
// FASE 2: MINITABELA DE SIMBOLOS (lista encadeada de variaveis)
// ===========================================================
// cada no guarda uma variavel declarada: nome, endereco e tipo.
// o endereco e a ordem de declaracao: a 1a variavel tem endereco 0, a 2a tem 1, ...
// obs: tipo[7] nao cabe "inteiro" (8 bytes com o '\0') nem "caractere",
// por isso guardamos o tipo abreviado: "int", "real", "char" ou "bool".
typedef struct _TNo {
    char ID[16];
    int endereco;
    char tipo[7];
    struct _TNo *prox;
} TNo;

TNo *tabela_variaveis = NULL; // inicio da lista encadeada
int proximo_endereco = 0;     // endereco que a proxima variavel vai receber

// converte o nome do tipo da linguagem para a abreviacao que cabe em tipo[7]
const char *abreviar_tipo(const char *tipo) {
    if (strcmp(tipo, "inteiro") == 0) return "int";
    if (strcmp(tipo, "real") == 0) return "real";
    if (strcmp(tipo, "caractere") == 0) return "char";
    return "bool"; // logico
}

// cria um no novo e coloca no FIM da lista (assim a lista fica na ordem de declaracao)
void inserir_variavel(const char *nome, const char *tipo) {
    TNo *novo = (TNo *) malloc(sizeof(TNo));
    if (novo == NULL) {
        printf("Erro: memoria insuficiente\n");
        exit(1);
    }
    snprintf(novo->ID, sizeof(novo->ID), "%s", nome);
    snprintf(novo->tipo, sizeof(novo->tipo), "%s", abreviar_tipo(tipo));
    novo->endereco = proximo_endereco++;
    novo->prox = NULL;

    if (tabela_variaveis == NULL) {
        tabela_variaveis = novo;
    } else {
        TNo *atual = tabela_variaveis;
        while (atual->prox != NULL) {
            atual = atual->prox;
        }
        atual->prox = novo;
    }
}

// mostra a minitabela na tela e em saida.txt (para conferir se a insercao funcionou)
void imprimir_tabela_variaveis(FILE *destino) {
    fprintf(destino, "\nMINITABELA DE SIMBOLOS\n");
    fprintf(destino, "%-16s %-8s %s\n", "ID", "ENDERECO", "TIPO");
    for (TNo *atual = tabela_variaveis; atual != NULL; atual = atual->prox) {
        fprintf(destino, "%-16s %-8d %s\n", atual->ID, atual->endereco, atual->tipo);
    }
}

// libera todos os nos da lista no final do programa
void liberar_tabela_variaveis(void) {
    TNo *atual = tabela_variaveis;
    while (atual != NULL) {
        TNo *proximo = atual->prox;
        free(atual);
        atual = proximo;
    }
    tabela_variaveis = NULL;
}

// ===========================================================
// FASE 2: TABELA DE ROTINAS (procedimentos e funcoes)
// ===========================================================
// a minitabela TNo do enunciado guarda so variaveis. para verificar as chamadas de
// rotina (numero e tipo dos argumentos, retorno) as rotinas ficam numa lista separada.
#define MAX_PARAMETROS 10

typedef struct _TRotina {
    char ID[16];
    int eh_funcao;                             // 1 = funcao, 0 = procedimento
    char tipo_retorno[7];                      // so tem sentido para funcao
    int qtd_parametros;
    char tipos_parametros[MAX_PARAMETROS][7];  // tipo de cada parametro, na ordem
    struct _TRotina *prox;
} TRotina;

TRotina *tabela_rotinas = NULL; // inicio da lista de rotinas
TRotina *rotina_atual = NULL;   // rotina cujo cabecalho esta sendo lido agora

// cria a rotina no FIM da lista e marca ela como a rotina atual
void inserir_rotina(const char *nome, int eh_funcao) {
    TRotina *nova = (TRotina *) malloc(sizeof(TRotina));
    if (nova == NULL) {
        printf("Erro: memoria insuficiente\n");
        exit(1);
    }
    snprintf(nova->ID, sizeof(nova->ID), "%s", nome);
    nova->eh_funcao = eh_funcao;
    snprintf(nova->tipo_retorno, sizeof(nova->tipo_retorno), "-");
    nova->qtd_parametros = 0;
    nova->prox = NULL;

    if (tabela_rotinas == NULL) {
        tabela_rotinas = nova;
    } else {
        TRotina *atual = tabela_rotinas;
        while (atual->prox != NULL) {
            atual = atual->prox;
        }
        atual->prox = nova;
    }
    rotina_atual = nova;
}

// guarda o tipo de mais um parametro na rotina atual
void adicionar_parametro_rotina(const char *tipo) {
    if (rotina_atual != NULL && rotina_atual->qtd_parametros < MAX_PARAMETROS) {
        snprintf(rotina_atual->tipos_parametros[rotina_atual->qtd_parametros],
                 sizeof(rotina_atual->tipos_parametros[0]), "%s", abreviar_tipo(tipo));
        rotina_atual->qtd_parametros++;
    }
}

// guarda o tipo de retorno da funcao atual (o que vem depois do ':' no cabecalho)
void definir_retorno_rotina(const char *tipo) {
    if (rotina_atual != NULL) {
        snprintf(rotina_atual->tipo_retorno, sizeof(rotina_atual->tipo_retorno),
                 "%s", abreviar_tipo(tipo));
    }
}

// mostra a tabela de rotinas (para conferir se o registro funcionou)
void imprimir_tabela_rotinas(FILE *destino) {
    fprintf(destino, "\nTABELA DE ROTINAS\n");
    fprintf(destino, "%-16s %-13s %-8s %s\n", "ID", "CATEGORIA", "RETORNO", "PARAMETROS");
    for (TRotina *atual = tabela_rotinas; atual != NULL; atual = atual->prox) {
        fprintf(destino, "%-16s %-13s %-8s ", atual->ID,
                atual->eh_funcao ? "funcao" : "procedimento", atual->tipo_retorno);
        if (atual->qtd_parametros == 0) {
            fprintf(destino, "-");
        }
        for (int i = 0; i < atual->qtd_parametros; i++) {
            fprintf(destino, "%s%s", i > 0 ? ", " : "", atual->tipos_parametros[i]);
        }
        fprintf(destino, "\n");
    }
}

// libera todos os nos da lista de rotinas
void liberar_tabela_rotinas(void) {
    TRotina *atual = tabela_rotinas;
    while (atual != NULL) {
        TRotina *proxima = atual->prox;
        free(atual);
        atual = proxima;
    }
    tabela_rotinas = NULL;
    rotina_atual = NULL;
}
FILE *saida;
FILE *arquivo;
int linha_atual = 1;
int c_atual;

// ===========================================================
// FASE 2: ANALISE SEMANTICA
// ===========================================================
// mostra ERRO SEMÂNTICO com a linha (na tela e em saida.txt) e encerra o programa
void erro_semantico(int linha, const char *mensagem, const char *nome) {
    printf("ERRO SEMÂNTICO na linha %d: %s '%s'\n", linha, mensagem, nome);
    fprintf(saida, "ERRO SEMÂNTICO na linha %d: %s '%s'\n", linha, mensagem, nome);
    liberar_tabela_variaveis();
    liberar_tabela_rotinas();
    fclose(saida);
    fclose(arquivo);
    exit(1);
}

// procura uma variavel pelo nome na minitabela; devolve o no ou NULL se nao achar
TNo *procurar_variavel(const char *nome) {
    for (TNo *atual = tabela_variaveis; atual != NULL; atual = atual->prox) {
        if (strcmp(atual->ID, nome) == 0) {
            return atual;
        }
    }
    return NULL;
}

// procura uma rotina pelo nome na tabela de rotinas; devolve o no ou NULL se nao achar
TRotina *procurar_rotina(const char *nome) {
    for (TRotina *atual = tabela_rotinas; atual != NULL; atual = atual->prox) {
        if (strcmp(atual->ID, nome) == 0) {
            return atual;
        }
    }
    return NULL;
}

// regra semantica: o identificador deve ser unico em todo o codigo. vale para variaveis,
// parametros e rotinas: nenhum nome pode ser declarado duas vezes
void verificar_declaracao_unica(const char *nome, int linha) {
    if (procurar_variavel(nome) != NULL) {
        erro_semantico(linha, "variavel ja declarada:", nome);
    }
    if (procurar_rotina(nome) != NULL) {
        erro_semantico(linha, "nome ja usado por uma rotina:", nome);
    }
}

// recebe o nome (atributo_ID) do identificador e devolve o endereco da variavel na
// minitabela. se a variavel nao foi declarada, gera ERRO SEMÂNTICO e encerra.
// a linha e passada so para a mensagem de erro mostrar onde a variavel foi usada
int busca_tabela_simbolos(const char *atributo_ID, int linha) {
    TNo *variavel = procurar_variavel(atributo_ID);
    if (variavel == NULL) {
        erro_semantico(linha, "variavel nao declarada:", atributo_ID);
    }
    return variavel->endereco;
}

// procura a rotina chamada no codigo. se ela nao foi declarada, gera ERRO SEMÂNTICO.
// devolve o no da rotina (com parametros e retorno) para as verificacoes da chamada
TRotina *busca_rotina(const char *nome, int linha) {
    TRotina *rotina = procurar_rotina(nome);
    if (rotina == NULL) {
        erro_semantico(linha, "rotina nao declarada:", nome);
    }
    return rotina;
}

// ===========================================================
// FASE 2: VARIAVEIS COM VALOR ATRIBUIDO E NUNCA USADAS
// ===========================================================
// a struct TNo do enunciado nao tem campo para isso, entao guardamos em vetores separados.
// o indice e o ENDERECO da variavel (0, 1, 2, ...), que e unico para cada variavel
#define MAX_VARIAVEIS 1000
int variavel_atribuida[MAX_VARIAVEIS];   // 1 se a variavel recebeu valor em algum lugar
int variavel_usada[MAX_VARIAVEIS];       // 1 se o valor da variavel foi lido em algum lugar
int linha_atribuicao[MAX_VARIAVEIS];     // linha da primeira vez que recebeu valor

// marca que a variavel recebeu valor (<-, leia, para ou parametro)
void marcar_atribuicao(const char *nome, int linha) {
    int endereco = procurar_variavel(nome)->endereco;
    if (endereco < MAX_VARIAVEIS && !variavel_atribuida[endereco]) {
        variavel_atribuida[endereco] = 1;
        linha_atribuicao[endereco] = linha;
    }
}

// marca que o valor da variavel foi lido (aparece numa expressao ou controla um para)
void marcar_uso(const char *nome) {
    int endereco = procurar_variavel(nome)->endereco;
    if (endereco < MAX_VARIAVEIS) {
        variavel_usada[endereco] = 1;
    }
}

// regra semantica: toda variavel declarada e com valor atribuido deve ser usada.
// chamada no fim da analise, quando o programa inteiro ja foi lido
void verificar_variaveis_sem_uso(void) {
    for (TNo *atual = tabela_variaveis; atual != NULL; atual = atual->prox) {
        int endereco = atual->endereco;
        if (endereco < MAX_VARIAVEIS && variavel_atribuida[endereco] && !variavel_usada[endereco]) {
            erro_semantico(linha_atribuicao[endereco], "variavel recebe valor mas nunca e usada:", atual->ID);
        }
    }
}

// ===========================================================
// PARTE 1: ANALISADOR LÉXICO (SCANNER)
// ===========================================================
// Imprime o token na tela e em saida.txt no formato: linha# NOME | atributo
void imprimir_token(int linha, const char *nome_token, const char *atributo_str) {
    if (atributo_str[0] == '\0') {
        printf("%d# %s\n", linha, nome_token);
        fprintf(saida, "%d# %s\n", linha, nome_token);
    } else {
        printf("%d# %s | %s\n", linha, nome_token, atributo_str);
        fprintf(saida, "%d# %s | %s\n", linha, nome_token, atributo_str);
    }
}
// Analisador lexico: le caracteres do arquivo e devolve o proximo token.
// Ignora espacos, quebras de linha e comentarios.
// Em caractere invalido mostra ERRO LÉXICO e encerra o programa.
Token obterToken(void) {
    Token token;
    char lexema[100];
    char atributo_str[100];
    int pos;

    while (1) {
        if (c_atual == EOF) {
            token.type = TOKEN_EOF;
            token.line = linha_atual;
            imprimir_token(linha_atual, "TOKEN_EOF", "");
            return token;
        }

        // quebra de linha: e AQUI que o contador de linhas aumenta, e so aqui.
        // cada '\n' lido soma 1 em linha_atual. como os tokens guardam a linha em que
        // comecam, os erros (lexico e sintatico) mostram a linha certa.
        // o continue volta pro inicio do while pra ler o proximo caractere sem devolver token
        if (c_atual == '\n') {
            linha_atual++;
            c_atual = fgetc(arquivo);
            continue;
        }
        // espaco, tab e \r (do windows) nao viram token, so pulamos pro proximo caractere.
        // lembrando que o scanner le caractere por caractere: o espaco nao serve pra separar
        // lexema, quem decide onde o token acaba e o proprio padrao dele (ex: letras e digitos)
        if (c_atual == ' ' || c_atual == '\t' || c_atual == '\r') {
            c_atual = fgetc(arquivo);
            continue;
        }

        // identificador ou palavra reservada: letra seguida de letras, digitos ou _
        if (isalpha(c_atual)) {
            pos = 0;
            // vai juntando caractere por caractere ate achar algo que nao faz parte do nome
            while (isalpha(c_atual) || isdigit(c_atual) || c_atual == '_') {
                lexema[pos++] = (char) c_atual;
                c_atual = fgetc(arquivo);
            }
            lexema[pos] = '\0';
            token.line = linha_atual;

            // com a palavra inteira montada, primeiro confere se e reservada (se, para, inicio...).
            // so se nao for e que ela vira identificador
            if (eh_palavra_reservada(lexema)) {
                // palavra reservada: imprime o proprio texto, nao tem numero de id
                token.type = TOKEN_KEYWORD;
                token.attribute.table_index = buscar_ou_inserir_simbolo(lexema);
                imprimir_token(token.line, "TOKEN_KEYWORD", lexema);
            } else {
                // identificador de verdade: guarda o texto na tabela (pro parser) e imprime o
                // numero sequencial dele (0, 1, 2...) vindo de obter_numero_id,
                // e nao mais o indice da tabela
                token.type = TOKEN_ID;
                token.attribute.table_index = buscar_ou_inserir_simbolo(lexema);
                sprintf(atributo_str, "%d", obter_numero_id(token.attribute.table_index));
                imprimir_token(token.line, "TOKEN_ID", atributo_str);
            }
            return token;
        }

        // Numero inteiro ou real (digitos, e opcionalmente ponto e digitos)
        if (isdigit(c_atual)) {
            pos = 0;
            int eh_real = 0;
            while (isdigit(c_atual)) {
                lexema[pos++] = (char) c_atual;
                c_atual = fgetc(arquivo);
            }
            if (c_atual == '.') {
                int proximo = fgetc(arquivo);
                if (isdigit(proximo)) {
                    eh_real = 1;
                    lexema[pos++] = '.';
                    c_atual = proximo;
                    while (isdigit(c_atual)) {
                        lexema[pos++] = (char) c_atual;
                        c_atual = fgetc(arquivo);
                    }
                } else {
                    ungetc(proximo, arquivo);
                }
            }
            lexema[pos] = '\0';
            token.line = linha_atual;
            
            if (eh_real) {
                token.type = TOKEN_NUM_FLOAT;
                token.attribute.float_value = atof(lexema);
                sprintf(atributo_str, "%.2f", token.attribute.float_value);
                imprimir_token(token.line, "TOKEN_NUM_FLOAT", atributo_str);
            } else {
                token.type = TOKEN_NUM_INT;
                token.attribute.int_value = atoi(lexema);
                sprintf(atributo_str, "%d", token.attribute.int_value);
                imprimir_token(token.line, "TOKEN_NUM_INT", atributo_str);
            }
            return token;
        }

        // String entre aspas (em uma unica linha); sem aspas de fechamento gera ERRO LÉXICO
        if (c_atual == '"') {
            pos = 0;
            lexema[pos++] = '"';
            c_atual = fgetc(arquivo);
            while (c_atual != '"' && c_atual != EOF && c_atual != '\n' && pos < 98) {
                lexema[pos++] = (char) c_atual;
                c_atual = fgetc(arquivo);
            }
            if (c_atual != '"') {
                lexema[pos] = '\0';
                printf("ERRO LÉXICO na linha %d: string sem aspas de fechamento ou muito longa: %s\n", linha_atual, lexema);
                fprintf(saida, "ERRO LÉXICO na linha %d: string sem aspas de fechamento ou muito longa: %s\n", linha_atual, lexema);
                fclose(saida);
                fclose(arquivo);
                exit(1);
            }
            lexema[pos++] = '"';
            c_atual = fgetc(arquivo);
            lexema[pos] = '\0';
            
            // correcao da professora: string (inclusive o nome do algoritmo) NAO e id, e uma
            // cadeia de caracteres. antes saia como "TOKEN_ID (STRING)" com um indice qualquer.
            // a figura 2 so tem 6 tipos de token e nenhum e de string, entao ela sai com o mesmo
            // tipo dos simbolos (TOKEN_KEYWORD) e o atributo impresso e o proprio texto, aspas
            // incluidas. nao chama obter_numero_id, entao nao gasta numero de id.
            // o texto continua indo pra tabela_simbolos porque o parser precisa dele
            token.type = TOKEN_KEYWORD;
            token.line = linha_atual;
            token.attribute.table_index = buscar_ou_inserir_simbolo(lexema);
            imprimir_token(token.line, "TOKEN_KEYWORD", lexema);
            return token;
        }

        // Comentario // (ate o fim da linha) ou operador de divisao
        if (c_atual == '/') {
            int proximo = fgetc(arquivo);
            if (proximo == '/') {
                while (c_atual != '\n' && c_atual != EOF) {
                    c_atual = fgetc(arquivo);
                }
                continue; 
            } else {
                ungetc(proximo, arquivo);
                token.type = TOKEN_KEYWORD;
                token.line = linha_atual;
                token.attribute.table_index = buscar_ou_inserir_simbolo("/");
                imprimir_token(token.line, "TOKEN_KEYWORD", "/");
                c_atual = fgetc(arquivo);
                return token;
            }
        }

        // Operadores que comecam com <: <- (atribuicao), <=, <> e <
        if (c_atual == '<') {
            int proximo = fgetc(arquivo);
            token.line = linha_atual;
            if (proximo == '-') {
                token.type = TOKEN_KEYWORD;
                token.attribute.table_index = buscar_ou_inserir_simbolo("<-");
                imprimir_token(token.line, "TOKEN_KEYWORD", "<-");
                c_atual = fgetc(arquivo);
            } else if (proximo == '=') {
                token.type = TOKEN_OP_REL;
                token.attribute.op_code = OP_LE;
                imprimir_token(token.line, "TOKEN_OP_REL", "OP_LE");
                c_atual = fgetc(arquivo);
            } else if (proximo == '>') {
                token.type = TOKEN_KEYWORD;
                token.attribute.table_index = buscar_ou_inserir_simbolo("<>");
                imprimir_token(token.line, "TOKEN_KEYWORD", "<>");
                c_atual = fgetc(arquivo);
            } else {
                ungetc(proximo, arquivo);
                token.type = TOKEN_OP_REL;
                token.attribute.op_code = OP_LT;
                imprimir_token(token.line, "TOKEN_OP_REL", "OP_LT");
                c_atual = fgetc(arquivo);
            }
            return token;
        }

        if (c_atual == '>') {
            int proximo = fgetc(arquivo);
            token.line = linha_atual;
            if (proximo == '=') {
                token.type = TOKEN_OP_REL;
                token.attribute.op_code = OP_GE;
                imprimir_token(token.line, "TOKEN_OP_REL", "OP_GE");
                c_atual = fgetc(arquivo);
            } else {
                ungetc(proximo, arquivo);
                token.type = TOKEN_OP_REL;
                token.attribute.op_code = OP_GT;
                imprimir_token(token.line, "TOKEN_OP_REL", "OP_GT");
                c_atual = fgetc(arquivo);
            }
            return token;
        }

        if (c_atual == '=') {
            token.type = TOKEN_OP_REL;
            token.line = linha_atual;
            token.attribute.op_code = OP_EQ;
            imprimir_token(token.line, "TOKEN_OP_REL", "OP_EQ");
            c_atual = fgetc(arquivo);
            return token;
        }
        if (c_atual == '.') {
            int proximo = fgetc(arquivo);
            token.type = TOKEN_KEYWORD;
            token.line = linha_atual;
            if (proximo == '.') {
                token.attribute.table_index = buscar_ou_inserir_simbolo("..");
                imprimir_token(token.line, "TOKEN_KEYWORD", "..");
                c_atual = fgetc(arquivo);
            } else {
                ungetc(proximo, arquivo);
                token.attribute.table_index = buscar_ou_inserir_simbolo(".");
                imprimir_token(token.line, "TOKEN_KEYWORD", ".");
                c_atual = fgetc(arquivo);
            }
            return token;
        }

        // simbolos de um caractere: ( ) [ ] , : + - * e barra invertida.
        // delimitadores e operadores tambem nao sao ids: saem como TOKEN_KEYWORD com o proprio
        // simbolo como atributo (ex: "TOKEN_KEYWORD | ("), sem numero de id e sem gastar contagem.
        // se o caractere nao estiver nessa lista, cai no erro lexico logo abaixo
        const char *simbolos_validos = "()[],:+-*\\.";
        if (strchr(simbolos_validos, c_atual) != NULL) {
            token.type = TOKEN_KEYWORD;
            token.line = linha_atual;
            char simbolo_texto[2] = {(char) c_atual, '\0'};
            token.attribute.table_index = buscar_ou_inserir_simbolo(simbolo_texto);
            imprimir_token(token.line, "TOKEN_KEYWORD", simbolo_texto);
            c_atual = fgetc(arquivo);
            return token;
        }

        printf("ERRO LÉXICO na linha %d: caractere invalido '%c'\n", linha_atual, c_atual);
        fprintf(saida, "ERRO LÉXICO na linha %d: caractere invalido '%c'\n", linha_atual, c_atual);
        fclose(saida);
        fclose(arquivo);
        exit(1);
    }
}

// ===========================================================
// PARTE 2: ANALISADOR SINTÁTICO (PARSER)
// ===========================================================

// lookahead = o token "da vez": ja foi lido pelo scanner mas o parser ainda nao consumiu.
// e com ele que o parser decide qual regra da gramatica seguir (analise com 1 token a frente)
Token lookahead;
// chamada pelo parser: pede ao analisador lexico o proximo token e guarda em lookahead.
// e por aqui que scanner e parser andam juntos: o scanner so le o proximo token quando o
// parser pede, entao nao existe uma etapa separada que le o arquivo todo antes.
void nextToken(void) {
    lookahead = obterToken();
}
// Devolve o texto do token atual (lexema na tabela de simbolos) para o parser comparar.
const char* obter_texto_lookahead() {
    if (lookahead.type == TOKEN_EOF) return "EOF";
    if (lookahead.type == TOKEN_NUM_INT || lookahead.type == TOKEN_NUM_FLOAT) return "NUMERO";
    if (lookahead.type == TOKEN_OP_REL) return "OPERADOR_RELACIONAL";
    return tabela_simbolos[lookahead.attribute.table_index];
}

// como a string agora e um TOKEN_KEYWORD, o jeito de diferenciar ela das palavras reservadas
// e olhar o primeiro caractere do texto: se comeca com aspas, e string.
// (palavra reservada nunca comeca com aspas, entao nao tem confusao)
int eh_string(void) {
    return lookahead.type == TOKEN_KEYWORD && obter_texto_lookahead()[0] == '"';
}

// identificador de verdade: e TOKEN_ID. a checagem do primeiro caractere ficou de sobra
// agora que string nao e mais TOKEN_ID, mas nao atrapalha em nada
int eh_identificador(void) {
    return lookahead.type == TOKEN_ID && obter_texto_lookahead()[0] != '"';
}
// Mostra ERRO SINTÁTICO com o que era esperado, o token encontrado e a linha; encerra o programa.
void erroSintatico(const char *esperado) {
    printf("ERRO SINTÁTICO: esperado '%s', encontrado '%s' na linha %d\n", 
           esperado, obter_texto_lookahead(), lookahead.line);
    fprintf(saida, "ERRO SINTÁTICO: esperado '%s', encontrado '%s' na linha %d\n", 
           esperado, obter_texto_lookahead(), lookahead.line);
    fclose(saida);
    fclose(arquivo);
    exit(1);
}
// Consome o token atual se for do tipo esperado (identificador, numero...); senao erro sintatico.
void match(TokenNome tipo_esperado, const char *msg_erro) {
    int ok = (tipo_esperado == TOKEN_ID) ? eh_identificador()
                                         : (lookahead.type == tipo_esperado);
    if (ok) {
        nextToken();
    } else {
        erroSintatico(msg_erro);
    }
}
// Consome o token atual se o texto for igual ao esperado (palavra reservada ou simbolo); senao erro sintatico.
void match_keyword(const char *keyword_esperada) {
    if ((lookahead.type == TOKEN_KEYWORD || lookahead.type == TOKEN_ID) && 
        strcmp(obter_texto_lookahead(), keyword_esperada) == 0) {
        nextToken();
    } else {
        erroSintatico(keyword_esperada);
    }
}

void parse_comandos();
const char *parse_expressao();

// ===========================================================
// FASE 2: VERIFICACAO DE TIPOS NAS EXPRESSOES
// ===========================================================
// cada funcao de expressao (fator, termo, soma, relacional, E, OU) agora DEVOLVE o tipo
// do valor que ela reconheceu. os tipos sao os mesmos da minitabela:
// "int", "real", "char" e "bool". tipos incompativeis geram ERRO SEMÂNTICO.

// int e real sao os tipos numericos (podem ser misturados em contas)
int eh_numerico(const char *tipo) {
    return strcmp(tipo, "int") == 0 || strcmp(tipo, "real") == 0;
}

// erro padrao de tipos: mostra os dois tipos e o operador onde aconteceu
void erro_tipos(int linha, const char *tipo1, const char *tipo2, const char *operador) {
    char mensagem[100];
    snprintf(mensagem, sizeof(mensagem), "tipos incompativeis (%s e %s) no operador", tipo1, tipo2);
    erro_semantico(linha, mensagem, operador);
}

// texto do operador do token atual, para as mensagens de erro.
// os relacionais (TOKEN_OP_REL) nao ficam na tabela_simbolos, por isso o switch
const char *texto_operador() {
    if (lookahead.type == TOKEN_OP_REL) {
        switch (lookahead.attribute.op_code) {
            case OP_LT: return "<";
            case OP_LE: return "<=";
            case OP_EQ: return "=";
            case OP_GT: return ">";
            case OP_GE: return ">=";
        }
    }
    return obter_texto_lookahead();
}

// ===========================================================
// FASE 2: VERIFICACAO DAS CHAMADAS DE ROTINA
// ===========================================================
// confere se a chamada tem a mesma quantidade de argumentos que a rotina tem de parametros
void verificar_quantidade_argumentos(TRotina *rotina, int qtd_argumentos, int linha) {
    if (qtd_argumentos != rotina->qtd_parametros) {
        char mensagem[100];
        snprintf(mensagem, sizeof(mensagem),
                 "numero de argumentos incorreto (esperado %d, recebido %d) na chamada de",
                 rotina->qtd_parametros, qtd_argumentos);
        erro_semantico(linha, mensagem, rotina->ID);
    }
}

// le os argumentos "( expr, expr, ... )" de uma chamada e confere cada um com o parametro
// da mesma posicao: o tipo tem que ser igual (inteiro tambem pode ir para parametro real).
// no fim confere a quantidade de argumentos
void parse_argumentos(TRotina *rotina, int linha) {
    int qtd_argumentos = 0;
    match_keyword("(");
    while (1) {
        int linha_argumento = lookahead.line;
        const char *tipo_argumento = parse_expressao();
        if (qtd_argumentos < rotina->qtd_parametros) {
            const char *tipo_parametro = rotina->tipos_parametros[qtd_argumentos];
            int compativel = strcmp(tipo_parametro, tipo_argumento) == 0 ||
                             (strcmp(tipo_parametro, "real") == 0 && strcmp(tipo_argumento, "int") == 0);
            if (!compativel) {
                char mensagem[100];
                snprintf(mensagem, sizeof(mensagem),
                         "argumento %d incompativel (esperado %s, recebido %s) na chamada de",
                         qtd_argumentos + 1, tipo_parametro, tipo_argumento);
                erro_semantico(linha_argumento, mensagem, rotina->ID);
            }
        }
        qtd_argumentos++;
        if (strcmp(obter_texto_lookahead(), ",") != 0) {
            break;
        }
        match_keyword(",");
    }
    match_keyword(")");
    verificar_quantidade_argumentos(rotina, qtd_argumentos, linha);
}

// Fator: sinal + ou -, numero, string, identificador (com indice ou chamada), ( expressao ), verdadeiro ou falso.
// Devolve o tipo do fator.
const char *parse_fator() {
    if (strcmp(obter_texto_lookahead(), "-") == 0 || strcmp(obter_texto_lookahead(), "+") == 0) {
        const char *sinal = obter_texto_lookahead();
        int linha = lookahead.line;
        nextToken();
        const char *tipo = parse_fator();
        // sinal so faz sentido em numero
        if (!eh_numerico(tipo)) {
            char mensagem[100];
            snprintf(mensagem, sizeof(mensagem), "sinal usado com tipo nao numerico (%s) no operador", tipo);
            erro_semantico(linha, mensagem, sinal);
        }
        return tipo;
    }

    const char *tipo = "int";
    if (lookahead.type == TOKEN_NUM_INT) {
        nextToken();
        tipo = "int";
    } else if (lookahead.type == TOKEN_NUM_FLOAT) {
        nextToken();
        tipo = "real";
    } else if (eh_string()) {
        nextToken();
        tipo = "char";
    } else if (eh_identificador()) {
        const char *nome = obter_texto_lookahead();
        int linha = lookahead.line;
        nextToken();
        if (strcmp(obter_texto_lookahead(), "(") == 0 || procurar_rotina(nome) != NULL) {
            // chamada de rotina dentro de expressao (com parenteses, ou sem eles se a
            // rotina nao tiver parametros). so funcao pode estar aqui, porque devolve valor
            TRotina *rotina = busca_rotina(nome, linha);
            if (!rotina->eh_funcao) {
                erro_semantico(linha, "procedimento nao pode ser usado em expressao:", nome);
            }
            if (strcmp(obter_texto_lookahead(), "(") == 0) {
                parse_argumentos(rotina, linha);
            } else {
                verificar_quantidade_argumentos(rotina, 0, linha);
            }
            tipo = rotina->tipo_retorno;
        } else {
            // uso de variavel: precisa estar declarada, e o tipo vem da minitabela
            marcar_uso(nome);
            busca_tabela_simbolos(nome, linha);
            tipo = procurar_variavel(nome)->tipo;
            if (strcmp(obter_texto_lookahead(), "[") == 0) {
                match_keyword("[");
                int linha_indice = lookahead.line;
                const char *tipo_indice = parse_expressao();
                match_keyword("]");
                // o indice do vetor tem que ser inteiro
                if (strcmp(tipo_indice, "int") != 0) {
                    char mensagem[100];
                    snprintf(mensagem, sizeof(mensagem), "indice de vetor nao inteiro (%s) em", tipo_indice);
                    erro_semantico(linha_indice, mensagem, nome);
                }
            }
        }
    } else if (strcmp(obter_texto_lookahead(), "(") == 0) {
        match_keyword("(");
        tipo = parse_expressao();
        match_keyword(")");
    } else if (strcmp(obter_texto_lookahead(), "verdadeiro") == 0 || strcmp(obter_texto_lookahead(), "falso") == 0) {
        nextToken();
        tipo = "bool";
    } else {
        erroSintatico("Fator valido (Numero, Variavel, String ou Expressao)");
    }
    return tipo;
}
// Termo: Fator seguido de operadores * / barra invertida ou MOD (maior precedencia).
// * e / aceitam int e real; barra invertida (divisao inteira) e MOD so aceitam int.
const char *parse_termo() {
    const char *tipo = parse_fator();
    while (strcmp(obter_texto_lookahead(), "*") == 0 || 
           strcmp(obter_texto_lookahead(), "/") == 0 || 
           strcmp(obter_texto_lookahead(), "\\") == 0 ||
           strcmp(obter_texto_lookahead(), "MOD") == 0) {
        const char *operador = obter_texto_lookahead();
        int linha = lookahead.line;
        nextToken();
        const char *tipo_direita = parse_fator();

        if (strcmp(operador, "\\") == 0 || strcmp(operador, "MOD") == 0) {
            if (strcmp(tipo, "int") != 0 || strcmp(tipo_direita, "int") != 0) {
                erro_tipos(linha, tipo, tipo_direita, operador);
            }
            tipo = "int";
        } else {
            if (!eh_numerico(tipo) || !eh_numerico(tipo_direita)) {
                erro_tipos(linha, tipo, tipo_direita, operador);
            }
            // int com int da int; se um dos lados for real, o resultado e real
            tipo = (strcmp(tipo, "int") == 0 && strcmp(tipo_direita, "int") == 0) ? "int" : "real";
        }
    }
    return tipo;
}
// Soma: Termo com + e -. Numeros podem ser somados entre si e caractere + caractere junta os textos.
const char *parse_soma() {
    const char *tipo = parse_termo();
    while (strcmp(obter_texto_lookahead(), "+") == 0 || 
           strcmp(obter_texto_lookahead(), "-") == 0) {
        const char *operador = obter_texto_lookahead();
        int linha = lookahead.line;
        nextToken();
        const char *tipo_direita = parse_termo();

        if (eh_numerico(tipo) && eh_numerico(tipo_direita)) {
            tipo = (strcmp(tipo, "int") == 0 && strcmp(tipo_direita, "int") == 0) ? "int" : "real";
        } else if (strcmp(operador, "+") == 0 && strcmp(tipo, "char") == 0 &&
                   strcmp(tipo_direita, "char") == 0) {
            tipo = "char";
        } else {
            erro_tipos(linha, tipo, tipo_direita, operador);
        }
    }
    return tipo;
}
// Relacional: Soma comparada com outra Soma (< <= = > >= <>). O resultado e logico.
// So compara numero com numero, ou valores do mesmo tipo.
const char *parse_relacional() {
    const char *tipo = parse_soma();
    while (lookahead.type == TOKEN_OP_REL || strcmp(obter_texto_lookahead(), "<>") == 0) {
        const char *operador = texto_operador();
        int linha = lookahead.line;
        nextToken();
        const char *tipo_direita = parse_soma();

        if (!(eh_numerico(tipo) && eh_numerico(tipo_direita)) && strcmp(tipo, tipo_direita) != 0) {
            erro_tipos(linha, tipo, tipo_direita, operador);
        }
        tipo = "bool";
    }
    return tipo;
}
// Conjuncao: Relacional E Relacional E ... (os dois lados tem que ser logicos)
const char *parse_expressao_e() {
    const char *tipo = parse_relacional();
    while (strcmp(obter_texto_lookahead(), "E") == 0) {
        int linha = lookahead.line;
        nextToken();
        const char *tipo_direita = parse_relacional();
        if (strcmp(tipo, "bool") != 0 || strcmp(tipo_direita, "bool") != 0) {
            erro_tipos(linha, tipo, tipo_direita, "E");
        }
        tipo = "bool";
    }
    return tipo;
}
// Expressao: Conjuncao OU Conjuncao OU ... (menor precedencia; os dois lados tem que ser logicos)
// Ordem de precedencia (da maior para a menor): sinal, * / \ MOD, + -, relacionais, E, OU.
const char *parse_expressao() {
    const char *tipo = parse_expressao_e();
    while (strcmp(obter_texto_lookahead(), "OU") == 0) {
        int linha = lookahead.line;
        nextToken();
        const char *tipo_direita = parse_expressao_e();
        if (strcmp(tipo, "bool") != 0 || strcmp(tipo_direita, "bool") != 0) {
            erro_tipos(linha, tipo, tipo_direita, "OU");
        }
        tipo = "bool";
    }
    return tipo;
}
// ===========================================================
// FASE 2: VERIFICACAO DE TIPOS NOS COMANDOS
// ===========================================================
// atribuicao: o valor tem que ser do mesmo tipo da variavel.
// unica excecao: inteiro pode ser guardado em variavel real (o contrario nao)
void verificar_atribuicao(const char *tipo_variavel, const char *tipo_valor,
                          const char *nome, int linha) {
    int compativel = strcmp(tipo_variavel, tipo_valor) == 0 ||
                     (strcmp(tipo_variavel, "real") == 0 && strcmp(tipo_valor, "int") == 0);
    if (!compativel) {
        char mensagem[100];
        snprintf(mensagem, sizeof(mensagem), "atribuicao incompativel (%s <- %s) na variavel",
                 tipo_variavel, tipo_valor);
        erro_semantico(linha, mensagem, nome);
    }
}

// indice de vetor (no lado esquerdo do <- ou no leia) tem que ser inteiro
void verificar_indice(const char *tipo_indice, const char *nome, int linha) {
    if (strcmp(tipo_indice, "int") != 0) {
        char mensagem[100];
        snprintf(mensagem, sizeof(mensagem), "indice de vetor nao inteiro (%s) em", tipo_indice);
        erro_semantico(linha, mensagem, nome);
    }
}

// condicao do se e do enquanto tem que ser logica
void verificar_condicao(const char *tipo, const char *comando, int linha) {
    if (strcmp(tipo, "bool") != 0) {
        char mensagem[100];
        snprintf(mensagem, sizeof(mensagem), "condicao nao logica (%s) no comando", tipo);
        erro_semantico(linha, mensagem, comando);
    }
}

// valores do para (de, ate, passo) tem que ser inteiros
void verificar_valor_para(const char *tipo, const char *palavra, int linha) {
    if (strcmp(tipo, "int") != 0) {
        char mensagem[100];
        snprintf(mensagem, sizeof(mensagem), "valor nao inteiro (%s) no para, em", tipo);
        erro_semantico(linha, mensagem, palavra);
    }
}

// Comando que comeca com identificador: atribuicao (var <- expr ou vetor[i] <- expr) ou chamada de procedimento.
void parse_comando_id() {
    const char *nome = obter_texto_lookahead();
    int linha = lookahead.line;
    match(TOKEN_ID, "Identificador");

    if (strcmp(obter_texto_lookahead(), "[") == 0 ||
        strcmp(obter_texto_lookahead(), "<-") == 0) {
        // atribuicao: a variavel que recebe o valor precisa estar declarada
        marcar_atribuicao(nome, linha);
        busca_tabela_simbolos(nome, linha);
        const char *tipo_variavel = procurar_variavel(nome)->tipo;
        if (strcmp(obter_texto_lookahead(), "[") == 0) {
            match_keyword("[");
            int linha_indice = lookahead.line;
            const char *tipo_indice = parse_expressao();
            match_keyword("]");
            verificar_indice(tipo_indice, nome, linha_indice);
        }
        int linha_atribuicao = lookahead.line;
        match_keyword("<-");
        const char *tipo_valor = parse_expressao();
        verificar_atribuicao(tipo_variavel, tipo_valor, nome, linha_atribuicao);
    } else {
        // chamada de rotina como comando: precisa estar declarada e tem que ser
        // procedimento (funcao devolve valor, entao so pode aparecer em expressao)
        TRotina *rotina = busca_rotina(nome, linha);
        if (rotina->eh_funcao) {
            erro_semantico(linha, "funcao nao pode ser usada como procedimento:", nome);
        }
        if (strcmp(obter_texto_lookahead(), "(") == 0) {
            parse_argumentos(rotina, linha);
        } else {
            // nome sozinho: chamada sem argumentos
            verificar_quantidade_argumentos(rotina, 0, linha);
        }
    }
}
// Um comando: escreva/escreval, leia, se, enquanto, para, retorne, atribuicao ou chamada.
void parse_comando() {
    const char* texto_atual = obter_texto_lookahead();

    if (strcmp(texto_atual, "escreva") == 0 || strcmp(texto_atual, "escreval") == 0) {
        nextToken();
        match_keyword("(");
        parse_expressao();
        while (strcmp(obter_texto_lookahead(), ",") == 0) {
            match_keyword(",");
            parse_expressao();
        }
        match_keyword(")");
    } 
    else if (strcmp(texto_atual, "leia") == 0) {
        match_keyword("leia");
        match_keyword("(");
        const char *nome_leitura = obter_texto_lookahead();
        int linha_leitura = lookahead.line;
        match(TOKEN_ID, "Variavel para leitura");
        busca_tabela_simbolos(nome_leitura, linha_leitura);
        marcar_atribuicao(nome_leitura, linha_leitura);
        if (strcmp(obter_texto_lookahead(), "[") == 0) {
            match_keyword("[");
            int linha_indice = lookahead.line;
            const char *tipo_indice = parse_expressao();
            match_keyword("]");
            verificar_indice(tipo_indice, nome_leitura, linha_indice);
        }
        match_keyword(")");
    }
    else if (strcmp(texto_atual, "se") == 0) {
        match_keyword("se");
        match_keyword("(");
        int linha_condicao = lookahead.line;
        verificar_condicao(parse_expressao(), "se", linha_condicao);
        match_keyword(")");
        match_keyword("entao");
        parse_comandos();
        if (strcmp(obter_texto_lookahead(), "senao") == 0) {
            match_keyword("senao");
            parse_comandos();
        }
        match_keyword("fimse");
    }
    else if (strcmp(texto_atual, "enquanto") == 0) {
        match_keyword("enquanto");
        match_keyword("(");
        int linha_condicao = lookahead.line;
        verificar_condicao(parse_expressao(), "enquanto", linha_condicao);
        match_keyword(")");
        match_keyword("faca");
        parse_comandos();
        match_keyword("fimenquanto");
    }
    else if (strcmp(texto_atual, "para") == 0) {
        match_keyword("para");
        const char *nome_controle = obter_texto_lookahead();
        int linha_controle = lookahead.line;
        match(TOKEN_ID, "Variavel de controle do para");
        busca_tabela_simbolos(nome_controle, linha_controle);
        // o para da valor a variavel de controle e tambem le esse valor para comparar
        marcar_atribuicao(nome_controle, linha_controle);
        marcar_uso(nome_controle);
        // a variavel de controle conta de um em um (ou de passo em passo): tem que ser inteira
        if (strcmp(procurar_variavel(nome_controle)->tipo, "int") != 0) {
            erro_semantico(linha_controle, "variavel de controle do para nao e inteira:", nome_controle);
        }
        match_keyword("de");
        int linha_valor = lookahead.line;
        verificar_valor_para(parse_expressao(), "de", linha_valor);
        match_keyword("ate");
        linha_valor = lookahead.line;
        verificar_valor_para(parse_expressao(), "ate", linha_valor);
        if (strcmp(obter_texto_lookahead(), "passo") == 0) {
            match_keyword("passo");
            linha_valor = lookahead.line;
            verificar_valor_para(parse_expressao(), "passo", linha_valor);
        }
        match_keyword("faca");
        parse_comandos();
        match_keyword("fimpara");
    }
        else if (strcmp(texto_atual, "retorne") == 0) {
        int linha_retorno = lookahead.line;
        match_keyword("retorne");
        const char *tipo_valor = parse_expressao();
        // retorne so existe dentro de funcao (rotina_atual aponta para a rotina cujo corpo
        // esta sendo lido; fora das rotinas ela e NULL)
        if (rotina_atual == NULL || !rotina_atual->eh_funcao) {
            erro_semantico(linha_retorno, "comando usado fora de uma funcao:", "retorne");
        }
        // o valor retornado tem que combinar com o tipo declarado da funcao
        int compativel = strcmp(rotina_atual->tipo_retorno, tipo_valor) == 0 ||
                         (strcmp(rotina_atual->tipo_retorno, "real") == 0 && strcmp(tipo_valor, "int") == 0);
        if (!compativel) {
            char mensagem[100];
            snprintf(mensagem, sizeof(mensagem), "retorno incompativel (funcao %s, valor %s) em",
                     rotina_atual->tipo_retorno, tipo_valor);
            erro_semantico(linha_retorno, mensagem, rotina_atual->ID);
        }
    }
    else if (eh_identificador()) {
        parse_comando_id();
    }
    else {
        erroSintatico("Comando valido (escreva, leia, se, para, enquanto, atribuicao)");
    }
}
// Lista de comandos; para ao encontrar o marcador de fim do bloco (fimse, senao, fimpara, etc.)
void parse_comandos() {
    while (lookahead.type != TOKEN_EOF && 
           strcmp(obter_texto_lookahead(), "fimalgoritmo") != 0 &&
           strcmp(obter_texto_lookahead(), "fimse") != 0 &&
           strcmp(obter_texto_lookahead(), "senao") != 0 &&
           strcmp(obter_texto_lookahead(), "fimenquanto") != 0 &&
           strcmp(obter_texto_lookahead(), "fimpara") != 0 &&
           strcmp(obter_texto_lookahead(), "fimprocedimento") != 0 &&
           strcmp(obter_texto_lookahead(), "fimfuncao") != 0) {
        parse_comando();
    }
}
// Secao var: declaracoes "ids : tipo" ou "id : vetor[a..b] de tipo"
void parse_secao_var() {
    if (strcmp(obter_texto_lookahead(), "var") == 0) {
        match_keyword("var");
        
                    while (eh_identificador()) {
            // guarda os nomes da linha (ex: "i, resultado") porque o tipo so aparece
            // depois do ':'; quando o tipo for lido, todos sao inseridos na minitabela
            char nomes[50][16];
            int qtd_nomes = 0;

            snprintf(nomes[qtd_nomes++], 16, "%s", obter_texto_lookahead());
            match(TOKEN_ID, "Identificador de Variavel");
            while (strcmp(obter_texto_lookahead(), ",") == 0) {
                match_keyword(",");
                if (eh_identificador() && qtd_nomes < 50) {
                    snprintf(nomes[qtd_nomes++], 16, "%s", obter_texto_lookahead());
                }
                match(TOKEN_ID, "Identificador de Variavel");
            }
            match_keyword(":");
            
            if (strcmp(obter_texto_lookahead(), "vetor") == 0) {
                match_keyword("vetor");
                match_keyword("[");
                match(TOKEN_NUM_INT, "Indice Inicial do Vetor");
                match_keyword("..");
                match(TOKEN_NUM_INT, "Indice Final do Vetor");
                match_keyword("]");
                match_keyword("de");
            }
            
            const char* tipo = obter_texto_lookahead();
            if (strcmp(tipo, "inteiro") == 0 || strcmp(tipo, "real") == 0 || 
                strcmp(tipo, "caractere") == 0 || strcmp(tipo, "logico") == 0) {
                for (int k = 0; k < qtd_nomes; k++) {
                    verificar_declaracao_unica(nomes[k], lookahead.line);
                    inserir_variavel(nomes[k], tipo);
                }
                nextToken();
            } else {
                erroSintatico("Tipo de variavel valido (inteiro, real, caractere, logico)");
            }
        }
    }
}

// Aceita um tipo: inteiro, real, caractere ou logico
void parse_tipo_basico() {
    const char* tipo = obter_texto_lookahead();
    if (strcmp(tipo, "inteiro") == 0 || strcmp(tipo, "real") == 0 ||
        strcmp(tipo, "caractere") == 0 || strcmp(tipo, "logico") == 0) {
        nextToken();
    } else {
        erroSintatico("Tipo valido (inteiro, real, caractere, logico)");
    }
}
// Um parametro "nome : tipo". O parametro e guardado na minitabela como variavel,
// para que o corpo da rotina possa usa-lo.
void parse_parametro() {
    const char *nome = obter_texto_lookahead();
    int linha = lookahead.line;
    match(TOKEN_ID, "Nome do parametro");
    match_keyword(":");
    const char *tipo = obter_texto_lookahead();
    parse_tipo_basico();
    verificar_declaracao_unica(nome, linha);
    inserir_variavel(nome, tipo);
    adicionar_parametro_rotina(tipo);
    // o parametro recebe valor na chamada da rotina
    marcar_atribuicao(nome, linha);
}
// Lista de parametros entre parenteses: nome : tipo, nome : tipo
void parse_parametros() {
    match_keyword("(");
    if (strcmp(obter_texto_lookahead(), ")") != 0) {
        parse_parametro();
        while (strcmp(obter_texto_lookahead(), ",") == 0) {
            match_keyword(",");
            parse_parametro();
        }
    }
    match_keyword(")");
}
// Regra inicial da gramatica: algoritmo "nome" [procedimentos/funcoes] [var] inicio ... fimalgoritmo.
void parse_programa() {
    match_keyword("algoritmo");
    if (!eh_string()) {
        erroSintatico("Nome do Algoritmo (texto entre aspas)");
    }
    nextToken();
    
        while (strcmp(obter_texto_lookahead(), "procedimento") == 0 || strcmp(obter_texto_lookahead(), "funcao") == 0) {
        if (strcmp(obter_texto_lookahead(), "procedimento") == 0) {
            match_keyword("procedimento");
            const char *nome_procedimento = obter_texto_lookahead();
            int linha_procedimento = lookahead.line;
            match(TOKEN_ID, "Nome do Procedimento");
            verificar_declaracao_unica(nome_procedimento, linha_procedimento);
            inserir_rotina(nome_procedimento, 0);
            if (strcmp(obter_texto_lookahead(), "(") == 0) {
                parse_parametros();
            }
            match_keyword("inicio");
            parse_comandos();
            match_keyword("fimprocedimento");
            rotina_atual = NULL; // saiu do corpo do procedimento
        } else {
            match_keyword("funcao");
            const char *nome_funcao = obter_texto_lookahead();
            int linha_funcao = lookahead.line;
            match(TOKEN_ID, "Nome da Funcao");
            verificar_declaracao_unica(nome_funcao, linha_funcao);
            inserir_rotina(nome_funcao, 1);
            if (strcmp(obter_texto_lookahead(), "(") == 0) {
                parse_parametros();
            }
            match_keyword(":");
            definir_retorno_rotina(obter_texto_lookahead());
            parse_tipo_basico(); 
            match_keyword("inicio");
            parse_comandos();
            match_keyword("fimfuncao");
            rotina_atual = NULL; // saiu do corpo da funcao
        }
    }

    parse_secao_var();
    
    match_keyword("inicio");
    parse_comandos();
    match_keyword("fimalgoritmo");

    // Apos o fimalgoritmo nao pode sobrar nada: o proximo token tem que ser o EOF
    if (lookahead.type != TOKEN_EOF) {
        erroSintatico("fim do arquivo (EOF)");
    }
}

// ===========================================================
// PARTE 3: PROGRAMA PRINCIPAL
// ===========================================================
// Abre o arquivo fonte (nome recebido por linha de comando), cria saida.txt e dispara a analise
int main(int argc, char *argv[]) {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    if (argc < 2) {
        printf("Uso: %s <arquivo_fonte>\n", argv[0]);
        return 1;
    }

    arquivo = fopen(argv[1], "r");
    if (arquivo == NULL) {
        printf("Erro: nao foi possivel abrir o arquivo %s\n", argv[1]);
        return 1;
    }

    saida = fopen("saida.txt", "w");
    if (saida == NULL) {
        printf("Erro: nao foi possivel criar o arquivo de saida\n");
        fclose(arquivo);
        return 1;
    }

    // a compilacao (analise) comeca aqui no main: le o primeiro caractere do arquivo.
    // c_atual sempre guarda o caractere que o scanner ja leu mas ainda nao usou
    c_atual = fgetc(arquivo);

    // Ignora o BOM UTF-8 (bytes EF BB BF) que alguns editores gravam no inicio do arquivo
    if (c_atual == 0xEF) {
        if (fgetc(arquivo) == 0xBB && fgetc(arquivo) == 0xBF) {
            c_atual = fgetc(arquivo);
        } else {
            rewind(arquivo);
            c_atual = fgetc(arquivo);
        }
    }

    // pede o primeiro token pro scanner (vai pro lookahead). sem isso o parser comecaria
    // sem ter nada pra olhar
    nextToken();

    // dispara a validacao sintatica a partir da raiz da gramatica (regra Programa).
    // dai pra frente o parser vai chamando nextToken, que chama o scanner token por token
    parse_programa();
    
    // fase 2: so no fim do programa da para saber se uma variavel nunca foi usada
    verificar_variaveis_sem_uso();

    // Se chegou até aqui sem abortar (exit(1)), está tudo correto
    printf("\nSUCESSO: Analise lexica e sintatica concluidas sem erros!\n");
    fprintf(saida, "\nSUCESSO: Analise lexica e sintatica concluidas sem erros!\n");

    
    // fase 2: mostra a minitabela montada e depois libera a memoria da lista
    imprimir_tabela_variaveis(stdout);
    imprimir_tabela_variaveis(saida);
    liberar_tabela_variaveis();
    imprimir_tabela_rotinas(stdout);
    imprimir_tabela_rotinas(saida);
    liberar_tabela_rotinas();

    fclose(saida);
    fclose(arquivo);
    return 0;
}
