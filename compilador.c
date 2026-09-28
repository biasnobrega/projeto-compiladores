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
    OP_GE   // >= (Greater or Equal)
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
    } Token;
} attribute;

// ===========================================================
// VARIÁVEIS GLOBAIS E TABELA DE SÍMBOLOS
// ===========================================================

char tabela_simbolos[1000][100]; 
int total_simbolos = 0;

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

void imprimir_token(int linha, const char *nome_token, const char *atributo_str) {
    if (atributo_str[0] == '\0') {
        printf("%d# %s\n", linha, nome_token);
        fprintf(saida, "%d# %s\n", linha, nome_token);
    } else {
        printf("%d# %s | %s\n", linha, nome_token, atributo_str);
        fprintf(saida, "%d# %s | %s\n", linha, nome_token, atributo_str);
    }
}

attribute obterToken(void) {
    attribute token;
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
                lexema[pos++] = (char) c_atual;
                c_atual = fgetc(arquivo);
            }
            lexema[pos] = '\0';
            token.line = linha_atual;

            if (eh_palavra_reservada(lexema)) {
                token.type = TOKEN_KEYWORD;
                token.Token.table_index = buscar_ou_inserir_simbolo(lexema);
                imprimir_token(token.line, "TOKEN_KEYWORD", lexema);
            } else {
                token.type = TOKEN_ID;
                token.Token.table_index = buscar_ou_inserir_simbolo(lexema);
                sprintf(atributo_str, "%d", token.Token.table_index);
                imprimir_token(token.line, "TOKEN_ID", atributo_str);
            }
            return token;
        }

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
                token.Token.float_value = atof(lexema);
                sprintf(atributo_str, "%.2f", token.Token.float_value);
                imprimir_token(token.line, "TOKEN_NUM_FLOAT", atributo_str);
            } else {
                token.type = TOKEN_NUM_INT;
                token.Token.int_value = atoi(lexema);
                sprintf(atributo_str, "%d", token.Token.int_value);
                imprimir_token(token.line, "TOKEN_NUM_INT", atributo_str);
            }
            return token;
        }

        if (c_atual == '"') {
            pos = 0;
            lexema[pos++] = '"';
            c_atual = fgetc(arquivo);
            while (c_atual != '"' && c_atual != EOF) {
                lexema[pos++] = (char) c_atual;
                c_atual = fgetc(arquivo);
            }
            if(c_atual == '"') {
                lexema[pos++] = '"';
                c_atual = fgetc(arquivo);
            }
            lexema[pos] = '\0';
            
            token.type = TOKEN_ID; 
            token.line = linha_atual;
            token.Token.table_index = buscar_ou_inserir_simbolo(lexema);
            sprintf(atributo_str, "%d", token.Token.table_index);
            imprimir_token(token.line, "TOKEN_ID (STRING)", atributo_str);
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
                token.type = TOKEN_KEYWORD;
                token.line = linha_atual;
                token.Token.table_index = buscar_ou_inserir_simbolo("/");
                imprimir_token(token.line, "TOKEN_KEYWORD", "/");
                c_atual = fgetc(arquivo);
                return token;
            }
        }

        if (c_atual == '<') {
            int proximo = fgetc(arquivo);
            token.line = linha_atual;
            if (proximo == '-') {
                token.type = TOKEN_KEYWORD;
                token.Token.table_index = buscar_ou_inserir_simbolo("<-");
                imprimir_token(token.line, "TOKEN_KEYWORD", "<-");
                c_atual = fgetc(arquivo);
            } else if (proximo == '=') {
                token.type = TOKEN_OP_REL;
                token.Token.op_code = OP_LE;
                imprimir_token(token.line, "TOKEN_OP_REL", "OP_LE");
                c_atual = fgetc(arquivo);
            } else if (proximo == '>') {
                token.type = TOKEN_KEYWORD;
                token.Token.table_index = buscar_ou_inserir_simbolo("<>");
                imprimir_token(token.line, "TOKEN_KEYWORD", "<>");
                c_atual = fgetc(arquivo);
            } else {
                ungetc(proximo, arquivo);
                token.type = TOKEN_OP_REL;
                token.Token.op_code = OP_LT;
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
                token.Token.op_code = OP_GE;
                imprimir_token(token.line, "TOKEN_OP_REL", "OP_GE");
                c_atual = fgetc(arquivo);
            } else {
                ungetc(proximo, arquivo);
                token.type = TOKEN_OP_REL;
                token.Token.op_code = OP_GT;
                imprimir_token(token.line, "TOKEN_OP_REL", "OP_GT");
                c_atual = fgetc(arquivo);
            }
            return token;
        }

        if (c_atual == '=') {
            token.type = TOKEN_OP_REL;
            token.line = linha_atual;
            token.Token.op_code = OP_EQ;
            imprimir_token(token.line, "TOKEN_OP_REL", "OP_EQ");
            c_atual = fgetc(arquivo);
            return token;
        }

        const char *simbolos_validos = "()[],:+-*\\.";
        if (strchr(simbolos_validos, c_atual) != NULL) {
            token.type = TOKEN_KEYWORD;
            token.line = linha_atual;
            char simbolo_texto[2] = {(char) c_atual, '\0'};
            token.Token.table_index = buscar_ou_inserir_simbolo(simbolo_texto);
            imprimir_token(token.line, "TOKEN_KEYWORD", simbolo_texto);
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

// ===========================================================
// PARTE 2: ANALISADOR SINTÁTICO (PARSER)
// ===========================================================

attribute lookahead; 

void nextToken(void) {
    lookahead = obterToken();
}

const char* obter_texto_lookahead() {
    if (lookahead.type == TOKEN_EOF) return "EOF";
    if (lookahead.type == TOKEN_NUM_INT || lookahead.type == TOKEN_NUM_FLOAT) return "NUMERO";
    if (lookahead.type == TOKEN_OP_REL) return "OPERADOR_RELACIONAL";
    return tabela_simbolos[lookahead.Token.table_index];
}

void erroSintatico(const char *esperado) {
    printf("ERRO SINTATICO: esperado '%s', encontrado '%s' na linha %d\n", 
           esperado, obter_texto_lookahead(), lookahead.line);
    fprintf(saida, "ERRO SINTATICO: esperado '%s', encontrado '%s' na linha %d\n", 
           esperado, obter_texto_lookahead(), lookahead.line);
    fclose(saida);
    fclose(arquivo);
    exit(1);
}

void match(TokenNome tipo_esperado, const char *msg_erro) {
    if (lookahead.type == tipo_esperado) {
        nextToken();
    } else {
        erroSintatico(msg_erro);
    }
}

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

void parse_fator() {
    if (lookahead.type == TOKEN_NUM_INT || lookahead.type == TOKEN_NUM_FLOAT) {
        nextToken();
    } else if (lookahead.type == TOKEN_ID) {
        nextToken();
        if (strcmp(obter_texto_lookahead(), "[") == 0) {
            match_keyword("[");
            parse_expressao();
            match_keyword("]");
        } else if (strcmp(obter_texto_lookahead(), "(") == 0) {
            match_keyword("(");
            parse_expressao();
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

void parse_expressao() {
    parse_termo();
    while (strcmp(obter_texto_lookahead(), "+") == 0 || 
           strcmp(obter_texto_lookahead(), "-") == 0) {
        nextToken();
        parse_termo();
    }
    
    if (lookahead.type == TOKEN_OP_REL || 
        strcmp(obter_texto_lookahead(), "E") == 0 || 
        strcmp(obter_texto_lookahead(), "OU") == 0) {
        nextToken();
        parse_expressao();
    }
}

void parse_atribuicao() {
    match(TOKEN_ID, "Identificador");
    if (strcmp(obter_texto_lookahead(), "[") == 0) {
        match_keyword("[");
        parse_expressao();
        match_keyword("]");
    }
    match_keyword("<-");
    parse_expressao();
}

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
    else if (lookahead.type == TOKEN_ID) {
        parse_atribuicao();
    }
    else {
        erroSintatico("Comando valido (escreva, leia, se, para, enquanto, atribuicao)");
    }
}

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

void parse_secao_var() {
    if (strcmp(obter_texto_lookahead(), "var") == 0) {
        match_keyword("var");
        
        while (lookahead.type == TOKEN_ID) {
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

void parse_programa() {
    match_keyword("algoritmo");
    match(TOKEN_ID, "Nome do Algoritmo (String)");
    
    while (strcmp(obter_texto_lookahead(), "procedimento") == 0 || strcmp(obter_texto_lookahead(), "funcao") == 0) {
        if (strcmp(obter_texto_lookahead(), "procedimento") == 0) {
            match_keyword("procedimento");
            match(TOKEN_ID, "Nome do Procedimento");
            match_keyword("inicio");
            parse_comandos();
            match_keyword("fimprocedimento");
        } else {
            match_keyword("funcao");
            match(TOKEN_ID, "Nome da Funcao");
            match_keyword("(");
            while (strcmp(obter_texto_lookahead(), ")") != 0) nextToken();
            match_keyword(")");
            match_keyword(":");
            nextToken(); 
            match_keyword("inicio");
            parse_comandos();
            match_keyword("fimfuncao");
        }
    }

    parse_secao_var();
    
    match_keyword("inicio");
    parse_comandos();
    match_keyword("fimalgoritmo");
}

// ===========================================================
// PARTE 3: PROGRAMA PRINCIPAL
// ===========================================================
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
        fclose(arquivo);
        return 1;
    }

    // Inicializa o buffer do analisador
    c_atual = fgetc(arquivo);
    nextToken(); 

    // Dispara a validação sintática a partir da raiz da gramática
    parse_programa();

    // Se chegou até aqui sem abortar (exit(1)), está tudo correto
    printf("\nSUCESSO: Analise lexica e sintatica concluidas sem erros!\n");
    fprintf(saida, "\nSUCESSO: Analise lexica e sintatica concluidas sem erros!\n");

    fclose(saida);
    fclose(arquivo);
    return 0;
}