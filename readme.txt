=============================================================================
PROJETO DE COMPILADORES - FASE 1: ANÁLISE LÉXICA E SINTÁTICA
=============================================================================
Integrantes do Grupo:
1. [Seu Nome Completo] - [Seu RA/Matrícula]
2. [Nome da sua Dupla] - [RA/Matrícula da Dupla]

Data de Entrega: 29/09/2026

=============================================================================
1. STATUS DO DESENVOLVIMENTO
=============================================================================
- Etapa 1 (Gramática Livre de Contexto): CONCLUÍDA.
- Etapa 2 (Analisador Léxico): [Em desenvolvimento / Concluída]
- Etapa 3 (Analisador Sintático): [Pendente / Em desenvolvimento / Concluída]

=============================================================================
2. INSTRUÇÕES DE COMPILAÇÃO E EXECUÇÃO
=============================================================================
Este projeto foi desenvolvido na linguagem C. Para testar o compilador 
utilizando o MinGW (conforme especificação), utilize o seguinte comando 
no terminal:

> gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador

Para executar a análise de um arquivo fonte em MiniVisualg, passe o nome do 
arquivo como argumento na linha de comando:

> ./compilador nome_do_arquivo.txt

=============================================================================
3. ETAPA 1 - EXPRESSÕES REGULARES E GRAMÁTICA LIVRE DE CONTEXTO (GLC)
=============================================================================
A linguagem suportada é o MiniVisualg (Visualg Simplificado), operando 
estritamente sobre as estruturas definidas no Anexo I.

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
Comando -> Atribuicao | ChamadaIO | Se | Para | Enquanto | ChamadaRotina | Retorno

Atribuicao -> TOKEN_ID IndiceVetor <- Expressao
IndiceVetor -> [ Expressao ] | ε
ChamadaIO -> escreva ( ListaExpressoes ) | escreval ( ListaExpressoes ) | leia ( TOKEN_ID IndiceVetor )

Se -> se ( Expressao ) entao Comandos SenaoOpcional fimse
SenaoOpcional -> senao Comandos | ε

Para -> para TOKEN_ID de Expressao ate Expressao PassoOpcional faca Comandos fimpara
PassoOpcional -> passo Expressao | ε

Enquanto -> enquanto ( Expressao ) faca Comandos fimenquanto

Retorno -> retorne Expressao
ChamadaRotina -> TOKEN_ID ArgumentosRotina
ArgumentosRotina -> ( ListaExpressoes ) | ε

ListaExpressoes -> Expressao MaisExpressoes | ε
MaisExpressoes -> , Expressao MaisExpressoes | ε

Expressao -> Termo ContinuacaoExpressao
ContinuacaoExpressao -> OperadorRelacionalLogico Expressao | ε
Termo -> Fator ContinuacaoTermo
ContinuacaoTermo -> OperadorAritmetico Termo | ε
Fator -> TOKEN_NUM_INT | TOKEN_NUM_FLOAT | TEXTO_STRING | verdadeiro | falso | TOKEN_ID IndiceOuChamada | ( Expressao )
IndiceOuChamada -> [ Expressao ] | ( ListaExpressoes ) | ε

OperadorRelacionalLogico -> TOKEN_OP_REL | E | OU
OperadorAritmetico -> + | - | * | / | \ | MOD

=============================================================================
4. DECISÕES DE DESIGN E ARQUITETURA
=============================================================================
[Descreva aqui as escolhas feitas por você e sua dupla durante o 
desenvolvimento. Exemplo: "Optamos por implementar o buffer do analisador 
léxico utilizando alocação dinâmica para evitar estouro de memória...", 
"O parser foi implementado utilizando Análise Descendente Recursiva..."]

=============================================================================
5. BUGS CONHECIDOS OU LIMITAÇÕES
=============================================================================
[Liste aqui qualquer comportamento inesperado ou erro que não conseguiram 
resolver a tempo. Se não houver bugs, escreva: "Nenhum bug conhecido."]