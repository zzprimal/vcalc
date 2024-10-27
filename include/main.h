#include "VCalcLexer.h"
#include "VCalcParser.h"

#include "ANTLRFileStream.h"
#include "CommonTokenStream.h"
#include "tree/ParseTree.h"
#include "tree/ParseTreeWalker.h"

#include "BackEnd.h"
#include "AstBuilder.h"
#include "Ast.h"
#include "AstVisitor.h"

#include <iostream>
#include <fstream>

int program_flags = 0;
#define DEBUG 1

void SetFlags(int argc, char **argv);
int main(int argc, char **argv);