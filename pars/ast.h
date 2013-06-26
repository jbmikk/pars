#ifndef AST_H
#define AST_H

typedef struct _Ast {
} Ast;

void ast_init(Ast *ast);
void ast_add(Ast *ast, int symbol, unsigned int index, unsigned int length);

#endif //AST_H
