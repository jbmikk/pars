#ifndef ASTLISTENER_H
#define ASTLISTENER_H

int ast_parser_pipe(void *_context, void *_cont);
int ast_parse_start(void *object, void *params);
int ast_parse_end(void *object, void *params);
int ast_parse_error(void *object, void *params);

#endif //ASTLISTENER_H
