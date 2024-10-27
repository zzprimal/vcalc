#include "main.h"

void SetFlags(int argc, char **argv){
  for (int i = 0; i < argc; i++){
    if (!strcmp(argv[i], "--debug")){
      program_flags |= DEBUG;
    }
  }
}

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "Missing required argument.\n"
              << "Required arguments: <input file path> <output file path>\n";
    return 1;
  }
  SetFlags(argc, argv);

  // Open the file then parse and lex it.
  antlr4::ANTLRFileStream afs;
  afs.loadFromFile(argv[1]);
  vcalc::VCalcLexer lexer(&afs);
  antlr4::CommonTokenStream tokens(&lexer);
  vcalc::VCalcParser parser(&tokens);

  // Get the root of the parse tree. Use your base rule name.
  antlr4::tree::ParseTree *tree = parser.file();
  AstBuilder::AstBuild tree_builder;
  std::shared_ptr<Ast::AstNode> AstTree = std::any_cast<std::shared_ptr<Ast::AstNode>>(tree_builder.visit(tree));

  if (program_flags & DEBUG){
    AstVisitor::AstDebugger walker;
    walker.DfsTraversal(AstTree);
  }

  if (program_flags & DEBUG){
    std::cout << "Building Scope Tree and Validating Types" << std::endl;
  }

  AstVisitor::DefRef def_ref_visitor;
  def_ref_visitor.Visit(AstTree);
  if (program_flags & DEBUG){
    std::cout << "Scope Tree Built and Types Validated" << std::endl << std::endl;
  }
  AstVisitor::CodeGen code_gen_visitor;
  code_gen_visitor.GenerateMlir(true, AstTree);
  std::ofstream os(argv[2]);
  code_gen_visitor.lowerDialects();
  code_gen_visitor.dumpLLVM(os);

  return 0;
  
}
