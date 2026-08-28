#pragma once
#if defined(RUPA_PACKAGE_H)
InterpreterResult interpretNode(Node *node, int id, RuntimeEnv *env,
                                Error *error);
InterpreterResult interpretExpression(Node *node, int id, RuntimeEnv *env,
                                      Error *error);
InterpreterResult interpretStatement(Node *node, int id, RuntimeEnv *env,
                                     Error *error);

InterpreterResult interpretFunction(Node *node, AstNode *ast, RuntimeEnv *env,
                                    Error *error);
InterpreterResult interpretCall(Node *node, AstNode *ast, RuntimeEnv *env,
                                Error *error);
InterpreterResult interpretUpdate(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error);
InterpreterResult interpretSubscript(Node *node, AstNode *ast, RuntimeEnv *env,
                                     Error *error);
InterpreterResult interpretLoop(Node *node, AstNode *ast, RuntimeEnv *env,
                                Error *error);
InterpreterResult interpretCase(Node *node, AstNode *ast, RuntimeEnv *env,
                                Error *error);
InterpreterResult interpretObject(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error);
InterpreterResult interpretMember(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error);
InterpreterResult interpretAsync(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error);
InterpreterResult interpretAwait(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error);
InterpreterResult interpretStruct(Node *node, AstNode *ast, RuntimeEnv *env,
                                   Error *error);
InterpreterResult interpretMemberAssign(Node *node, AstNode *ast,
                                        RuntimeEnv *env, Error *error);

#endif
