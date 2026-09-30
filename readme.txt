=============================================================================
PROJETO DE COMPILADORES - FASE 1: ANÁLISE LÉXICA E SINTÁTICA
=============================================================================
Integrantes do Grupo:
1. Beatriz Silva Nóbrega - 10435789
2. Felipe Marques Leite Martha - 10437877

=============================================================================
1. STATUS DO DESENVOLVIMENTO
=============================================================================
- Etapa 1 (Gramática Livre de Contexto): CONCLUÍDA.
- Etapa 2 (Analisador Léxico): CONCLUÍDA.
- Etapa 3 (Analisador Sintático): CONCLUÍDA.

=============================================================================
2. INSTRUÇÕES DE COMPILAÇÃO E EXECUÇÃO
=============================================================================
Projeto desenvolvido em C. Compilação (MinGW / VSCode):

> gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador

Execução (o nome do arquivo fonte é passado por linha de comando):

> .\compilador.exe nome_do_arquivo.txt

A saída (tokens ou mensagem de erro) aparece na tela e é gravada em
saida.txt. O programa retorna 0 em caso de sucesso e 1 em caso de erro
léxico ou sintático.

=============================================================================
3. ETAPA 1 - EXPRESSÕES REGULARES E GRAMÁTICA LIVRE DE CONTEXTO (GLC)
=============================================================================
A linguagem suportada é a definida nos exemplos do Anexo I do enunciado.

3.1 Expressões Regulares (Tokens)
-----------------------------------------------------------------------------
- Palavras Reservadas: algoritmo | var | inicio | fimalgoritmo | escreva | escreval | leia | se | entao | senao | fimse | para | de | ate | passo | faca | fimpara | enquanto | fimenquanto | vetor | procedimento | fimprocedimento | funcao | fimfuncao | retorne | inteiro | real | caractere | logico | verdadeiro | falso | E | OU | MOD
- Identificadores: letra (letra | digito | _)*
- Números Inteiros: digito+
- Números Reais: digito+ . digito+
- Strings: " (qualquer caractere exceto ") "
- Operadores Relacionais: < | <= | = | <> | > | >=
- Operadores Aritméticos: + | - | * | / | \
- Atribuição: <-
- Símbolos: ( | ) | [ | ] | : | , | ..
- Comentário: // até o fim da linha (ignorado)

3.2 Gramática Livre de Contexto (GLC) Fatorada
-----------------------------------------------------------------------------
Programa -> algoritmo TEXTO_STRING DeclRotinas SecaoVar SecaoInicio

DeclRotinas -> Rotina DeclRotinas | ε
Rotina -> Procedimento | Funcao
Procedimento -> procedimento TOKEN_ID Parametros inicio Comandos fimprocedimento
Funcao -> funcao TOKEN_ID Parametros : TipoBasico inicio Comandos fimfuncao

Parametros -> ( ListaParametros ) | ε
ListaParametros -> TOKEN_ID : TipoBasico MaisParametros | ε
MaisParametros -> , TOKEN_ID : TipoBasico MaisParametros | ε

SecaoVar -> var ListaDeclaracoes | ε
ListaDeclaracoes -> Declaracao ListaDeclaracoes | ε
Declaracao -> ListaIDs : TipoDef
ListaIDs -> TOKEN_ID MaisIDs
MaisIDs -> , TOKEN_ID MaisIDs | ε
TipoDef -> TipoBasico | vetor [ TOKEN_NUM_INT .. TOKEN_NUM_INT ] de TipoBasico
TipoBasico -> inteiro | real | caractere | logico

SecaoInicio -> inicio Comandos fimalgoritmo
Comandos -> Comando Comandos | ε
Comando -> ComandoID | ChamadaIO | Se | Para | Enquanto | Retorno

ComandoID -> TOKEN_ID RestoComandoID
RestoComandoID -> [ Expressao ] <- Expressao | <- Expressao | ( ListaExpressoes ) | ε
IndiceVetor -> [ Expressao ] | ε
ChamadaIO -> escreva ( ListaExpressoes ) | escreval ( ListaExpressoes ) | leia ( TOKEN_ID IndiceVetor )

Se -> se ( Expressao ) entao Comandos SenaoOpcional fimse
SenaoOpcional -> senao Comandos | ε
Para -> para TOKEN_ID de Expressao ate Expressao PassoOpcional faca Comandos fimpara
PassoOpcional -> passo Expressao | ε
Enquanto -> enquanto ( Expressao ) faca Comandos fimenquanto
Retorno -> retorne Expressao

ListaExpressoes -> Expressao MaisExpressoes
MaisExpressoes -> , Expressao MaisExpressoes | ε

Expressao -> Soma RestoExpressao
RestoExpressao -> OperadorRelacionalLogico Expressao | ε
Soma -> Termo RestoSoma
RestoSoma -> + Termo RestoSoma | - Termo RestoSoma | ε
Termo -> Fator RestoTermo
RestoTermo -> OperadorMult Fator RestoTermo | ε
Fator -> + Fator | - Fator | TOKEN_NUM_INT | TOKEN_NUM_FLOAT | TEXTO_STRING | verdadeiro | falso | TOKEN_ID IndiceOuChamada | ( Expressao )
IndiceOuChamada -> [ Expressao ] | ( ListaExpressoes ) | ε

OperadorRelacionalLogico -> TOKEN_OP_REL | <> | E | OU
OperadorMult -> * | / | \ | MOD

=============================================================================
4. DECISÕES DE DESIGN E ARQUITETURA
=============================================================================
- Estruturas Token, TokenNome e OpRelType conforme a Figura 2 do enunciado.
- O parser chama nextToken(), que chama obterToken() (analisador léxico) e
  guarda o resultado em lookahead (1 token de antecipação).
- Parser descendente recursivo, sem retrocesso, baseado na GLC da seção 3.2.
- A Figura 2 só define 6 tipos de token. Por isso palavras reservadas,
  símbolos ( ) [ ] , : + - * / \ .. e os operadores <- e <> são
  TOKEN_KEYWORD, com o lexema guardado na tabela de símbolos. O operador <>
  não tem código em OpRelType e é tratado como relacional no parser.
- Strings são TOKEN_ID cujo lexema começa com aspas (impresso como
  "TOKEN_ID (STRING)"). O parser diferencia string de identificador.
- A tabela de símbolos guarda identificadores, palavras reservadas,
  símbolos e strings. O atributo do token é o índice na tabela.
- Erros terminam o programa com retorno 1 e mensagem "ERRO LÉXICO" ou
  "ERRO SINTÁTICO" com a linha. A mensagem também vai para saida.txt.
- No Windows, o programa ajusta o console para UTF-8 (chcp 65001) para
  exibir os acentos das mensagens de erro.

=============================================================================
5. BUGS CONHECIDOS OU LIMITAÇÕES
=============================================================================
- Palavras reservadas são sensíveis a maiúsculas/minúsculas (E, OU e MOD
  em maiúsculas; as demais em minúsculas, como no Anexo I).
- Strings aceitam até 97 caracteres e devem estar em uma única linha.
  String maior ou sem aspas de fechamento gera ERRO LÉXICO.
- Identificadores e números com mais de 99 caracteres não são tratados.
- A tabela de símbolos suporta até 1000 símbolos distintos.
- Não há análise semântica (tipos, variáveis declaradas, número de
  argumentos). Um identificador sozinho é aceito como chamada de
  procedimento sem parâmetros.
- Números reais são exibidos com 2 casas decimais na saída dos tokens.
- Nenhum outro bug conhecido.