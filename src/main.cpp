#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace clang::tooling;
using namespace clang::driver;
using namespace llvm;

bool isSystemCModule(CXXRecordDecl* decl) {
  if (decl->hasDefinition()) {
    if (decl->getNumBases() > 0) {
      return true;
    }
  }

  return false;
}

class FindNamedClassVisitor
  : public RecursiveASTVisitor<FindNamedClassVisitor> {
public:
  explicit FindNamedClassVisitor(ASTContext *Context)
    : Context(Context) {}

  bool VisitCXXRecordDecl(CXXRecordDecl *decl) {
    if (isSystemCModule(decl)) {
      errs() << "Found SystemC module " << decl->getQualifiedNameAsString() << "\n";
    }
    
    return true;
  }

private:
  ASTContext* Context;
};

class FindNamedClassConsumer : public clang::ASTConsumer {
public:
  explicit FindNamedClassConsumer(ASTContext *Context)
    : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
private:
  FindNamedClassVisitor Visitor;
};

class FindNamedClassAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::unique_ptr<clang::ASTConsumer>(
        new FindNamedClassConsumer(&Compiler.getASTContext()));
  }
};

class FindNamedClassActionFactory : public FrontendActionFactory {
public:
  virtual FrontendAction* create() override {
    return new FindNamedClassAction();
  }
};

static cl::OptionCategory MyToolCategory("My tool options");

int main(int argc, const char **argv) {

  CommonOptionsParser options(argc, argv, MyToolCategory);
  const auto& sources = options.getSourcePathList();
  auto& db = options.getCompilations();
  
  ClangTool tool(db, sources);

  FindNamedClassActionFactory factory;
  tool.run(&factory);

  errs() << "Done\n";
  // if (argc > 1) {
  //   clang::tooling::runToolOnCode(new FindNamedClassAction, argv[1]);
  // }
}
