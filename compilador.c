#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Nomes dos tipos de token que existem
typedef enum {
    TOKEN_EOF = 0,      // fim do arquivo
    TOKEN_ID,           // identificador (nome de variável, função...)
    TOKEN_NUM_INT,      // número inteiro
    TOKEN_NUM_FLOAT,    // número real
    TOKEN_OP_REL,       // operador relacional (=, <>, <, <=, >, >=)
    TOKEN_KEYWORD       // palavra reservada
} TokenNome;

// A "caixinha" que guarda um token completo
typedef struct {
    TokenNome type;   // qual é o tipo do token
    int line;          // em que linha do arquivo ele apareceu

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

int main(int argc, char *argv[]) {
    // Confere se o usuário passou o nome do arquivo
    if (argc < 2) {
        printf("Uso: %s <arquivo_fonte>\n", argv[0]);
        return 1;
    }

    // Abre o arquivo para leitura
    FILE *arquivo = fopen(argv[1], "r");
    if (arquivo == NULL) {
        printf("Erro: nao foi possivel abrir o arquivo %s\n", argv[1]);
        return 1;
    }

    printf("Arquivo aberto com sucesso!\n");


    saida = fopen("saida.txt", "w");
    if (saida == NULL) {
        printf("Erro: nao foi possivel criar o arquivo de saida\n");
        return 1;
    }

    int c;
    int linha = 1;
    char lexema[100]; // onde vamos guardar o pedaço atual
    int pos;

    c = fgetc(arquivo);
    while (c != EOF) {

        // Ignora espaços e conta quebras de linha
        if (c == '\n') {
            linha++;
            c = fgetc(arquivo);
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\r') {
            c = fgetc(arquivo);
            continue;
        }

        // Se comecou uma palavra (letra)
        if (isalpha(c)) {
            pos = 0;
            while (isalpha(c) || isdigit(c) || c == '_') {
                lexema[pos] = (char) c;
                pos++;
                c = fgetc(arquivo);
            }
                        lexema[pos] = '\0'; // termina a palavra

            if (eh_palavra_reservada(lexema)) {
                imprimir_token(linha, "PALAVRA_RESERVADA", lexema);
            } else {
                int indice = buscar_ou_inserir_simbolo(lexema);
                char texto_indice[10];
                sprintf(texto_indice, "%d", indice);
                imprimir_token(linha, "IDENTIFICADOR", texto_indice);
            }
            continue;
        }

        // Se comecou um numero (inteiro ou real)
        if (isdigit(c)) {
            pos = 0;
            int eh_real = 0;

            while (isdigit(c)) {
                lexema[pos] = (char) c;
                pos++;
                c = fgetc(arquivo);
            }

            // Se vier um ponto seguido de digito, e um numero real
            if (c == '.') {
                int proximo = fgetc(arquivo);
                if (isdigit(proximo)) {
                    eh_real = 1;
                    lexema[pos] = '.';
                    pos++;
                    c = proximo;
                    while (isdigit(c)) {
                        lexema[pos] = (char) c;
                        pos++;
                        c = fgetc(arquivo);
                    }
                } else {
                    // nao era numero real, devolve os dois caracteres lidos
                    ungetc(proximo, arquivo);
                }
            }

            lexema[pos] = '\0';
            if (eh_real) {
                imprimir_token(linha, "NUMERO_REAL", lexema);
            } else {
                imprimir_token(linha, "NUMERO_INT", lexema);
            }
            continue;
        }

        // Se comecou uma string (aspas)
        if (c == '"') {
            pos = 0;
            c = fgetc(arquivo); // pula a aspas de abertura
            while (c != '"' && c != EOF) {
                lexema[pos] = (char) c;
                pos++;
                c = fgetc(arquivo);
            }
            lexema[pos] = '\0';
            c = fgetc(arquivo); // pula a aspas de fechamento
            imprimir_token(linha, "STRING", lexema);
            continue;
        }

        // Caso especial: comeca com '/'  ->  pode ser comentario "//"
        if (c == '/') {
            int proximo = fgetc(arquivo);
            if (proximo == '/') {
                // Comentario: ignora tudo ate o fim da linha
                while (c != '\n' && c != EOF) {
                    c = fgetc(arquivo);
                }
                continue; // volta ao topo do while, sem consumir o '\n' de novo
            } else {
                ungetc(proximo, arquivo);
                imprimir_token(linha, "SIMBOLO", "/");
                c = fgetc(arquivo);
                continue;
            }
        }

        // Caso especial: comeca com '<'  ->  pode ser <-, <=, <> ou < sozinho
        if (c == '<') {
            int proximo = fgetc(arquivo);
            if (proximo == '-') {
                imprimir_token(linha, "OPERADOR_ATRIBUICAO", "<-");
                c = fgetc(arquivo);
            } else if (proximo == '=') {
                imprimir_token(linha, "OP_REL", "<=");
                c = fgetc(arquivo);
            } else if (proximo == '>') {
                imprimir_token(linha, "OP_REL", "<>");
                c = fgetc(arquivo);
            } else {
                ungetc(proximo, arquivo);
                imprimir_token(linha, "OP_REL", "<");
                c = fgetc(arquivo);
            }
            continue;
        }
        // Caso especial: comeca com '>'  ->  pode ser >= ou > sozinho
        if (c == '>') {
            int proximo = fgetc(arquivo);
            if (proximo == '=') {
                imprimir_token(linha, "OP_REL", ">=");
                c = fgetc(arquivo);
            } else {
                ungetc(proximo, arquivo);
                imprimir_token(linha, "OP_REL", ">");
                c = fgetc(arquivo);
            }
            continue;
        }

        // Simbolos validos da linguagem
        const char *simbolos_validos = "()[],:+-*/\\<>=.";

        if (strchr(simbolos_validos, c) != NULL) {
            char simbolo_texto[2] = {(char) c, '\0'};
            imprimir_token(linha, "SIMBOLO", simbolo_texto);
            c = fgetc(arquivo);
        } else {
            // Caractere invalido: erro lexico
            printf("ERRO LEXICO na linha %d: caractere invalido '%c'\n", linha, c);
            fprintf(saida, "ERRO LEXICO na linha %d: caractere invalido '%c'\n", linha, c);
            fclose(saida);
            fclose(arquivo);
            return 1;
        }

    }

    printf("--- Fim do arquivo. Total de linhas: %d ---\n", linha);

    fclose(saida);
    fclose(arquivo);
    return 0;
}