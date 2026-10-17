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

FILE *saida;
FILE *arquivo;
int linha_atual = 1;
int c_atual;

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
void parse_expressao();
// Fator: sinal + ou -, numero, string, identificador (com indice ou chamada), ( expressao ), verdadeiro ou falso.
void parse_fator() {
    if (strcmp(obter_texto_lookahead(), "-") == 0 || strcmp(obter_texto_lookahead(), "+") == 0) {
        nextToken();
        parse_fator();
        return;
    }

    if (lookahead.type == TOKEN_NUM_INT || lookahead.type == TOKEN_NUM_FLOAT) {
        nextToken();
    } else if (eh_string()) {
        nextToken();
    } else if (eh_identificador()) {
        nextToken();
        if (strcmp(obter_texto_lookahead(), "[") == 0) {
            match_keyword("[");
            parse_expressao();
            match_keyword("]");
        } else if (strcmp(obter_texto_lookahead(), "(") == 0) {
            match_keyword("(");
            parse_expressao();
            while (strcmp(obter_texto_lookahead(), ",") == 0) {
                match_keyword(",");
                parse_expressao();
            }
            match_keyword(")");
        }
    } else if (strcmp(obter_texto_lookahead(), "(") == 0) {
        match_keyword("(");
        parse_expressao();
        match_keyword(")");
    } else if (strcmp(obter_texto_lookahead(), "verdadeiro") == 0 || strcmp(obter_texto_lookahead(), "falso") == 0) {
        nextToken();
    } else {
        erroSintatico("Fator valido (Numero, Variavel, String ou Expressao)");
    }
}
// Termo: Fator seguido de operadores * / barra invertida ou MOD (maior precedencia).
void parse_termo() {
    parse_fator();
    while (strcmp(obter_texto_lookahead(), "*") == 0 || 
           strcmp(obter_texto_lookahead(), "/") == 0 || 
           strcmp(obter_texto_lookahead(), "\\") == 0 ||
           strcmp(obter_texto_lookahead(), "MOD") == 0) {
        nextToken();
        parse_fator();
    }
}
// Expressao: Termo com + e -, seguido opcionalmente de operador relacional ou E/OU e outra expressao.
void parse_expressao() {
    parse_termo();
    while (strcmp(obter_texto_lookahead(), "+") == 0 || 
           strcmp(obter_texto_lookahead(), "-") == 0) {
        nextToken();
        parse_termo();
    }
    
    if (lookahead.type == TOKEN_OP_REL ||
        strcmp(obter_texto_lookahead(), "<>") == 0 || 
        strcmp(obter_texto_lookahead(), "E") == 0 || 
        strcmp(obter_texto_lookahead(), "OU") == 0) {
        nextToken();
        parse_expressao();
    }
}
// Comando que comeca com identificador: atribuicao (var <- expr ou vetor[i] <- expr) ou chamada de procedimento.
void parse_comando_id() {
    match(TOKEN_ID, "Identificador");

    if (strcmp(obter_texto_lookahead(), "(") == 0) {
        match_keyword("(");
        parse_expressao();
        while (strcmp(obter_texto_lookahead(), ",") == 0) {
            match_keyword(",");
            parse_expressao();
        }
        match_keyword(")");
    } else if (strcmp(obter_texto_lookahead(), "[") == 0 ||
               strcmp(obter_texto_lookahead(), "<-") == 0) {
        if (strcmp(obter_texto_lookahead(), "[") == 0) {
            match_keyword("[");
            parse_expressao();
            match_keyword("]");
        }
        match_keyword("<-");
        parse_expressao();
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
        match(TOKEN_ID, "Variavel para leitura");
        if (strcmp(obter_texto_lookahead(), "[") == 0) {
            match_keyword("[");
            parse_expressao();
            match_keyword("]");
        }
        match_keyword(")");
    }
    else if (strcmp(texto_atual, "se") == 0) {
        match_keyword("se");
        match_keyword("(");
        parse_expressao();
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
        parse_expressao();
        match_keyword(")");
        match_keyword("faca");
        parse_comandos();
        match_keyword("fimenquanto");
    }
    else if (strcmp(texto_atual, "para") == 0) {
        match_keyword("para");
        match(TOKEN_ID, "Variavel de controle do para");
        match_keyword("de");
        parse_expressao();
        match_keyword("ate");
        parse_expressao();
        if (strcmp(obter_texto_lookahead(), "passo") == 0) {
            match_keyword("passo");
            parse_expressao();
        }
        match_keyword("faca");
        parse_comandos();
        match_keyword("fimpara");
    }
    else if (strcmp(texto_atual, "retorne") == 0) {
        match_keyword("retorne");
        parse_expressao();
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
            match(TOKEN_ID, "Identificador de Variavel");
            while (strcmp(obter_texto_lookahead(), ",") == 0) {
                match_keyword(",");
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
// Lista de parametros entre parenteses: nome : tipo, nome : tipo
void parse_parametros() {
    match_keyword("(");
    if (strcmp(obter_texto_lookahead(), ")") != 0) {
        match(TOKEN_ID, "Nome do parametro");
        match_keyword(":");
        parse_tipo_basico();
        while (strcmp(obter_texto_lookahead(), ",") == 0) {
            match_keyword(",");
            match(TOKEN_ID, "Nome do parametro");
            match_keyword(":");
            parse_tipo_basico();
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
            match(TOKEN_ID, "Nome do Procedimento");
            if (strcmp(obter_texto_lookahead(), "(") == 0) {
                parse_parametros();
            }
            match_keyword("inicio");
            parse_comandos();
            match_keyword("fimprocedimento");
        } else {
            match_keyword("funcao");
            match(TOKEN_ID, "Nome da Funcao");
            if (strcmp(obter_texto_lookahead(), "(") == 0) {
                parse_parametros();
            }
            match_keyword(":");
            parse_tipo_basico(); 
            match_keyword("inicio");
            parse_comandos();
            match_keyword("fimfuncao");
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

    // Se chegou até aqui sem abortar (exit(1)), está tudo correto
    printf("\nSUCESSO: Analise lexica e sintatica concluidas sem erros!\n");
    fprintf(saida, "\nSUCESSO: Analise lexica e sintatica concluidas sem erros!\n");

    fclose(saida);
    fclose(arquivo);
    return 0;
}
