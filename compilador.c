#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Nomes dos tipos de token que existem
typedef enum {
    TOKEN_EOF = 0,       // fim do arquivo
    TOKEN_ID,            // identificador (nome de variável, função...)
    TOKEN_NUM_INT,       // número inteiro
    TOKEN_NUM_FLOAT,     // número real
    TOKEN_OP_REL,        // operador relacional (=, <>, <, <=, >, >=)
    TOKEN_KEYWORD,       // palavra reservada
    TOKEN_STRING,        // texto entre aspas
    TOKEN_SIMBOLO,       // ( ) [ ] , : + - * / \ .
    TOKEN_ATRIBUICAO     // <-
} TokenNome;

// A "caixinha" que guarda um token completo
typedef struct {
    TokenNome type;    // qual é o tipo do token
    int line;          // em que linha do arquivo ele apareceu
    char texto[100];   // o texto do token (palavra, numero, simbolo...)

    union {
        int table_index;    // se for ID: posição na tabela de nomes
        int int_value;      // se for número inteiro: o valor
        double float_value; // se for número real: o valor
    } attribute;
} Token;

// Lista de todas as palavras reservadas do MiniVisualg (baseado no Anexo I)
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

// Funcao que confere se uma palavra e reservada
// Devolve 1 (verdadeiro) se for, 0 (falso) se nao for
int eh_palavra_reservada(const char *palavra) {
    for (int i = 0; i < total_palavras_reservadas; i++) {
        if (strcmp(palavra, palavras_reservadas[i]) == 0) {
            return 1;
        }
    }
    return 0;
}
// Tabela de simbolos: guarda os identificadores encontrados, sem repetir
char tabela_simbolos[100][100]; // ate 100 identificadores, cada um com ate 100 letras
int total_simbolos = 0;

// Procura o identificador na tabela. Se nao existir, adiciona.
// Devolve a posicao (indice) onde ele esta.
int buscar_ou_inserir_simbolo(const char *nome) {
    for (int i = 0; i < total_simbolos; i++) {
        if (strcmp(tabela_simbolos[i], nome) == 0) {
            return i; // ja existe, devolve a posicao
        }
    }
    // nao existe, adiciona no fim
    strcpy(tabela_simbolos[total_simbolos], nome);
    total_simbolos++;
    return total_simbolos - 1;
}
// Ponteiro global para o arquivo de saida
FILE *saida;

// Imprime um token no formato pedido, na tela E no arquivo de saida
void imprimir_token(int linha, const char *nome_token, const char *atributo) {
    if (atributo[0] == '\0') {
        // sem atributo (ex: palavras reservadas e simbolos)
        printf("%d# %s\n", linha, nome_token);
        fprintf(saida, "%d# %s\n", linha, nome_token);
    } else {
        printf("%d# %s | %s\n", linha, nome_token, atributo);
        fprintf(saida, "%d# %s | %s\n", linha, nome_token, atributo);
    }
}

FILE *arquivo;
int linha_atual = 1;
int c_atual;

Token obterToken(void) {
    Token token;
    char lexema[100];
    int pos;

    while (1) {
        if (c_atual == EOF) {
            token.type = TOKEN_EOF;
            token.line = linha_atual;
            strcpy(token.texto, "EOF");
            return token;
        }

        if (c_atual == '\n') {
            linha_atual++;
            c_atual = fgetc(arquivo);
            continue;
        }
        if (c_atual == ' ' || c_atual == '\t' || c_atual == '\r') {
            c_atual = fgetc(arquivo);
            continue;
        }

        if (isalpha(c_atual)) {
            pos = 0;
            while (isalpha(c_atual) || isdigit(c_atual) || c_atual == '_') {
                lexema[pos] = (char) c_atual;
                pos++;
                c_atual = fgetc(arquivo);
            }
            lexema[pos] = '\0';
            token.line = linha_atual;

            if (eh_palavra_reservada(lexema)) {
                token.type = TOKEN_KEYWORD;
                strcpy(token.texto, lexema);
                imprimir_token(token.line, "PALAVRA_RESERVADA", lexema);
            } else {
                token.type = TOKEN_ID;
                int indice = buscar_ou_inserir_simbolo(lexema);
                token.attribute.table_index = indice;
                sprintf(token.texto, "%d", indice);
                imprimir_token(token.line, "IDENTIFICADOR", token.texto);
            }
            return token;
        }

        if (isdigit(c_atual)) {
            pos = 0;
            int eh_real = 0;
            while (isdigit(c_atual)) {
                lexema[pos] = (char) c_atual;
                pos++;
                c_atual = fgetc(arquivo);
            }
            if (c_atual == '.') {
                int proximo = fgetc(arquivo);
                if (isdigit(proximo)) {
                    eh_real = 1;
                    lexema[pos] = '.';
                    pos++;
                    c_atual = proximo;
                    while (isdigit(c_atual)) {
                        lexema[pos] = (char) c_atual;
                        pos++;
                        c_atual = fgetc(arquivo);
                    }
                } else {
                    ungetc(proximo, arquivo);
                }
            }
            lexema[pos] = '\0';
            token.line = linha_atual;
            strcpy(token.texto, lexema);
            if (eh_real) {
                token.type = TOKEN_NUM_FLOAT;
                token.attribute.float_value = atof(lexema);
                imprimir_token(token.line, "NUMERO_REAL", lexema);
            } else {
                token.type = TOKEN_NUM_INT;
                token.attribute.int_value = atoi(lexema);
                imprimir_token(token.line, "NUMERO_INT", lexema);
            }
            return token;
        }

        if (c_atual == '"') {
            pos = 0;
            c_atual = fgetc(arquivo);
            while (c_atual != '"' && c_atual != EOF) {
                lexema[pos] = (char) c_atual;
                pos++;
                c_atual = fgetc(arquivo);
            }
            lexema[pos] = '\0';
            c_atual = fgetc(arquivo);
            token.type = TOKEN_STRING;
            token.line = linha_atual;
            strcpy(token.texto, lexema);
            imprimir_token(token.line, "STRING", lexema);
            return token;
        }

        if (c_atual == '/') {
            int proximo = fgetc(arquivo);
            if (proximo == '/') {
                while (c_atual != '\n' && c_atual != EOF) {
                    c_atual = fgetc(arquivo);
                }
                continue;
            } else {
                ungetc(proximo, arquivo);
                token.type = TOKEN_SIMBOLO;
                token.line = linha_atual;
                strcpy(token.texto, "/");
                imprimir_token(token.line, "SIMBOLO", "/");
                c_atual = fgetc(arquivo);
                return token;
            }
        }

        if (c_atual == '<') {
            int proximo = fgetc(arquivo);
            token.line = linha_atual;
            if (proximo == '-') {
                token.type = TOKEN_ATRIBUICAO;
                strcpy(token.texto, "<-");
                imprimir_token(token.line, "OPERADOR_ATRIBUICAO", "<-");
                c_atual = fgetc(arquivo);
            } else if (proximo == '=') {
                token.type = TOKEN_OP_REL;
                strcpy(token.texto, "<=");
                imprimir_token(token.line, "OP_REL", "<=");
                c_atual = fgetc(arquivo);
            } else if (proximo == '>') {
                token.type = TOKEN_OP_REL;
                strcpy(token.texto, "<>");
                imprimir_token(token.line, "OP_REL", "<>");
                c_atual = fgetc(arquivo);
            } else {
                ungetc(proximo, arquivo);
                token.type = TOKEN_OP_REL;
                strcpy(token.texto, "<");
                imprimir_token(token.line, "OP_REL", "<");
                c_atual = fgetc(arquivo);
            }
            return token;
        }

        if (c_atual == '>') {
            int proximo = fgetc(arquivo);
            token.line = linha_atual;
            if (proximo == '=') {
                token.type = TOKEN_OP_REL;
                strcpy(token.texto, ">=");
                imprimir_token(token.line, "OP_REL", ">=");
                c_atual = fgetc(arquivo);
            } else {
                ungetc(proximo, arquivo);
                token.type = TOKEN_OP_REL;
                strcpy(token.texto, ">");
                imprimir_token(token.line, "OP_REL", ">");
                c_atual = fgetc(arquivo);
            }
            return token;
        }

        if (c_atual == '=') {
            token.type = TOKEN_OP_REL;
            token.line = linha_atual;
            strcpy(token.texto, "=");
            imprimir_token(token.line, "OP_REL", "=");
            c_atual = fgetc(arquivo);
            return token;
        }

        const char *simbolos_validos = "()[],:+-*\\.";
        if (strchr(simbolos_validos, c_atual) != NULL) {
            token.type = TOKEN_SIMBOLO;
            token.line = linha_atual;
            char simbolo_texto[2] = {(char) c_atual, '\0'};
            strcpy(token.texto, simbolo_texto);
            imprimir_token(token.line, "SIMBOLO", simbolo_texto);
            c_atual = fgetc(arquivo);
            return token;
        }

        printf("ERRO LEXICO na linha %d: caractere invalido '%c'\n", linha_atual, c_atual);
        fprintf(saida, "ERRO LEXICO na linha %d: caractere invalido '%c'\n", linha_atual, c_atual);
        fclose(saida);
        fclose(arquivo);
        exit(1);
    }
}

int main(int argc, char *argv[]) {
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
        return 1;
    }

    printf("Arquivo aberto com sucesso!\n");

    c_atual = fgetc(arquivo);

    Token t = obterToken();
    while (t.type != TOKEN_EOF) {
        t = obterToken();
    }

    printf("--- Fim do arquivo ---\n");

    fclose(saida);
    fclose(arquivo);
    return 0;
}